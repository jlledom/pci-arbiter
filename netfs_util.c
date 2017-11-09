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

/* Utility functions */

#include <netfs_util.h>

#include <stdio.h>
#include <hurd/netfs.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#include <pci_arbiter.h>

static error_t
create_dir_entry (int32_t domain, int16_t bus, int16_t dev,
		  int16_t func, int32_t device_class, char *name,
		  struct pci_dirent *parent, io_statbuf_t stat,
		  struct node *node, struct pci_dirent *entry)
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
	  entry->parent->dir = calloc (1, sizeof (struct pci_dir));
	  if (!entry->parent->dir)
	    return ENOMEM;
	}

      parent_num_entries = entry->parent->dir->num_entries++;
      entry->parent->dir->entries = realloc (entry->parent->dir->entries,
					     entry->parent->dir->num_entries *
					     sizeof (struct pci_dirent *));
      if (!entry->parent->dir->entries)
	return ENOMEM;
      entry->parent->dir->entries[parent_num_entries] = entry;
    }

  return 0;
}

error_t
create_file_system (file_t underlying_node, struct pcifs ** fs)
{
  error_t err;
  struct node *np;
  struct netnode *nn;
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

  nn = netfs_node_netnode (np);
  nn->ln = calloc (1, sizeof (struct pci_dirent));
  if (!nn->ln)
    {
      free (np);
      return ENOMEM;
    }

  err = create_dir_entry (-1, -1, -1, -1, -1, "", 0, np->nn_stat, np, nn->ln);

  *fs = calloc (1, sizeof (struct pcifs));
  if (!*fs)
    return ENOMEM;

  (*fs)->root = netfs_root_node = np;
  (*fs)->params.node_cache_max = 16;
  pthread_mutex_init (&(*fs)->node_cache_lock, 0);
  nn->fs = *fs;

  return 0;
}

error_t
create_fs_tree (struct pcifs * fs, struct pci_system * pci_sys)
{
  error_t err = 0;
  int c_domain, c_bus, c_dev, i;
  size_t nentries;
  struct pci_device *device;
  struct pci_dirent *e, *domain_parent, *bus_parent, *dev_parent, *list;
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

  list = realloc (fs->root->nn->ln, nentries * sizeof (struct pci_dirent));
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
  domain_parent = e++;
  for (i = 0, device = pci_sys->devices; i <= pci_sys->num_devices; i++)
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
      err =
	create_dir_entry (c_domain, device->bus, device->dev, device->func,
			  device->device_class, entry_name, dev_parent,
			  dev_parent->stat, 0, e++);
      if (err)
	return err;

      device++;
    }

  /* The root node points to the first element of the entry list */
  fs->root->nn->ln = list;

  return err;
}

error_t
create_node (struct pci_dirent * e, struct node ** node)
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
  nn->fs = fs;

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
