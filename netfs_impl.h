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

/* Data types required to create a directory tree using libnetfs */

#ifndef NETFS_IMPL_H
#define NETFS_IMPL_H

#include <hurd/netfs.h>

#ifndef NAME_SIZE
#define NAME_SIZE 16
#endif

/* 
 * Directory entriy. Contains all per-node data our problem requires.
 * 
 * All directory entries are created on startup and used to generate the
 * fs tree and create or retrieve libnetfs node objects.
 * 
 * From libnetfs' point of view, these are the light nodes.
 */
struct pci_dirent
{
  /*
   * Complete bus identification, including domain, of the device.  On
   * platforms that do not support PCI domains (e.g., 32-bit x86 hardware),
   * the domain will always be zero.
   * 
   * Negative value means no value.
   */
  int32_t domain;
  int16_t bus;
  int16_t dev;
  int8_t func;

  /*
   * Device's class, subclass, and programming interface packed into a
   * single 32-bit value.  The class is at bits [23:16], subclass is at
   * bits [15:8], and programming interface is at [7:0].
   * 
   * Negative value means no value.
   */
  int32_t device_class;

  char name[NAME_SIZE];
  struct pci_dirent *parent;
  io_statbuf_t stat;

  /*
   * We only need two kind of nodes: files and directories.
   * When `dir' is null, this is a file; when not null, it's a directory.
   */
  struct pci_dir *dir;

  /* Active node on this entry */
  struct node *node;
};

/*
 * A directory, it only contains a list of directory entries
 */
struct pci_dir
{
  /* Number of directory entries */
  uint16_t num_entries;

  /* Array of directory entries */
  struct pci_dirent **entries;
};

/*
 * A netnode has a 1:1 relation with a generic libnetfs node. Hence, it's only
 * purpose is to extend a generic node to add the new attributes our problem
 * requires.
 */
struct netnode
{
  /* Light node */
  struct pci_dirent *ln;
};

#endif /* NETFS_IMPL_H */
