/*
   Copyright (C) 2017 Free Software Foundation, Inc.

   Written by Miles Bader <miles@gnu.org>

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

/* Fsysopts and command line option parsing */

#ifndef OPTIONS_H
#define OPTIONS_H

#include <stdint.h>
#include <sys/types.h>
#include <argp.h>

/* Used to describe a particular set of permissions during argument parsing.  */
struct parse_permset
{
  /*
   * D/b/d/f scope of permissions.
   * 
   * Negative value means no value.
   */
  int32_t domain;
  int16_t bus;
  int16_t dev;
  int8_t func;

  /*
   * Class and subclass scope of permissions
   * 
   * Negative value means no value.
   */
  int16_t d_class;
  int16_t d_subclass;

  /* User and group ids */
  uid_t uid;
  gid_t gid;
};

/* Used to hold data during argument parsing.  */
struct parse_hook
{
  /* A list of specified interfaces and their corresponding options.  */
  struct parse_permset *permsets;
  size_t num_permsets;

  /* Interface to which options apply.  If the device field isn't filled in
     then it should be by the next --interface option.  */
  struct parse_permset *curset;

  /* Node cache length */
  size_t ncache_len;
};

/* Lwip translator options.  Used for both startup and runtime.  */
static const struct argp_option options[] = {
  {0, 0, 0, 0, "Permission scope:", 1},
  {"class", 'C', "CLASS", 0, "Device class in hexadecimal"},
  {"subclass", 's', "SUBCLASS", 0,
   "Device subclass in hexadecimal, only valid with -c"},
  {"domain", 'D', "DOMAIN", 0, "Device domain in hexadecimal"},
  {"bus", 'b', "BUS", 0, "Device bus in hexadecimal, only valid with -D"},
  {"dev", 'd', "DEV", 0, "Device dev in hexadecimal, only valid with -b"},
  {"func", 'f', "FUNC", 0, "Device func in hexadecimal, only valid with -d"},
  {0, 0, 0, 0, "These apply to a given permission scope:", 2},
  {"uid", 'u', "UID", 0, "User ID to give permissions to"},
  {"gid", 'g', "GID", 0, "Group ID to give permissions to"},
  {0, 0, 0, 0, "Global configuration options:", 3},
  {"ncache", 'n', "LENGTH", 0, "Node cache length. 16 by default"},
  {0}
};

static const char doc[] = "More than one permission scope may be specified. \
-D and -C create a new permission scope if the current one already has a value \
for that option.";

#endif // OPTIONS_H