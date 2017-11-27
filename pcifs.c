/*
   Copyright (C) 2017 Free Software Foundation, Inc.

   This file is part of the GNU Hurd.

   The GNU Hurd is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2, or (at
   your option) any later version.

   The GNU Hurd is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with the GNU Hurd.  If not, see <http://www.gnu.org/licenses/>.
*/

/* PCI Filesystem implementation */

#include <pcifs.h>

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <hurd/netfs.h>

#include <ncache.h>
#include <func_files.h>

static error_t
create_dir_entry (int32_t domain, int16_t bus, int16_t dev,
		  int16_t func, int32_t device_class, char *name,
		  struct pcifs_dirent *parent, io_statbuf_t stat,
		  struct node *node, struct pci_device *device,
		  struct pcifs_dirent *entry)
{
  uint16_t parent_num_entries;

  entry->domain = domain;
  entry->bus = bus;
  entry->dev = dev;
  entry->func = func;
  entry->device_class = device_class;
  strncpy (entry->name, name, NAME_SIZE);
  entry->parent = parent;
  entry->stat = stat;
  entry->dir = 0;
  entry->node = node;
  entry->device = device;

  /* Update parent's child list */
  if (entry->parent)
    {
      if (!entry->parent->dir)
	{
	  /* First child */
	  entry->parent->dir = calloc (1, sizeof (struct pcifs_dir));
	  if (!entry->parent->dir)
	    return ENOMEM;
	}

      parent_num_entries = entry->parent->dir->num_entries++;
      entry->parent->dir->entries = realloc (entry->parent->dir->entries,
					     entry->parent->dir->num_entries *
					     sizeof (struct pcifs_dirent *));
      if (!entry->parent->dir->entries)
	return ENOMEM;
      entry->parent->dir->entries[parent_num_entries] = entry;
    }

  return 0;
}

error_t
alloc_file_system (struct pcifs ** fs)
{
  *fs = calloc (1, sizeof (struct pcifs));
  if (!*fs)
    return ENOMEM;

  return 0;
}

error_t
init_file_system (file_t underlying_node, struct pcifs * fs)
{
  error_t err;
  struct node *np;
  io_statbuf_t underlying_node_stat;

  /* Initialize status from underlying node.  */
  err = io_stat (underlying_node, &underlying_node_stat);
  if (err)
    return err;

  np = netfs_make_node_alloc (sizeof (struct netnode));
  if (!np)
    return ENOMEM;
  np->nn_stat = underlying_node_stat;
  np->nn_stat.st_fsid = getpid ();
  np->nn_stat.st_mode = S_IFDIR | S_IROOT
    | (underlying_node_stat.st_mode & ~S_IFMT & ~S_ITRANS);
  np->nn_translated = np->nn_stat.st_mode;

  /* If the underlying node isn't a directory, enhance the stat
     information.  */
  if (!S_ISDIR (underlying_node_stat.st_mode))
    {
      if (underlying_node_stat.st_mode & S_IRUSR)
	np->nn_stat.st_mode |= S_IXUSR;
      if (underlying_node_stat.st_mode & S_IRGRP)
	np->nn_stat.st_mode |= S_IXGRP;
      if (underlying_node_stat.st_mode & S_IROTH)
	np->nn_stat.st_mode |= S_IXOTH;
    }

  /* Remove write permissions to others */
  np->nn_stat.st_mode &= ~S_IWOTH;

  /* Set times to now */
  fshelp_touch (&np->nn_stat, TOUCH_ATIME | TOUCH_MTIME | TOUCH_CTIME,
		pcifs_maptime);

  fs->entries = calloc (1, sizeof (struct pcifs_dirent));
  if (!fs->entries)
    {
      free (fs->entries);
      return ENOMEM;
    }

  err =
    create_dir_entry (-1, -1, -1, -1, -1, "", 0, np->nn_stat, np, 0,
		      fs->entries);

  fs->num_entries = 1;
  fs->root = netfs_root_node = np;
  fs->root->nn->ln = fs->entries;
  fs->params.node_cache_max = NODE_CACHE_MAX;
  pthread_mutex_init (&fs->node_cache_lock, 0);

  return 0;
}

error_t
create_fs_tree (struct pcifs * fs, struct pci_system * pci_sys)
{
  error_t err = 0;
  int c_domain, c_bus, c_dev, i, j;
  size_t nentries;
  struct pci_device *device;
  struct pcifs_dirent *e, *domain_parent, *bus_parent, *dev_parent,
    *func_parent, *list;
  struct stat e_stat;
  char entry_name[NAME_SIZE];

  nentries = 2;			/* Skip root and domain entries */
  c_bus = c_dev = -1;
  for (i = 0, device = pci_sys->devices; i < pci_sys->num_devices;
       i++, device++)
    {
      if (device->bus != c_bus)
	{
	  c_bus = device->bus;
	  c_dev = -1;
	  nentries++;
	}

      if (device->dev != c_dev)
	{
	  c_dev = device->dev;
	  nentries++;
	}

      nentries += 2;		/* func dir + config */

      for (j = 0; j < 6; j++)
	if (device->regions[j].size > 0)
	  nentries++;		/* + memory region */

      if (device->rom_size)
	nentries++;		/* + rom */
    }

  list = realloc (fs->entries, nentries * sizeof (struct pcifs_dirent));
  if (!list)
    return ENOMEM;

  /* Add an entry for domain = 0. We still don't support PCI express */
  e = list + 1;
  c_domain = 0;
  e_stat = list->stat;
  e_stat.st_mode &= ~S_IROOT;	/* Remove the root mode */
  memset (entry_name, 0, NAME_SIZE);
  snprintf (entry_name, NAME_SIZE, "%04x", c_domain);
  err = create_dir_entry (c_domain, -1, -1, -1, -1, entry_name, list,
			  e_stat, 0, 0, e);
  if (err)
    return err;

  c_bus = c_dev = -1;
  bus_parent = dev_parent = func_parent = 0;
  domain_parent = e++;
  for (i = 0, device = pci_sys->devices; i < pci_sys->num_devices;
       i++, device++)
    {
      if (device->bus != c_bus)
	{
	  /* We've found a new bus. Add an entry for it */
	  memset (entry_name, 0, NAME_SIZE);
	  snprintf (entry_name, NAME_SIZE, "%02x", device->bus);
	  err =
	    create_dir_entry (c_domain, device->bus, -1, -1, -1, entry_name,
			      domain_parent, domain_parent->stat, 0, 0, e);
	  if (err)
	    return err;

	  /* Switch to dev level */
	  bus_parent = e++;
	  c_bus = device->bus;
	  c_dev = -1;
	}

      if (device->dev != c_dev)
	{
	  /* We've found a new dev. Add an entry for it */
	  memset (entry_name, 0, NAME_SIZE);
	  snprintf (entry_name, NAME_SIZE, "%02x", device->dev);
	  err =
	    create_dir_entry (c_domain, device->bus, device->dev, -1, -1,
			      entry_name, bus_parent, bus_parent->stat, 0, 0,
			      e);
	  if (err)
	    return err;

	  /* Switch to func level */
	  dev_parent = e++;
	  c_dev = device->dev;
	}

      /* Remove all permissions to others */
      e_stat = dev_parent->stat;
      e_stat.st_mode &= ~(S_IROTH | S_IWOTH | S_IXOTH);

      /* Add func entry */
      memset (entry_name, 0, NAME_SIZE);
      snprintf (entry_name, NAME_SIZE, "%01u", device->func);
      err =
	create_dir_entry (c_domain, device->bus, device->dev, device->func,
			  device->device_class, entry_name, dev_parent,
			  e_stat, 0, device, e);
      if (err)
	return err;

      /* Switch to the lowest level */
      func_parent = e++;

      /* Change mode to a regular file */
      e_stat = func_parent->stat;
      e_stat.st_mode &= ~(S_IFDIR | S_IXUSR | S_IXGRP);
      e_stat.st_size = FILE_CONFIG_SIZE;

      /* Create config entry */
      strncpy (entry_name, FILE_CONFIG_NAME, NAME_SIZE);
      err =
	create_dir_entry (c_domain, device->bus, device->dev, device->func,
			  device->device_class, entry_name, func_parent,
			  e_stat, 0, device, e++);
      if (err)
	return err;

      /* Create regions entries */
      for (j = 0; j < 6; j++)
	{
	  if (device->regions[j].size > 0)
	    {
	      e_stat.st_size = device->regions[j].size;
	      snprintf (entry_name, NAME_SIZE, "%s%01u", FILE_REGION_NAME, j);
	      err =
		create_dir_entry (c_domain, device->bus, device->dev,
				  device->func, device->device_class,
				  entry_name, func_parent, e_stat, 0, device,
				  e++);
	      if (err)
		return err;
	    }
	}

      /* Create rom entry */
      if (device->rom_size)
	{
	  /* Make rom is read only */
	  e_stat.st_mode &= ~(S_IWUSR | S_IWGRP);
	  e_stat.st_size = device->rom_size;
	  strncpy (entry_name, FILE_ROM_NAME, NAME_SIZE);
	  err =
	    create_dir_entry (c_domain, device->bus, device->dev,
			      device->func, device->device_class, entry_name,
			      func_parent, e_stat, 0, device, e++);
	  if (err)
	    return err;
	}
    }

  /* The root node points to the first element of the entry list */
  fs->entries = list;
  fs->num_entries = nentries;
  fs->root->nn->ln = fs->entries;

  return err;
}

error_t
entry_check_perms (struct iouser * user, struct pcifs_dirent * e, int flags)
{
  error_t err = 0;

  if (!err && (flags & O_READ))
    err = fshelp_access (&e->stat, S_IREAD, user);
  if (!err && (flags & O_WRITE))
    err = fshelp_access (&e->stat, S_IWRITE, user);
  if (!err && (flags & O_EXEC))
    err = fshelp_access (&e->stat, S_IEXEC, user);

  return err;
}

static void
entry_default_perms (struct pcifs *fs, struct pcifs_dirent *e)
{
  int i;
  struct pcifs_perm *perms = fs->params.perms, *p;
  size_t num_perms = fs->params.num_perms;

  for (i = 0, p = perms; i < num_perms; i++, p++)
    {
      /* Set default owner and group */
      UPDATE_OWNER (e, fs->root->nn->ln->stat.st_uid);
      UPDATE_GROUP (e, fs->root->nn->ln->stat.st_gid);

      /* Update ctime */
      UPDATE_TIMES (e, TOUCH_CTIME);
    }

  return;
}

static void
entry_set_perms (struct pcifs *fs, struct pcifs_dirent *e)
{
  int i;
  struct pcifs_perm *perms = fs->params.perms, *p;
  size_t num_perms = fs->params.num_perms;

  for (i = 0, p = perms; i < num_perms; i++, p++)
    {
      uint8_t e_class = e->device_class >> 16;
      uint8_t e_subclass = ((e->device_class >> 8) & 0xFF);

      /* Check whether the entry is convered by this permission scope */
      if (p->d_class >= 0 && e_class != p->d_class)
	continue;
      if (p->d_subclass >= 0 && e_subclass != p->d_subclass)
	continue;
      if (p->domain >= 0 && p->domain != e->domain)
	continue;
      if (p->bus >= 0 && e->bus != p->bus)
	continue;
      if (p->dev >= 0 && e->dev != p->dev)
	continue;
      if (p->func >= 0 && e->func != p->func)
	continue;

      /* This permission set covers this entry */
      if (p->uid >= 0)
	UPDATE_OWNER (e, p->uid);
      if (p->gid >= 0)
	UPDATE_GROUP (e, p->gid);

      /* Update ctime */
      UPDATE_TIMES (e, TOUCH_CTIME);

      /* Break, as only one permission set can cover each node */
      break;
    }

  return;
}

error_t
fs_set_permissions (struct pcifs * fs)
{
  int i;
  struct pcifs_dirent *e;

  for (i = 0, e = fs->entries; i < fs->num_entries; i++, e++)
    {
      /* Restore default perms, as this may be called from fsysopts */
      entry_default_perms (fs, e);

      /* Set new permissions, if any */
      entry_set_perms (fs, e);
    }

  return 0;
}

error_t
create_node (struct pcifs_dirent * e, struct node ** node)
{
  struct node *np;
  struct netnode *nn;

  np = netfs_make_node_alloc (sizeof (struct netnode));
  if (!np)
    return ENOMEM;
  np->nn_stat = e->stat;
  np->nn_translated = np->nn_stat.st_mode;

  nn = netfs_node_netnode (np);
  memset (nn, 0, sizeof (struct netnode));
  nn->ln = e;

  *node = e->node = np;

  return 0;
}

void
destroy_node (struct node *node)
{
  if (node->nn->ln)
    node->nn->ln->node = 0;
  free (node);
}
