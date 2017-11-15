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
 * PCI access backend header.
 *
 * Following code is borrowed from libpciaccess:
 * https://cgit.freedesktop.org/xorg/lib/libpciaccess/
 */

#ifndef X86_PCI_H
#define X86_PCI_H

#define PCI_VENDOR(reg)		((reg) & 0xFFFF)
#define PCI_VENDOR_INVALID	0xFFFF

#define PCI_VENDOR_ID		0x00
#define PCI_VENDOR_ID_COMPAQ		0x0e11
#define PCI_VENDOR_ID_INTEL		0x8086

#define PCI_CLASS		0x08
#define PCI_CLASS_DEVICE	0x0a
#define PCI_CLASS_DISPLAY_VGA		0x0300
#define PCI_CLASS_BRIDGE_HOST		0x0600

#define PCI_XROMBAR_ADDR_00   0x30
#define PCI_XROMBAR_ADDR_01   0x38

#define PCI_HDRTYPE	0x0E
#define PCI_HDRTYPE_DEVICE	0x00
#define PCI_HDRTYPE_BRIDGE	0x01

#define PCI_COMMAND   0x04

int pci_system_x86_create (void);

#endif /* X86_PCI_H */
