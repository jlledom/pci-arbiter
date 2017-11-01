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

/* Libnetfs callbacks */

#include <stddef.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

#include <hurd/netfs.h>

/* Attempt to create a file named NAME in DIR for USER with MODE.  Set *NODE
   to the new node upon return.  On any error, clear *NODE.  *NODE should be
   locked on success; no matter what, unlock DIR before returning.  */
error_t
netfs_attempt_create_file (struct iouser * user, struct node * dir,
			   char *name, mode_t mode, struct node ** node)
{
  return EOPNOTSUPP;
}

/* Node NODE is being opened by USER, with FLAGS.  NEWNODE is nonzero if we
   just created this node.  Return an error if we should not permit the open
   to complete because of a permission restriction. */
error_t
netfs_check_open_permissions (struct iouser * user, struct node * node,
			      int flags, int newnode)
{
  error_t err = 0;

  if (!err && (flags & O_READ))
    err = fshelp_access (&node->nn_stat, S_IREAD, user);
  if (!err && (flags & O_WRITE))
    err = fshelp_access (&node->nn_stat, S_IWRITE, user);
  if (!err && (flags & O_EXEC))
    err = fshelp_access (&node->nn_stat, S_IEXEC, user);

  return err;
}

/* This should attempt a utimes call for the user specified by CRED on node
   NODE, to change the atime to ATIME and the mtime to MTIME. */
error_t
netfs_attempt_utimes (struct iouser * cred, struct node * node,
		      struct timespec * atime, struct timespec * mtime)
{
  return EOPNOTSUPP;
}

/* Return the valid access types (bitwise OR of O_READ, O_WRITE, and O_EXEC)
   in *TYPES for file NODE and user CRED.  */
error_t
netfs_report_access (struct iouser * cred, struct node * node, int *types)
{
  return EOPNOTSUPP;
}

/* Trivial definitions.  */

/* Make sure that NP->nn_stat is filled with current information.  CRED
   identifies the user responsible for the operation.  */
error_t
netfs_validate_stat (struct node * node, struct iouser * cred)
{
  return 0;
}

/* This should sync the file NODE completely to disk, for the user CRED.  If
   WAIT is set, return only after sync is completely finished.  */
error_t
netfs_attempt_sync (struct iouser * cred, struct node * node, int wait)
{
  return EOPNOTSUPP;
}

error_t
netfs_get_dirents (struct iouser * cred, struct node * dir,
		   int first_entry, int max_entries, char **data,
		   mach_msg_type_number_t * data_len,
		   vm_size_t max_data_len, int *data_entries)
{
  return EOPNOTSUPP;
}

/* Lookup NAME in DIR for USER; set *NODE to the found name upon return.  If
   the name was not found, then return ENOENT.  On any error, clear *NODE.
   (*NODE, if found, should be locked, this call should unlock DIR no matter
   what.) */
error_t
netfs_attempt_lookup (struct iouser * user, struct node * dir,
		      char *name, struct node ** node)
{
  return 0;
}

/* Delete NAME in DIR for USER. */
error_t
netfs_attempt_unlink (struct iouser * user, struct node * dir, char *name)
{
  return EOPNOTSUPP;
}

/* Note that in this one call, neither of the specific nodes are locked. */
error_t
netfs_attempt_rename (struct iouser * user, struct node * fromdir,
		      char *fromname, struct node * todir,
		      char *toname, int excl)
{
  return EOPNOTSUPP;
}

/* Attempt to create a new directory named NAME in DIR for USER with mode
   MODE.  */
error_t
netfs_attempt_mkdir (struct iouser * user, struct node * dir,
		     char *name, mode_t mode)
{
  return EOPNOTSUPP;
}

/* Attempt to remove directory named NAME in DIR for USER. */
error_t
netfs_attempt_rmdir (struct iouser * user, struct node * dir, char *name)
{
  return EOPNOTSUPP;
}

/* This should attempt a chmod call for the user specified by CRED on node
   NODE, to change the owner to UID and the group to GID. */
error_t
netfs_attempt_chown (struct iouser * cred, struct node * node,
		     uid_t uid, uid_t gid)
{
  return EOPNOTSUPP;
}

/* This should attempt a chauthor call for the user specified by CRED on node
   NODE, to change the author to AUTHOR. */
error_t
netfs_attempt_chauthor (struct iouser * cred, struct node * node,
			uid_t author)
{
  return EOPNOTSUPP;
}

/* This should attempt a chmod call for the user specified by CRED on node
   NODE, to change the mode to MODE.  Unlike the normal Unix and Hurd meaning
   of chmod, this function is also used to attempt to change files into other
   types.  If such a transition is attempted which is impossible, then return
   EOPNOTSUPP.  */
error_t
netfs_attempt_chmod (struct iouser * cred, struct node * node, mode_t mode)
{
  return EOPNOTSUPP;
}

/* Attempt to turn NODE (user CRED) into a symlink with target NAME. */
error_t
netfs_attempt_mksymlink (struct iouser * cred, struct node * node, char *name)
{
  return EOPNOTSUPP;
}

/* Attempt to turn NODE (user CRED) into a device.  TYPE is either S_IFBLK or
   S_IFCHR. */
error_t
netfs_attempt_mkdev (struct iouser * cred, struct node * node,
		     mode_t type, dev_t indexes)
{
  return EOPNOTSUPP;
}

/* Attempt to set the passive translator record for FILE to ARGZ (of length
   ARGZLEN) for user CRED. */
error_t
netfs_set_translator (struct iouser * cred, struct node * node,
		      char *argz, size_t argzlen)
{
  return EOPNOTSUPP;
}

/* This should attempt a chflags call for the user specified by CRED on node
   NODE, to change the flags to FLAGS. */
error_t
netfs_attempt_chflags (struct iouser * cred, struct node * node, int flags)
{
  return EOPNOTSUPP;
}

/* This should attempt to set the size of the file NODE (for user CRED) to
   SIZE bytes long. */
error_t
netfs_attempt_set_size (struct iouser * cred, struct node * node, off_t size)
{
  return EOPNOTSUPP;
}

/* This should attempt to fetch filesystem status information for the remote
   filesystem, for the user CRED. */
error_t
netfs_attempt_statfs (struct iouser * cred, struct node * node,
		      struct statfs * st)
{
  return EOPNOTSUPP;
}

/* This should sync the entire remote filesystem.  If WAIT is set, return
   only after sync is completely finished.  */
error_t
netfs_attempt_syncfs (struct iouser * cred, int wait)
{
  return 0;
}

/* Create a link in DIR with name NAME to FILE for USER.  Note that neither
   DIR nor FILE are locked.  If EXCL is set, do not delete the target, but
   return EEXIST if NAME is already found in DIR.  */
error_t
netfs_attempt_link (struct iouser * user, struct node * dir,
		    struct node * file, char *name, int excl)
{
  return EOPNOTSUPP;
}

/* Attempt to create an anonymous file related to DIR for USER with MODE.
   Set *NODE to the returned file upon success.  No matter what, unlock DIR. */
error_t
netfs_attempt_mkfile (struct iouser * user, struct node * dir,
		      mode_t mode, struct node ** node)
{
  return EOPNOTSUPP;
}

/* Read the contents of NODE (a symlink), for USER, into BUF. */
error_t
netfs_attempt_readlink (struct iouser * user, struct node * node, char *buf)
{
  return EOPNOTSUPP;
}

/* Read from the file NODE for user CRED starting at OFFSET and continuing for
   up to *LEN bytes.  Put the data at DATA.  Set *LEN to the amount
   successfully read upon return.  */
error_t
netfs_attempt_read (struct iouser * cred, struct node * node,
		    off_t offset, size_t * len, void *data)
{
  return EOPNOTSUPP;
}

/* Write to the file NODE for user CRED starting at OFSET and continuing for up
   to *LEN bytes from DATA.  Set *LEN to the amount seccessfully written upon
   return. */
error_t
netfs_attempt_write (struct iouser * cred, struct node * node,
		     off_t offset, size_t * len, void *data)
{
  return EOPNOTSUPP;
}

/* Node NP is all done; free all its associated storage. */
void
netfs_node_norefs (struct node *node)
{
}
