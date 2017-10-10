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

/* Implementation of PCI operations */

#include <pci_S.h>

error_t
S_pci_read (struct trivfs_protid *master, int bus, int dev, int func, int reg,
	    char **data, size_t * datalen, mach_msg_type_number_t amount)
{
  return EOPNOTSUPP;
}

error_t
S_pci_write (struct trivfs_protid * master, int bus, int dev, int func,
	     int reg, char *data, size_t datalen,
	     mach_msg_type_number_t * amount)
{
  return EOPNOTSUPP;
}
