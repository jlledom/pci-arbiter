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
#include <sys/io.h>

static error_t
config_block_op (struct pci_device *dev, off_t offset, size_t *len,
		 void *data, pci_io_op_t op)
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
io_config_file (struct pci_device *dev, off_t offset, size_t *len,
		void *data, pci_io_op_t op)
{
  error_t err;

  /* This should never happen */
  assert_backtrace (dev != 0);

  /* Don't exceed the config space size */
  if (offset > FILE_CONFIG_SIZE)
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
read_rom_file (struct pci_device *dev, off_t offset, size_t *len,
	       void *data)
{
  error_t err;

  /* This should never happen */
  assert_backtrace (dev != 0);

  /* Refresh the ROM */
  err = pci_sys->device_refresh (dev, -1, 1);
  if (err)
    return err;

  /* Don't exceed the ROM size */
  if (offset > dev->rom_size)
    return EINVAL;
  if ((offset + *len) > dev->rom_size)
    *len = dev->rom_size - offset;

  memcpy (data, dev->rom_memory + offset, *len);

  return 0;
}

static error_t
region_block_ioport_op (uint16_t port, off_t offset, size_t *len,
			void *data, int read)
{
  size_t pending = *len;

  while (pending >= 4)
    {
      /* read == true: read; else: write */
      if (read)
	*((unsigned int *) data) = inl (port + offset);
      else
	outl (*((unsigned int *) data), port + offset);

      offset += 4;
      data += 4;
      pending -= 4;
    }

  if (pending >= 2)
    {
      if (read)
	*((unsigned short *) data) = inw (port + offset);
      else
	outw (*((unsigned short *) data), port + offset);

      offset += 2;
      data += 2;
      pending -= 2;
    }

  if (pending)
    {
      if (read)
	*((unsigned char *) data) = inb (port + offset);
      else
	outb (*((unsigned char *) data), port + offset);

      offset++;
      data++;
      pending--;
    }

  *len -= pending;

  return 0;
}

error_t
io_region_file (struct pcifs_dirent *e, off_t offset, size_t *len,
		void *data, int read)
{
  error_t err;
  size_t reg_num;
  struct pci_mem_region *region;

  /* This should never happen */
  assert_backtrace (e->device != 0);

  /* Get the region */
  reg_num = strtol (&e->name[strlen (e->name) - 1], 0, 16);
  region = &e->device->regions[reg_num];

  /* Refresh the region */
  err = pci_sys->device_refresh (e->device, reg_num, -1);
  if (err)
    return err;

  /* Don't exceed the ROM size */
  if (offset > region->size)
    return EINVAL;
  if ((offset + *len) > region->size)
    *len = region->size - offset;

  if (region->is_IO)
    region_block_ioport_op (region->base_addr, offset, len, data, read);
  else if (read)
    memcpy (data, region->memory + offset, *len);
  else
    memcpy (region->memory + offset, data, *len);

  return 0;
}
