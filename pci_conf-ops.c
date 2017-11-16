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
  err = entry_check_perms (master->user, e, flags);
  if (err)
    return err;

  /* Check wheter the request has been sent to the proper node */
  if (e->domain != 0		/* Only domain 0 can be accessed by I/O ports */
      || e->bus < 0 || e->dev < 0 || e->func < 0)
    err = EINVAL;
  else if (bus != e->bus)
    err = EPERM;
  else if (dev != e->dev)
    err = EPERM;
  else if (func != e->func)
    err = EPERM;

  return err;
}

static size_t
calculate_ndevs (struct iouser *user)
{
  size_t ndevs = 0;
  int i;
  struct pcifs_dirent *e;

  for (i = 0, e = fs->entries; i < fs->num_entries; i++, e++)
    {
      if (e->func < 0		/* Skip entries without a full address  */
	  || !S_ISDIR (e->stat.st_mode))	/* and entries that are not folders     */
	continue;

      if (!entry_check_perms (user, e, O_READ))
	/* If no error, user may access this device */
	ndevs++;
    }

  return ndevs;
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
  pthread_rwlock_t *lock;
  struct pcifs_dirent *e;

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
  pthread_rwlock_rdlock (lock);
  err = pci_sys->read (bus, dev, func, reg, *data, amount);
  pthread_rwlock_unlock (lock);

  if (!err)
    {
      *datalen = amount;
      /* Update atime, only if this is not a directory */
      e = master->po->np->nn->ln;
      if (!S_ISDIR (e->stat.st_mode))
	UPDATE_TIMES (e, TOUCH_ATIME);
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
  pthread_rwlock_t *lock;
  struct pcifs_dirent *e;

  if (!master)
    return EOPNOTSUPP;

  lock = &fs->pci_conf_lock;

  err = check_permissions (master, bus, dev, func, O_WRITE);
  if (err)
    return err;

  pthread_rwlock_wrlock (lock);
  err = pci_sys->write (bus, dev, func, reg, data, datalen);
  pthread_rwlock_unlock (lock);

  if (!err)
    {
      *amount = datalen;
      /* Update mtime and ctime, only if this is not a directory */
      e = master->po->np->nn->ln;
      if (!S_ISDIR (e->stat.st_mode))
	UPDATE_TIMES (e, TOUCH_MTIME | TOUCH_CTIME);
    }

  return err;
}

/* Write in `amount' the number of devices allowed for the 'user'. */
error_t
S_pci_conf_get_ndevs (struct protid * master, mach_msg_type_number_t * amount)
{
  /* This RPC may only be addressed to the root node */
  if (master->po->np != fs->root)
    return EINVAL;

  *amount = calculate_ndevs (master->user);

  return 0;
}
