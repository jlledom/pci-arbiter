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

/* Translator global declarations */

#ifndef PCI_ARBITER_H
#define PCI_ARBITER_H

#include <hurd/ports.h>

/* Libports stuff */
struct port_bucket *pci_bucket;

struct port_class *pci_protid_portclass;
struct port_class *pci_cntl_portclass;

mach_port_t fsys_identity;

/* Trivfs control structure for lwip.  */
struct trivfs_control *pcicntl;

/* Owner of the underlying node.  */
uid_t pci_owner;

/* Group of the underlying node.  */
uid_t pci_group;

#endif /* PCI_ARBITER_H */
