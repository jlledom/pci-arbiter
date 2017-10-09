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

/* Translator initialization and demuxing */

#include <pci_arbiter.h>

#include <error.h>
#include <fcntl.h>
#include <hurd/trivfs.h>

int trivfs_fstype = FSTYPE_MISC;
int trivfs_fsid = 0;
int trivfs_support_read = 0;
int trivfs_support_write = 0;
int trivfs_support_exec = 0;
int trivfs_allow_open = O_READ | O_WRITE;

void
trivfs_modify_stat (struct trivfs_protid *cred, io_statbuf_t * st)
{
}

error_t
trivfs_goaway (struct trivfs_control *fsys, int flags)
{
  exit (0);
}

int
pci_demuxer (mach_msg_header_t *inp,
		mach_msg_header_t *outp)
{
  mig_routine_t routine;
  if ((routine = NULL, trivfs_demuxer (inp, outp)))
  {
    if (routine)
      (*routine) (inp, outp);
    return TRUE;
  }
  else
    return FALSE;
}

int
main (int argc, char **argv)
{
  error_t err;
  struct stat st;
  mach_port_t bootstrap;

  pci_bucket = ports_create_bucket ();

  mach_port_allocate (mach_task_self (),
		      MACH_PORT_RIGHT_RECEIVE, &fsys_identity);

  task_get_bootstrap_port (mach_task_self (), &bootstrap);
  if (bootstrap == MACH_PORT_NULL)
    error (-1, 0, "Must be started as a translator");

  err =
    trivfs_add_protid_port_class (&pci_protid_portclass);
  if (err)
    error (1, 0, "error creating control port class");

  err =
    trivfs_add_control_port_class (&pci_cntl_portclass);
  if (err)
    error (1, 0, "error creating control port class");

  /* Reply to our parent */
  err = trivfs_startup (bootstrap, 0,
			pci_cntl_portclass,
			pci_bucket,
			pci_protid_portclass,
			pci_bucket, &pcicntl);
  mach_port_deallocate (mach_task_self (), bootstrap);
  if (err)
    {
      return (-1);
    }

  /* Initialize status from underlying node.  */
  pci_owner = pci_group = 0;
  err = io_stat (pcicntl->underlying, &st);
  if (!err)
    {
      pci_owner = st.st_uid;
      pci_group = st.st_gid;
    }

  ports_manage_port_operations_multithread (pci_bucket, pci_demuxer,
					    30 * 1000, 2 * 60 * 1000, 0);

  return 0;
}
