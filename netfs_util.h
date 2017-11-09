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

#include <pci_access.h>
#include <netfs_impl.h>

error_t create_file_system (file_t underlying_node, struct pcifs **root_node);
error_t create_fs_tree (struct pcifs *fs, struct pci_system *pci_sys);
error_t create_node (struct pci_dirent *e, struct node **node);
void destroy_node (struct node *node);

#endif /* NETFS_UTIL_H */
