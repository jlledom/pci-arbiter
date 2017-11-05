/*
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
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
#include <strings.h>
#include <sys/io.h>

#include <pci_access.h>
#include <netfs_impl.h>
#include <netfs_util.h>

/* Tree levels */
enum tree_level
{
  LEVEL_NONE,
  LEVEL_BUS,
  LEVEL_DEV,
  LEVEL_FUNC
};

static int
x86_enable_io (void)
{
  if (!ioperm (0, 0xffff, 1))
    return 0;
  return errno;
}

static int
x86_disable_io (void)
{
  if (!ioperm (0, 0xffff, 0))
    return 0;
  return errno;
}

static int
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

static int
pci_system_x86_conf1_read (unsigned bus, unsigned dev, unsigned func,
			   pciaddr_t reg, void *data, unsigned size)
{
  unsigned addr = 0xCFC + (reg & 3);
  unsigned long sav;
  int ret = 0;

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

static int
pci_system_x86_conf1_write (unsigned bus, unsigned dev, unsigned func,
			    pciaddr_t reg, const void *data, unsigned size)
{
  unsigned addr = 0xCFC + (reg & 3);
  unsigned long sav;
  int ret = 0;

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

static int
pci_system_x86_conf2_probe (void)
{
  outb (0, 0xCFB);
  outb (0, 0xCF8);
  outb (0, 0xCFA);
  if (inb (0xCF8) == 0 && inb (0xCFA) == 0)
    return 0;

  return ENODEV;
}

static int
pci_system_x86_conf2_read (unsigned bus, unsigned dev, unsigned func,
			   pciaddr_t reg, void *data, unsigned size)
{
  unsigned addr = 0xC000 | dev << 8 | reg;
  int ret = 0;

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

static int
pci_system_x86_conf2_write (unsigned bus, unsigned dev, unsigned func,
			    pciaddr_t reg, const void *data, unsigned size)
{
  unsigned addr = 0xC000 | dev << 8 | reg;
  int ret = 0;

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

/* Check that this really looks like a PCI configuration. */
static int
pci_system_x86_check (struct pci_iface *pci_ifc)
{
  int dev;
  uint16_t class, vendor;

  /* Look on bus 0 for a device that is a host bridge, a VGA card,
   * or an intel or compaq device.  */

  for (dev = 0; dev < 32; dev++)
    {
      if (pci_ifc->read (0, dev, 0, PCI_CLASS_DEVICE, &class, sizeof (class)))
	continue;
      if (class == PCI_CLASS_BRIDGE_HOST || class == PCI_CLASS_DISPLAY_VGA)
	return 0;
      if (pci_ifc->read (0, dev, 0, PCI_VENDOR_ID, &vendor, sizeof (vendor)))
	continue;
      if (vendor == PCI_VENDOR_ID_INTEL || class == PCI_VENDOR_ID_COMPAQ)
	return 0;
    }

  return ENODEV;
}

static int
pci_probe (struct pci_iface *pci_ifc)
{
  if (pci_system_x86_conf1_probe () == 0)
    {
      pci_ifc->read = pci_system_x86_conf1_read;
      pci_ifc->write = pci_system_x86_conf1_write;
      if (pci_system_x86_check (pci_ifc) == 0)
	return 0;
    }

  if (pci_system_x86_conf2_probe () == 0)
    {
      pci_ifc->read = pci_system_x86_conf2_read;
      pci_ifc->write = pci_system_x86_conf2_write;
      if (pci_system_x86_check (pci_ifc) == 0)
	return 0;
    }

  return ENODEV;
}

static int
pci_nfuncs (int bus, int dev)
{
  uint8_t hdr;
  int err;

  err = pci_ifc->read (bus, dev, 0, PCI_HDRTYPE, &hdr, sizeof (hdr));

  if (err)
    return err;

  return hdr & 0x80 ? 8 : 1;
}

int
pci_system_x86_create (void)
{
  struct pci_dirent *device, *cur_parent;
  int ret, domain, bus, dev, nentries, func, nfuncs;
  enum tree_level level;
  char entry_name[NAME_SIZE];
  uint32_t reg;

  ret = x86_enable_io ();
  if (ret)
    return ret;

  pci_ifc = calloc (1, sizeof (struct pci_iface));
  if (pci_ifc == NULL)
    {
      x86_disable_io ();
      return ENOMEM;
    }

  ret = pci_probe (pci_ifc);
  if (ret)
    {
      x86_disable_io ();
      free (pci_ifc);
      return ret;
    }

  /*
   * First, count how many directory entries will we need.
   *
   * Start from 2, for root and domain entries.
   */
  nentries = 2;
  level = LEVEL_BUS;		/* Start on bus level */
  for (bus = 0; bus < 256; bus++)
    {
      for (dev = 0; dev < 32; dev++)
	{
	  nfuncs = pci_nfuncs (bus, dev);
	  for (func = 0; func < nfuncs; func++)
	    {
	      if (pci_ifc->read (bus, dev, func, PCI_VENDOR_ID, &reg,
				 sizeof (reg)) != 0)
		continue;
	      if (PCI_VENDOR (reg) == PCI_VENDOR_INVALID ||
		  PCI_VENDOR (reg) == 0)
		continue;
	      if (level == LEVEL_BUS)
		level++;
	      if (level == LEVEL_DEV)
		level++;
	      nentries++;
	    }
	  if (level == LEVEL_FUNC)
	    {
	      nentries++;
	      level--;
	    }
	}
      if (level == LEVEL_DEV)
	{
	  nentries++;
	  level--;
	}
    }

  /*
   * At this point, `netfs_root_node->nn->ln' contains the root entry only.
   * Resize it to fit all entries.
   */
  netfs_root_node->nn->ln =
    realloc (netfs_root_node->nn->ln,
	     (nentries) * sizeof (struct pci_dirent));
  if (netfs_root_node->nn->ln == NULL)
    {
      x86_disable_io ();
      free (pci_ifc);
      return ENOMEM;
    }

  /* Add an entry for domain = 0. We still don't support PCI express */
  cur_parent = netfs_root_node->nn->ln;
  device = netfs_root_node->nn->ln + 1;
  domain = 0;
  memset (entry_name, 0, NAME_SIZE);
  snprintf (entry_name, NAME_SIZE, "%04d", domain);
  ret = create_dir_entry (domain, -1, -1, -1, -1, entry_name, cur_parent,
			  cur_parent->stat, device);
  if (ret)
    return ret;

  /* Entering bus level. */
  level = LEVEL_BUS;
  cur_parent = device;
  device++;
  for (bus = 0; bus < 256; bus++)
    {
      for (dev = 0; dev < 32; dev++)
	{
	  nfuncs = pci_nfuncs (bus, dev);
	  for (func = 0; func < nfuncs; func++)
	    {
	      if (pci_ifc->read (bus, dev, func, PCI_VENDOR_ID, &reg,
				 sizeof (reg)) != 0)
		continue;
	      if (PCI_VENDOR (reg) == PCI_VENDOR_INVALID ||
		  PCI_VENDOR (reg) == 0)
		continue;
	      if (pci_ifc->read
		  (bus, dev, func, PCI_CLASS, &reg, sizeof (reg)) != 0)
		continue;

	      /*
	       * This address contains a valid device.
	       *
	       * Add intermediate level entries and switch to lower levels
	       * only when a valid device is found.
	       */
	      if (level == LEVEL_BUS)
		{
		  /* Add entry for bus */
		  memset (entry_name, 0, NAME_SIZE);
		  snprintf (entry_name, NAME_SIZE, "%02d", bus);
		  ret =
		    create_dir_entry (domain, bus, -1, -1, -1, entry_name,
				      cur_parent, cur_parent->stat, device);
		  if (ret)
		    return ret;

		  /* Switch to dev level */
		  cur_parent = device;
		  device++;
		  level++;
		}
	      if (level == LEVEL_DEV)
		{
		  /* Add entry for dev */
		  memset (entry_name, 0, NAME_SIZE);
		  snprintf (entry_name, NAME_SIZE, "%02d", dev);
		  ret =
		    create_dir_entry (domain, bus, dev, -1, -1, entry_name,
				      cur_parent, cur_parent->stat, device);
		  if (ret)
		    return ret;

		  /* Switch to func level */
		  cur_parent = device;
		  device++;
		  level++;
		}

	      /* Func entry */
	      memset (entry_name, 0, NAME_SIZE);
	      snprintf (entry_name, NAME_SIZE, "%02d", func);
	      ret =
		create_dir_entry (domain, bus, dev, func, reg >> 8,
				  entry_name, cur_parent, cur_parent->stat,
				  device);
	      if (ret)
		return ret;

	      device++;
	    }
	  if (level == LEVEL_FUNC)
	    {
	      /* Switch to dev level */
	      cur_parent = cur_parent->parent;
	      level--;
	    }
	}
      if (level == LEVEL_DEV)
	{
	  /* Switch to bus level */
	  cur_parent = cur_parent->parent;
	  level--;
	}
    }

  return 0;
}
