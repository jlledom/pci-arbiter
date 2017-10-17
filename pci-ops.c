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

#include <hurd/fshelp.h>

#include <pci_arbiter.h>
#include <pci_access.h>

error_t
S_pci_read (struct trivfs_protid *master, int bus, int dev, int func,
	    int reg, char **data, size_t * datalen,
	    mach_msg_type_number_t amount)
{
  error_t err;

  if (!master)
    return EOPNOTSUPP;

  if (!master->isroot)
    {
      struct stat st;

      st.st_uid = pci_owner;
      st.st_gid = pci_group;

      err = fshelp_isowner (&st, master->user);
      if (err)
	return EPERM;
    }

  if (amount > *datalen)
    amount = *datalen;

  err = pci_ifc->read (bus, dev, func, reg, *data, amount);

  if(!err)
    *datalen = amount;

  return err;
}

error_t
S_pci_write (struct trivfs_protid * master, int bus, int dev, int func,
	     int reg, char *data, size_t datalen,
	     mach_msg_type_number_t * amount)
{
  error_t err;

  if (!master)
    return EOPNOTSUPP;

  if (!master->isroot)
    {
      struct stat st;

      st.st_uid = pci_owner;
      st.st_gid = pci_group;

      err = fshelp_isowner (&st, master->user);
      if (err)
	return EPERM;
    }

  err = pci_ifc->write (bus, dev, func, reg, data, datalen);

  *amount = datalen;

  return err;
}
