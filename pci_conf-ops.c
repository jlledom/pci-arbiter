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

#include <pci_conf_S.h>

#include <fcntl.h>
#include <hurd/netfs.h>

#include <pci_arbiter.h>
#include <pci_access.h>
#include <netfs_impl.h>

/*
 * Read min(amount,*datalen) bytes and store them on `*data'.
 *
 * `*datalen' is updated.
 */
error_t
S_pci_conf_read (struct protid * master, int bus, int dev, int func,
		 int reg, char **data, size_t * datalen,
		 mach_msg_type_number_t amount)
{
  error_t err;
  struct node *node;
  pthread_mutex_t *lock;

  if (!master)
    return EOPNOTSUPP;

  node = master->po->np;
  lock = &node->nn->fs->pci_conf_lock;

  err =
    netfs_check_open_permissions (master->user, node, O_READ, 0);
  if (err)
    return EPERM;

  /*
   * We don't allocate new memory since we expect no more than 4 bytes-long
   * buffers. Instead, we just take the lower value as length.
   */
  if (amount > *datalen)
    amount = *datalen;

  /*
   * The server is not single-threaded anymore. Incoming rpcs are handled by
   * libnetfs which is multi-threaded. A lock is needed for arbitration.
   */
  pthread_mutex_lock (lock);
  err = pci_ifc->read (bus, dev, func, reg, *data, amount);
  pthread_mutex_unlock (lock);

  if (!err)
    *datalen = amount;

  return err;
}

/* Write `datalen' bytes from `data'. `amount' is updated. */
error_t
S_pci_conf_write (struct protid * master, int bus, int dev, int func,
		  int reg, char *data, size_t datalen,
		  mach_msg_type_number_t * amount)
{
  error_t err;
  struct node *node;
  pthread_mutex_t *lock;

  if (!master)
    return EOPNOTSUPP;

  node = master->po->np;
  lock = &node->nn->fs->pci_conf_lock;

  err =
    netfs_check_open_permissions (master->user, node, O_WRITE, 0);
  if (err)
    return EPERM;

  pthread_mutex_lock (lock);
  err = pci_ifc->write (bus, dev, func, reg, data, datalen);
  pthread_mutex_unlock (lock);

  *amount = datalen;

  return err;
}
