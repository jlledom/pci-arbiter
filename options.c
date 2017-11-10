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

#include <options.h>

#include <stdlib.h>
#include <argp.h>
#include <argz.h>
#include <error.h>

#define NCACHE_DEFAULT_LEN  16

/* Fsysopts and command line option parsing */

/* Adds an empty interface slot to H, and sets H's current interface to it, or
   returns an error. */
static error_t
parse_hook_add_set (struct parse_hook *h)
{
  struct parse_permset *new = realloc (h->permsets,
				       (h->num_permsets +
					1) * sizeof (struct parse_permset));
  if (!new)
    return ENOMEM;

  h->permsets = new;
  h->num_permsets++;
  h->curset = new + h->num_permsets - 1;
  h->curset->domain = -1;
  h->curset->bus = -1;
  h->curset->dev = -1;
  h->curset->func = -1;
  h->curset->d_class = -1;
  h->curset->d_subclass = -1;
  h->curset->uid = 0;
  h->curset->gid = 0;

  return 0;
}

static error_t
check_options_validity (struct parse_hook *h)
{
  int i;
  struct parse_permset *p;

  for (p = h->permsets, i = 0; i < h->num_permsets; i++, p++)
    {
      if ((p->func >= 0 && p->dev < 0)
	  || (p->dev >= 0 && p->bus < 0)
	  || (p->bus >= 0 && p->domain < 0)
	  || (p->d_subclass >= 0 && p->d_class < 0)
	  || ((p->uid || p->gid) && (p->d_class < 0 && p->domain < 0))
	  || ((p->d_class >= 0 || p->domain >= 0) && !(p->uid || p->gid)))
	error (1, EINVAL, "Option dependence error");
    }

  return 0;
}

/* Option parser */
static error_t
parse_opt (int opt, char *arg, struct argp_state *state)
{
  error_t err = 0;
  struct parse_hook *h = state->hook;

  /* Return _ERR from this routine */
#define RETURN(_err)                          \
  do { return _err; } while (0)

  /* Print a parsing error message and (if exiting is turned off) return the
     error code ERR.  */
#define PERR(err, fmt, args...)               \
  do { argp_error (state, fmt , ##args); RETURN (err); } while (0)

  /* Like PERR but for non-parsing errors.  */
#define FAIL(rerr, status, perr, fmt, args...)  \
  do{ argp_failure (state, status, perr, fmt , ##args); RETURN (rerr); } while(0)

  /* Parse STR and return the corresponding  internet address.  If STR is not
     a valid internet address, signal an error mentioned TYPE.  */
#undef	ADDR
#define ADDR(str, type)                         \
  ({ unsigned long addr = inet_addr (str);      \
     if (addr == INADDR_NONE) PERR (EINVAL, "Malformed %s", type);  \
     addr; })

  if (!arg && state->next < state->argc && (*state->argv[state->next] != '-'))
    {
      arg = state->argv[state->next];
      state->next++;
    }

  switch (opt)
    {
    case 'C':
      /* Init a new set if the current one already has a value for this option */
      if (h->curset->d_class >= 0)
	parse_hook_add_set (h);

      h->curset->d_class = strtol (arg, 0, 16);
      break;
    case 's':
      h->curset->d_subclass = strtol (arg, 0, 16);
      break;
    case 'D':
      /* Init a new set if the current one already has a value for this option */
      if (h->curset->domain >= 0)
	parse_hook_add_set (h);

      h->curset->domain = strtol (arg, 0, 16);
      break;
    case 'b':
      h->curset->bus = strtol (arg, 0, 16);
      break;
    case 'd':
      h->curset->dev = strtol (arg, 0, 16);
      break;
    case 'f':
      h->curset->func = strtol (arg, 0, 16);
      break;
    case 'u':
      h->curset->uid = atoi (arg);
      break;
    case 'g':
      h->curset->gid = atoi (arg);
      break;
    case 'n':
      h->ncache_len = atoi (arg);
      break;
    case ARGP_KEY_INIT:
      /* Initialize our parsing state.  */
      h = malloc (sizeof (struct parse_hook));
      if (!h)
	FAIL (ENOMEM, 1, ENOMEM, "option parsing");

      h->permsets = 0;
      h->num_permsets = 0;
      h->ncache_len = NCACHE_DEFAULT_LEN;
      err = parse_hook_add_set (h);
      if (err)
	FAIL (err, 1, err, "option parsing");

      state->hook = h;
      break;

    case ARGP_KEY_SUCCESS:
      /* Check option dependencies */
      err = check_options_validity (h);

      /* Do nothing for now */
      break;

    case ARGP_KEY_ERROR:
      /* Parsing error occurred, free everything. */
      free (h->permsets);
      free (h);
      break;

    default:
      return ARGP_ERR_UNKNOWN;
    }

  return err;
}

struct argp pci_argp = { options, parse_opt, 0, doc };

struct argp *netfs_runtime_argp = &pci_argp;
