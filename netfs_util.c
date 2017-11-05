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

#include <hurd/netfs.h>
#include <string.h>
#include <unistd.h>

error_t
create_dir_entry (int32_t domain, int16_t bus, int16_t dev,
		  int16_t func, int32_t device_class, char *name,
		  struct pci_dirent *parent, io_statbuf_t stat,
		  struct pci_dirent *entry)
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
create_root_node (file_t underlying_node, struct node ** root_node)
{
  error_t err;
  struct node *np;
  struct netnode *nn;
  io_statbuf_t underlying_node_stat;

  /* Initialize status from underlying node.  */
  err = io_stat (underlying_node, &underlying_node_stat);
  if (err)
    return err;
  nn = calloc (1, sizeof (struct netnode));
  if (!nn)
    return ENOMEM;

  nn->ln = calloc (1, sizeof (struct pci_dirent));
  if (!nn->ln)
    {
      free (nn);
      return ENOMEM;
    }

  np = netfs_make_node (nn);
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

  err = create_dir_entry (-1, -1, -1, -1, -1, "", 0, np->nn_stat, nn->ln);

  *root_node = np;

  return 0;
}
