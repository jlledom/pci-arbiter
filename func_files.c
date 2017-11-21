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

/* Per-function files implementation */

#include <func_files.h>

#include <assert.h>

static error_t
config_block_op (struct pci_device *dev, off_t offset, size_t * len,
		 void *data, pciop_t op)
{
  error_t err;
  size_t pendent = *len;

  while (pendent >= 4)
    {
      err = op (dev->bus, dev->dev, dev->func, offset, data, 4);
      if (err)
	return err;

      offset += 4;
      data += 4;
      pendent -= 4;
    }

  if (pendent >= 2)
    {
      err = op (dev->bus, dev->dev, dev->func, offset, data, 2);
      if (err)
	return err;

      offset += 2;
      data += 2;
      pendent -= 2;
    }

  if (pendent)
    {
      err = op (dev->bus, dev->dev, dev->func, offset, data, 1);
      if (err)
	return err;

      offset++;
      data++;
      pendent--;
    }

  *len -= pendent;

  return 0;
}

error_t
io_config_file (struct pci_device *dev, off_t offset, size_t * len,
		void *data, pciop_t op)
{
  error_t err;

  /* This should never happen */
  assert_backtrace (dev != 0);

  /* Don't exceed the config space size */
  if(offset > FILE_CONFIG_SIZE)
    return EINVAL;
  if ((offset + *len) > FILE_CONFIG_SIZE)
    *len = FILE_CONFIG_SIZE - offset;

  if (op == pci_sys->read)
    pthread_rwlock_rdlock (&fs->pci_conf_lock);
  else if (op == pci_sys->write)
    pthread_rwlock_wrlock (&fs->pci_conf_lock);
  else
    return EINVAL;

  err = config_block_op (dev, offset, len, data, op);
  pthread_rwlock_unlock (&fs->pci_conf_lock);

  return err;
}

error_t
read_rom_file (struct pci_device * dev, off_t offset, size_t * len,
	       void *data)
{
  /* This should never happen */
  assert_backtrace (dev != 0);

  /* Don't exceed the ROM size */
  if(offset > dev->rom_size)
    return EINVAL;
  if ((offset + *len) > dev->rom_size)
    *len = dev->rom_size - offset;

  memcpy (data, dev->rom_memory + offset, *len);

  return 0;
}
