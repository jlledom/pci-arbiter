/*
 * Copyright (c) 2017 Joan Lledó
 * Copyright (c) 2009, 2012 Samuel Thibault
 * Heavily inspired from the freebsd, netbsd, and openbsd backends
 * (C) Copyright Eric Anholt 2006
 * (C) Copyright IBM Corporation 2006
 * Copyright (c) 2008 Juan Romero Pardines
 * Copyright (c) 2008 Mark Kettenis
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/*
 * PCI access backend.
 *
 * Following code is borrowed from libpciaccess:
 * https://cgit.freedesktop.org/xorg/lib/libpciaccess/
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <x86_pci.h>

#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/io.h>

#include <pci_access.h>

static error_t
x86_enable_io (void)
{
  if (!ioperm (0, 0xffff, 1))
    return 0;
  return errno;
}

static error_t
x86_disable_io (void)
{
  if (!ioperm (0, 0xffff, 0))
    return 0;
  return errno;
}

static error_t
pci_system_x86_conf1_probe (void)
{
  unsigned long sav;
  int res = ENODEV;

  outb (0x01, 0xCFB);
  sav = inl (0xCF8);
  outl (0x80000000, 0xCF8);
  if (inl (0xCF8) == 0x80000000)
    res = 0;
  outl (sav, 0xCF8);

  return res;
}

static error_t
pci_system_x86_conf1_read (unsigned bus, unsigned dev, unsigned func,
			   pciaddr_t reg, void *data, unsigned size)
{
  unsigned addr = 0xCFC + (reg & 3);
  unsigned long sav;
  error_t ret = 0;

  if (bus >= 0x100 || dev >= 32 || func >= 8 || reg >= 0x100 || size > 4
      || size == 3)
    return EIO;

  sav = inl (0xCF8);
  outl (0x80000000 | (bus << 16) | (dev << 11) | (func << 8) | (reg & ~3),
	0xCF8);
  /* NOTE: x86 is already LE */
  switch (size)
    {
    case 1:
      {
	uint8_t *val = data;
	*val = inb (addr);
	break;
      }
    case 2:
      {
	uint16_t *val = data;
	*val = inw (addr);
	break;
      }
    case 4:
      {
	uint32_t *val = data;
	*val = inl (addr);
	break;
      }
    }
  outl (sav, 0xCF8);

  return ret;
}

static error_t
pci_system_x86_conf1_write (unsigned bus, unsigned dev, unsigned func,
			    pciaddr_t reg, void *data, unsigned size)
{
  unsigned addr = 0xCFC + (reg & 3);
  unsigned long sav;
  error_t ret = 0;

  if (bus >= 0x100 || dev >= 32 || func >= 8 || reg >= 0x100 || size > 4
      || size == 3)
    return EIO;

  sav = inl (0xCF8);
  outl (0x80000000 | (bus << 16) | (dev << 11) | (func << 8) | (reg & ~3),
	0xCF8);
  /* NOTE: x86 is already LE */
  switch (size)
    {
    case 1:
      {
	const uint8_t *val = data;
	outb (*val, addr);
	break;
      }
    case 2:
      {
	const uint16_t *val = data;
	outw (*val, addr);
	break;
      }
    case 4:
      {
	const uint32_t *val = data;
	outl (*val, addr);
	break;
      }
    }
  outl (sav, 0xCF8);

  return ret;
}

static error_t
pci_system_x86_conf2_probe (void)
{
  outb (0, 0xCFB);
  outb (0, 0xCF8);
  outb (0, 0xCFA);
  if (inb (0xCF8) == 0 && inb (0xCFA) == 0)
    return 0;

  return ENODEV;
}

static error_t
pci_system_x86_conf2_read (unsigned bus, unsigned dev, unsigned func,
			   pciaddr_t reg, void *data, unsigned size)
{
  unsigned addr = 0xC000 | dev << 8 | reg;
  error_t ret = 0;

  if (bus >= 0x100 || dev >= 16 || func >= 8 || reg >= 0x100)
    return EIO;

  outb ((func << 1) | 0xF0, 0xCF8);
  outb (bus, 0xCFA);
  /* NOTE: x86 is already LE */
  switch (size)
    {
    case 1:
      {
	uint8_t *val = data;
	*val = inb (addr);
	break;
      }
    case 2:
      {
	uint16_t *val = data;
	*val = inw (addr);
	break;
      }
    case 4:
      {
	uint32_t *val = data;
	*val = inl (addr);
	break;
      }
    default:
      ret = EIO;
      break;
    }
  outb (0, 0xCF8);

  return ret;
}

static error_t
pci_system_x86_conf2_write (unsigned bus, unsigned dev, unsigned func,
			    pciaddr_t reg, void *data, unsigned size)
{
  unsigned addr = 0xC000 | dev << 8 | reg;
  error_t ret = 0;

  if (bus >= 0x100 || dev >= 16 || func >= 8 || reg >= 0x100)
    return EIO;

  outb ((func << 1) | 0xF0, 0xCF8);
  outb (bus, 0xCFA);
  /* NOTE: x86 is already LE */
  switch (size)
    {
    case 1:
      {
	const uint8_t *val = data;
	outb (*val, addr);
	break;
      }
    case 2:
      {
	const uint16_t *val = data;
	outw (*val, addr);
	break;
      }
    case 4:
      {
	const uint32_t *val = data;
	outl (*val, addr);
	break;
      }
    default:
      ret = EIO;
      break;
    }
  outb (0, 0xCF8);

  return ret;
}

/* Returns the number of regions (base address registers) the device has */
static int
pci_device_hurd_get_num_regions (uint8_t header_type)
{
  switch (header_type & 0x7f)
    {
    case 0:
      return 6;
    case 1:
      return 2;
    case 2:
      return 1;
    default:
      return 0;
    }
}

/* Masks out the flag bigs of the base address register value */
static uint32_t
get_map_base (uint32_t val)
{
  if (val & 0x01)
    return val & ~0x03;
  else
    return val & ~0x0f;
}

/* Returns the size of a region based on the all-ones test value */
static unsigned
get_test_val_size (uint32_t testval)
{
  unsigned size = 1;

  if (testval == 0)
    return 0;

  /* Mask out the flag bits */
  testval = get_map_base (testval);
  if (!testval)
    return 0;

  while ((testval & 1) == 0)
    {
      size <<= 1;
      testval >>= 1;
    }

  return size;
}

static error_t
pci_device_x86_regions_probe (struct pci_device *dev)
{
  error_t err;
  uint8_t hdrtype;
  uint32_t reg;
  int i, bar, memfd;

  err = pci_sys->read (dev->bus, dev->dev, dev->func, PCI_HDRTYPE, &hdrtype,
		       sizeof (hdrtype));
  if (err)
    return err;

  bar = 0x10;
  for (i = 0; i < pci_device_hurd_get_num_regions (hdrtype); i++, bar += 4)
    {
      uint32_t addr, testval;

      /* Get the base address */
      err =
	pci_sys->read (dev->bus, dev->dev, dev->func, bar, &addr,
		       sizeof (addr));
      if (err)
	continue;

      /* Test write all ones to the register, then restore it. */
      reg = 0xffffffff;
      err = pci_sys->write (dev->bus, dev->dev, dev->func, bar, &reg,
			    sizeof (reg));
      if (err)
	continue;
      err = pci_sys->read (dev->bus, dev->dev, dev->func, bar, &testval,
			   sizeof (testval));
      if (err)
	continue;
      err = pci_sys->write (dev->bus, dev->dev, dev->func, bar, &addr,
			    sizeof (addr));
      if (err)
	continue;

      if (addr & 0x01)
	dev->regions[i].is_IO = 1;
      if (addr & 0x04)
	dev->regions[i].is_64 = 1;
      if (addr & 0x08)
	dev->regions[i].is_prefetchable = 1;

      /* Set the size */
      dev->regions[i].size = get_test_val_size (testval);

      /* Set the base address value */
      dev->regions[i].base_addr = get_map_base (addr);

      if (dev->regions[i].is_IO)
	{
	  /* Enable the I/O Space bit */
	  err =
	    pci_sys->read (dev->bus, dev->dev, dev->func, PCI_COMMAND, &reg,
			   sizeof (reg));
	  if (err)
	    return err;

	  if (!(reg & 0x1))
	    {
	      reg |= 0x1;

	      err =
		pci_sys->write (dev->bus, dev->dev, dev->func, PCI_COMMAND,
				&reg, sizeof (reg));
	      if (err)
		return err;
	    }
	}
      else
	{
	  /* Enable the Memory Space bit */
	  err =
	    pci_sys->read (dev->bus, dev->dev, dev->func, PCI_COMMAND, &reg,
			   sizeof (reg));
	  if (err)
	    return err;

	  if (!(reg & 0x2))
	    {
	      reg |= 0x2;

	      err =
		pci_sys->write (dev->bus, dev->dev, dev->func, PCI_COMMAND,
				&reg, sizeof (reg));
	      if (err)
		return err;
	    }

	  /* Map the region in our space */
	  memfd = open ("/dev/mem", O_RDONLY | O_CLOEXEC);
	  if (memfd == -1)
	    return errno;

	  dev->regions[i].memory =
	    mmap (NULL, dev->regions[i].size, PROT_READ | PROT_WRITE, 0,
		  memfd, dev->regions[i].base_addr);
	  if (dev->regions[i].memory == MAP_FAILED)
	    {
	      dev->regions[i].memory = 0;
	      close (memfd);
	      continue;
	    }

	  close (memfd);

	}

      if (dev->regions[i].is_64)
	{
	  /*
	   * This is a 32bit arch. Only a valid 32bit address may be written here
	   * and the reamining 4 bytes of the address can only be 0. So there's no
	   * need to read them.
	   */
	  bar += 4;
	  i++;
	}

    }

  return 0;
}

static error_t
pci_device_x86_rom_probe (struct pci_device *dev)
{
  error_t err;
  uint8_t reg_8, xrombar_addr;
  uint32_t reg, reg_back;
  pciaddr_t rom_size;
  pciaddr_t rom_addr;
  void *rom_mapped;
  int memfd;

  /*
   * If it's a VGA device, use the 0xc0000 mapping.
   */
  if ((dev->device_class & 0x00ffff00) == (PCI_CLASS_DISPLAY_VGA << 8))
    {
      rom_size = 64 * 1024;
      rom_addr = 0xc0000;
    }
  else
    {

      /* Else, read the XROMBAR */
      /* First we need to know which type of header is this */
      err = pci_sys->read (dev->bus, dev->dev, dev->func, PCI_HDRTYPE, &reg_8,
			   sizeof (reg_8));
      if (err)
	return err;

      /* Get the XROMBAR register address */
      switch (reg_8 & 0x3)
	{
	case PCI_HDRTYPE_DEVICE:
	  xrombar_addr = PCI_XROMBAR_ADDR_00;
	  break;
	case PCI_HDRTYPE_BRIDGE:
	  xrombar_addr = PCI_XROMBAR_ADDR_01;
	  break;
	default:
	  return -1;
	}

      /* Get size and physical address */
      err = pci_sys->read (dev->bus, dev->dev, dev->func, xrombar_addr, &reg,
			   sizeof (reg));
      if (err)
	return err;

      reg_back = reg;
      reg = 0xFFFFF800;		/* Base address: first 21 bytes */
      err = pci_sys->write (dev->bus, dev->dev, dev->func, xrombar_addr, &reg,
			    sizeof (reg));
      if (err)
	return err;
      err = pci_sys->read (dev->bus, dev->dev, dev->func, xrombar_addr, &reg,
			   sizeof (reg));
      if (err)
	return err;

      rom_size = (~reg + 1);
      rom_addr = reg_back & reg;

      if (rom_size == 0)
	return 0;

      /* Enable the address decoder and write the physical address back */
      reg_back |= 0x1;
      err = pci_sys->write
	(dev->bus, dev->dev, dev->func, xrombar_addr, &reg_back,
	 sizeof (reg_back));
      if (err)
	return err;

      /* Enable the Memory Space bit */
      err = pci_sys->read (dev->bus, dev->dev, dev->func, PCI_COMMAND, &reg,
			   sizeof (reg));
      if (err)
	return err;

      if (!(reg & 0x2))
	{
	  reg |= 0x2;

	  err =
	    pci_sys->write (dev->bus, dev->dev, dev->func, PCI_COMMAND, &reg,
			    sizeof (reg));
	  if (err)
	    return err;
	}
    }

  /* Map the ROM in our space */
  memfd = open ("/dev/mem", O_RDONLY | O_CLOEXEC);
  if (memfd == -1)
    return errno;

  rom_mapped = mmap (NULL, rom_size, PROT_READ, 0, memfd, rom_addr);
  if (rom_mapped == MAP_FAILED)
    {
      close (memfd);
      return errno;
    }

  close (memfd);

  dev->rom_size = rom_size;
  dev->rom_addr = rom_addr;
  dev->rom_memory = rom_mapped;

  return 0;
}

/* Check that this really looks like a PCI configuration. */
static error_t
pci_system_x86_check (struct pci_system *pci_sys)
{
  int dev;
  uint16_t class, vendor;

  /* Look on bus 0 for a device that is a host bridge, a VGA card,
   * or an intel or compaq device.  */

  for (dev = 0; dev < 32; dev++)
    {
      if (pci_sys->read (0, dev, 0, PCI_CLASS_DEVICE, &class, sizeof (class)))
	continue;
      if (class == PCI_CLASS_BRIDGE_HOST || class == PCI_CLASS_DISPLAY_VGA)
	return 0;
      if (pci_sys->read (0, dev, 0, PCI_VENDOR_ID, &vendor, sizeof (vendor)))
	continue;
      if (vendor == PCI_VENDOR_ID_INTEL || class == PCI_VENDOR_ID_COMPAQ)
	return 0;
    }

  return ENODEV;
}

static error_t
pci_probe (struct pci_system *pci_sys)
{
  if (pci_system_x86_conf1_probe () == 0)
    {
      pci_sys->read = pci_system_x86_conf1_read;
      pci_sys->write = pci_system_x86_conf1_write;
      if (pci_system_x86_check (pci_sys) == 0)
	return 0;
    }

  if (pci_system_x86_conf2_probe () == 0)
    {
      pci_sys->read = pci_system_x86_conf2_read;
      pci_sys->write = pci_system_x86_conf2_write;
      if (pci_system_x86_check (pci_sys) == 0)
	return 0;
    }

  return ENODEV;
}

static error_t
pci_nfuncs (struct pci_system *pci_sys, int bus, int dev)
{
  uint8_t hdr;
  error_t err;

  err = pci_sys->read (bus, dev, 0, PCI_HDRTYPE, &hdr, sizeof (hdr));

  if (err)
    return err;

  return hdr & 0x80 ? 8 : 1;
}

error_t
pci_system_x86_create (void)
{
  struct pci_device *device;
  error_t err;
  int bus, dev, ndevs, func, nfuncs;
  uint32_t reg;

  err = x86_enable_io ();
  if (err)
    return err;

  pci_sys = calloc (1, sizeof (struct pci_system));
  if (pci_sys == NULL)
    {
      x86_disable_io ();
      return ENOMEM;
    }

  err = pci_probe (pci_sys);
  if (err)
    {
      x86_disable_io ();
      free (pci_sys);
      return err;
    }

  ndevs = 0;
  for (bus = 0; bus < 256; bus++)
    {
      for (dev = 0; dev < 32; dev++)
	{
	  nfuncs = pci_nfuncs (pci_sys, bus, dev);
	  for (func = 0; func < nfuncs; func++)
	    {
	      if (pci_sys->read (bus, dev, func, PCI_VENDOR_ID, &reg,
				 sizeof (reg)) != 0)
		continue;
	      if (PCI_VENDOR (reg) == PCI_VENDOR_INVALID ||
		  PCI_VENDOR (reg) == 0)
		continue;
	      ndevs++;
	    }
	}
    }

  pci_sys->num_devices = ndevs;
  pci_sys->devices = calloc (ndevs, sizeof (struct pci_device));
  if (pci_sys->devices == NULL)
    {
      x86_disable_io ();
      free (pci_sys);
      pci_sys = NULL;
      return ENOMEM;
    }

  device = pci_sys->devices;
  for (bus = 0; bus < 256; bus++)
    {
      for (dev = 0; dev < 32; dev++)
	{
	  nfuncs = pci_nfuncs (pci_sys, bus, dev);
	  for (func = 0; func < nfuncs; func++)
	    {
	      if (pci_sys->read (bus, dev, func, PCI_VENDOR_ID, &reg,
				 sizeof (reg)) != 0)
		continue;
	      if (PCI_VENDOR (reg) == PCI_VENDOR_INVALID ||
		  PCI_VENDOR (reg) == 0)
		continue;
	      device->domain = 0;
	      device->bus = bus;
	      device->dev = dev;
	      device->func = func;

	      if (pci_sys->read
		  (bus, dev, func, PCI_CLASS, &reg, sizeof (reg)) != 0)
		continue;
	      device->device_class = reg >> 8;

	      err = pci_device_x86_rom_probe (device);
	      if (err)
		return err;

	      err = pci_device_x86_regions_probe (device);
	      if (err)
		return err;

	      device++;
	    }
	}
    }

  return 0;
}
