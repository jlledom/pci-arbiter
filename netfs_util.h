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

/* Header for utility functions */

#ifndef NETFS_UTIL_H
#define NETFS_UTIL_H

#include <hurd/netfs.h>

#include <netfs_impl.h>

error_t create_dir_entry (int32_t domain, int16_t bus, int16_t dev,
			  int16_t func, int32_t device_class, char *name,
			  struct pci_dirent * parent, io_statbuf_t stat,
			  struct node * node, struct pci_dirent * entry);
error_t create_root_node (file_t underlying_node, struct node **root_node);
error_t create_node (struct pci_dirent *e, struct node **node);
void destroy_node (struct node *node);

#endif /* NETFS_UTIL_H */
