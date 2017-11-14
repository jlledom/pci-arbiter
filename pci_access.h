/*
 * (C) Copyright IBM Corporation 2006
 * Copyright 2009 Red Hat, Inc.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * on the rights to use, copy, modify, merge, publish, distribute, sub
 * license, and/or sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.  IN NO EVENT SHALL
 * IBM AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */
/*
 * Copyright (c) 2007 Paulo R. Zanoni, Tiago Vignatti
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 */

/*
 * PCI access general header.
 *
 * Following code is borrowed from libpciaccess:
 * https://cgit.freedesktop.org/xorg/lib/libpciaccess/
 */

#ifndef PCI_ACCESS_H
#define PCI_ACCESS_H

#include <stddef.h>
#include <stdint.h>

typedef uint64_t pciaddr_t;

typedef int (*pciop_t) (unsigned bus, unsigned dev, unsigned func, pciaddr_t reg, const void *data, unsigned size);

/*
 * PCI device.
 *
 * Contains all of the information about a particular PCI device.
 */
struct pci_device
{
  /*
   * Complete bus identification, including domain, of the device.  On
   * platforms that do not support PCI domains (e.g., 32-bit x86 hardware),
   * the domain will always be zero.
   */
  uint16_t domain;
  uint8_t bus;
  uint8_t dev;
  uint8_t func;

  /*
   * Device's class, subclass, and programming interface packed into a
   * single 32-bit value.  The class is at bits [23:16], subclass is at
   * bits [15:8], and programming interface is at [7:0].
   */
  uint32_t device_class;
};

/* Global PCI data */
struct pci_system
{
  size_t num_devices;
  struct pci_device *devices;

  /* Callbacks */
  pciop_t read;
  pciop_t write;
};

struct pci_system *pci_sys;

int pci_system_init (void);

#endif /* PCI_ACCESS_H */
