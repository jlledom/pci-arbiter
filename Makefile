#   Copyright (C) 2017 Free Software Foundation, Inc.
#
#   This file is part of the GNU Hurd.
#
#   The GNU Hurd is free software; you can redistribute it and/or
#   modify it under the terms of the GNU General Public License as
#   published by the Free Software Foundation; either version 2, or (at
#   your option) any later version.
#
#   The GNU Hurd is distributed in the hope that it will be useful, but
#   WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#   General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with the GNU Hurd.  If not, see <http://www.gnu.org/licenses/>.

dir		= pci_arbiter
makemode	= server

PORTDIR = $(srcdir)/port

SRCS		= main.c pci_conf-ops.c pci_access.c x86_pci.c
MIGSRCS		= pci_confServer.c
OBJS		= $(patsubst %.S,%.o,$(patsubst %.c,%.o, $(SRCS) $(MIGSRCS)))

HURDLIBS= trivfs fshelp ports shouldbeinlibc

target = pci_arbiter

include ../Makeconf

CFLAGS += -I$(PORTDIR)/include

CPPFLAGS += -imacros $(srcdir)/config.h
pci_conf-MIGSFLAGS = -imacros $(srcdir)/mig-mutate.h

# cpp doesn't automatically make dependencies for -imacros dependencies. argh.
pci_conf_S.h pci_confServer.c: mig-mutate.h
$(OBJS): config.h
