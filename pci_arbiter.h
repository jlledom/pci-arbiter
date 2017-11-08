/*
   Copyright (C) 2017 Free Software Foundation, Inc.

   Written by Michael I. Bushnell, p/BSG.

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

/* Translator global declarations */

#ifndef PCI_ARBITER_H
#define PCI_ARBITER_H

#include <hurd/netfs.h>
#include <pthread.h>

/* Various parameters that can be used to change the behavior of an ftpfs.  */
struct pcifs_params
{
  /* The size of the node cache.  */
  size_t node_cache_max;
};

/* A particular filesystem.  */
struct pcifs
{
  /* Root of filesystem.  */
  struct node *root;

  struct pcifs_params params;

  /* A cache that holds a reference to recently used nodes.  */
  struct node *node_cache_mru, *node_cache_lru;
  size_t node_cache_len;	/* Number of entries in it.  */
  pthread_mutex_t node_cache_lock;
};

struct pcifs *fs;

#endif /* PCI_ARBITER_H */
