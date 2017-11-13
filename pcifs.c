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
#include <hurd/netfs.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#include <ncache.h>

static error_t
create_dir_entry (int32_t domain, int16_t bus, int16_t dev,
		  int16_t func, int32_t device_class, char *name,
		  struct pcifs_dirent *parent, io_statbuf_t stat,
		  struct node *node, struct pcifs_dirent *entry)
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
  entry->node = node;

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
  np->nn_stat.st_mode = S_IFDIR | (underlying_node_stat.st_mode
				   & ~S_IFMT & ~S_ITRANS);
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

  /* Remove all permissions to others */
  np->nn_stat.st_mode &= ~(S_IROTH | S_IWOTH | S_IXOTH);

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
    create_dir_entry (-1, -1, -1, -1, -1, "", 0, np->nn_stat, np,
		      fs->entries);

  fs->num_entries = 1;
  fs->root = netfs_root_node = np;
  fs->root->nn->ln = fs->entries;
  fs->params.node_cache_max = 16;
  pthread_mutex_init (&fs->node_cache_lock, 0);

  return 0;
}

error_t
create_fs_tree (struct pcifs * fs, struct pci_system * pci_sys)
{
  error_t err = 0;
  int c_domain, c_bus, c_dev, i;
  size_t nentries;
  struct pci_device *device;
  struct pcifs_dirent *e, *domain_parent, *bus_parent, *dev_parent, *list;
  struct stat e_stat;
  char entry_name[NAME_SIZE];

  nentries = 2;			/* Skip root and domain entries */
  c_bus = c_dev = -1;
  for (i = 0, device = pci_sys->devices; i < pci_sys->num_devices; i++)
    {
      if (device->bus != c_bus)
	{
	  c_bus = device->bus;
	  nentries++;
	}

      if (device->dev != c_dev)
	{
	  c_bus = device->bus;
	  nentries++;
	}

      nentries++;
    }

  list = realloc (fs->entries, nentries * sizeof (struct pcifs_dirent));
  if (!list)
    return ENOMEM;

  /* Add an entry for domain = 0. We still don't support PCI express */
  e = list + 1;
  c_domain = 0;
  memset (entry_name, 0, NAME_SIZE);
  snprintf (entry_name, NAME_SIZE, "%04x", c_domain);
  err = create_dir_entry (c_domain, -1, -1, -1, -1, entry_name, list,
			  list->stat, 0, e);
  if (err)
    return err;

  c_bus = c_dev = -1;
  bus_parent = dev_parent = 0;
  domain_parent = e++;
  for (i = 0, device = pci_sys->devices; i <= pci_sys->num_devices;
       i++, device++)
    {
      if (device->bus != c_bus)
	{
	  /* We've found a new bus. Add entry for it */
	  memset (entry_name, 0, NAME_SIZE);
	  snprintf (entry_name, NAME_SIZE, "%02x", device->bus);
	  err =
	    create_dir_entry (c_domain, device->bus, -1, -1, -1, entry_name,
			      domain_parent, domain_parent->stat, 0, e);
	  if (err)
	    return err;

	  /* Switch to dev level */
	  bus_parent = e++;
	  c_bus = device->bus;
	  c_dev = -1;
	}

      if (device->dev != c_dev)
	{
	  /* We've found a new dev. Add entry for it */
	  memset (entry_name, 0, NAME_SIZE);
	  snprintf (entry_name, NAME_SIZE, "%02x", device->dev);
	  err =
	    create_dir_entry (c_domain, device->bus, device->dev, -1, -1,
			      entry_name, bus_parent, bus_parent->stat, 0, e);
	  if (err)
	    return err;

	  /* Switch to func level */
	  dev_parent = e++;
	  c_dev = device->dev;
	}

      /* Add func entry */
      memset (entry_name, 0, NAME_SIZE);
      snprintf (entry_name, NAME_SIZE, "%01u", device->func);
      /* Remove directory mode as this is the lowest level */
      e_stat = dev_parent->stat;
      e_stat.st_mode &= ~S_IFDIR;
      err =
	create_dir_entry (c_domain, device->bus, device->dev, device->func,
			  device->device_class, entry_name, dev_parent,
			  e_stat, 0, e++);
      if (err)
	return err;
    }

  /* The root node points to the first element of the entry list */
  fs->entries = list;
  fs->num_entries = nentries;
  fs->root->nn->ln = fs->entries;

  return err;
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
	e->stat.st_uid = p->uid;
      if (p->gid >= 0)
	e->stat.st_gid = p->gid;

      /* Update ctime */
      fshelp_touch (&e->stat, TOUCH_CTIME, pcifs_maptime);

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
    /* Set new permissions, if any */
    entry_set_perms (fs, e);

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
