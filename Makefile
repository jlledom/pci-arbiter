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

SRCS		= main.c
OBJS		= $(patsubst %.S,%.o,$(patsubst %.c,%.o, $(SRCS)))

HURDLIBS= trivfs ports

target = pci_arbiter

include ../Makeconf

CFLAGS += -I$(PORTDIR)/include

$(OBJS): config.h
