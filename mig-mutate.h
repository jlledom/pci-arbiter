/*
   Copyright (C) 2017 Free Software Foundation, Inc.
   Written by Michael I. Bushnell, p/BSG.

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

/* Only CPP macro definitions should go in this file. */

#define PCI_IMPORTS				\
  import "../libtrivfs/mig-decls.h";		\

#define PCI_INTRAN trivfs_protid_t trivfs_begin_using_protid (pci_t)
#define PCI_INTRAN_PAYLOAD trivfs_protid_t trivfs_begin_using_protid_payload
#define PCI_DESTRUCTOR trivfs_end_using_protid (trivfs_protid_t)
