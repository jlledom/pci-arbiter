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

#include <pcifs.h>
#include <pci_access.h>
#include <netfs_impl.h>

static error_t
check_permissions (struct protid *master, int bus, int dev, int func,
		   int flags)
{
  error_t err = 0;
  struct node *node;
  struct pcifs_dirent *e;

  node = master->po->np;
  e = node->nn->ln;

  /* Check wheter the user has permissions to access this node */
  err = netfs_check_open_permissions (master->user, node, flags, 0);
  if (err)
    return err;

  /* Check wheter the request has been sent to the proper node */
  if (e->domain > 0)
    err = EINVAL;		/* Only domain 0 can be accessed by I/O ports */
  if (e->bus >= 0 && bus != e->bus)
    err = EPERM;
  if (e->dev >= 0 && dev != e->dev)
    err = EPERM;
  if (e->func >= 0 && func != e->func)
    err = EPERM;

  return err;
}

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
  pthread_mutex_t *lock;
  struct stat *e_stat;

  if (!master)
    return EOPNOTSUPP;

  lock = &fs->pci_conf_lock;

  err = check_permissions (master, bus, dev, func, O_READ);
  if (err)
    return err;

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
  err = pci_sys->read (bus, dev, func, reg, *data, amount);
  pthread_mutex_unlock (lock);

  if (!err)
    {
      *datalen = amount;
      /* Update atime, only if this is not a directory */
      e_stat = &master->po->np->nn->ln->stat;
      if (!S_ISDIR (e_stat->st_mode))
	fshelp_touch (e_stat, TOUCH_ATIME, pcifs_maptime);
    }

  return err;
}

/* Write `datalen' bytes from `data'. `amount' is updated. */
error_t
S_pci_conf_write (struct protid * master, int bus, int dev, int func,
		  int reg, char *data, size_t datalen,
		  mach_msg_type_number_t * amount)
{
  error_t err;
  pthread_mutex_t *lock;
  struct stat *e_stat;

  if (!master)
    return EOPNOTSUPP;

  lock = &fs->pci_conf_lock;

  err = check_permissions (master, bus, dev, func, O_WRITE);
  if (err)
    return err;

  pthread_mutex_lock (lock);
  err = pci_sys->write (bus, dev, func, reg, data, datalen);
  pthread_mutex_unlock (lock);

  if (!err)
    {
      *amount = datalen;
      /* Update mtime and ctime, only if this is not a directory */
      e_stat = &master->po->np->nn->ln->stat;
      if (!S_ISDIR (e_stat->st_mode))
	fshelp_touch (e_stat, TOUCH_MTIME | TOUCH_CTIME, pcifs_maptime);
    }

  return err;
}
