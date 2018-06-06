/* fileio.c -- Managing files, pipes, and such
   Copyright (c) 1994-1998 by Eberhard Mattes

This file is part of emx.

emx is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

emx is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with emx; see the file COPYING.  If not, write to
the Free Software Foundation, 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.

As special exception, emx.dll can be distributed without source code
unless it has been changed.  If you modify emx.dll, this exception
no longer applies and you must remove this paragraph from all source
files for emx.dll.  */


#define INCL_DOSDEVIOCTL
#define INCL_DOSSEMAPHORES
#define INCL_DOSERRORS
#include <os2emx.h>
#include <emx/syscalls.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <sys/nls.h>
#include <sys/termio.h>
#include <sys/errno.h>
#include "emxdll.h"
#include "files.h"
#include "tcpip.h"
#include "clib.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>

/* Evaluate to true if P points to the path name of a pty.
   xf86sup.sys currently supports 32 ptys, /dev/ptyp0 through
   /dev/ptyqf. */

#define IS_PTY_NAME(p)                          \
    (strncmp ((p), "/dev/pty", 8) == 0          \
     && ((p)[8] == 'p' || (p)[8] == 'q')        \
     && (((p)[9] >= '0' && (p)[9] <= '9')       \
         || ((p)[9] >= 'a' && (p)[9] <= 'f'))   \
     && (p)[10] == 0)

/* The first implementation of umask() was quite broken; to avoid
   breaking old programs using that broken version of umask(), we keep
   (and ignore) a separate umask value for __umask1(). */

ULONG umask_bits1;

/* This is the current umask.  Only the 0200 bit is used. */

ULONG umask_bits;

/* Number of file handles. */

ULONG handle_count;

/* Array of handle flags. */

ULONG *handle_flags;

/* This array maps file handles to file descriptions (index into
   files[]). */

ULONG *handle_file;

/* This array contains file descriptions.  Multiple file handles may
   point to one file descriptions if dup() is used.  Unfortunately, we
   cannot (efficiently, or at all, for devices) find out whether two
   inherited file handles refer to the same file.  Therefore, every
   inherited file handle has its own file description. */

my_file *files;

/* This macro is used for accessing FDATE or FTIME values as
   USHORT. */

#define XUSHORT(x) (*(USHORT *)&(x))

/* We fake inode numbers by assigning a `unique' inode number every
   time an inode number is fetched.  When querying the inode number of
   a file twice, you'll get two different numbers.  This variable
   holds the inode number to be used for the next file.  After using
   the value, it will be incremented.  When wrapping around to 0, skip
   to 1 (0 seems to be a reserved value). */

long ino_number;

/* `handle_flags' is protected by this Mutex semaphore. */

HMTX files_access;


/* Prototypes. */

static ULONG new_handle (ULONG handle);


/* Initialize the data structures related to file handles, and other
   variables. */

void init_fileio (void)
{
  LONG req_count;
  ULONG rc, i;

  create_mutex_sem (&files_access);

  /* Retrieve the maximum number of file handles. */

  req_count = 0;
  rc = DosSetRelMaxFH (&req_count, &handle_count);
  if (rc != 0)
    error (rc, "DosSetRelMaxFH");

  /* Allocate an array which holds a word of flag bits for each
     handle, including inactive handles.  Allocate an array of file
     descriptions, and an array which maps file handles to file
     descriptions. */

  handle_flags = checked_private_alloc (handle_count * sizeof (*handle_flags));
  handle_file = checked_private_alloc (handle_count * sizeof (*handle_file));
  files = checked_private_alloc (handle_count * sizeof (*files));

  /* Initialize the file descriptions. */

  for (i = 0; i < handle_count; ++i)
    {
      files[i].flags = 0;
      files[i].ref_count = 0;
      files[i].tio_buf = NULL;
      SET_INVALID_FILE (i);
    }

  /* Initialize the flag bits and the file descripions for all
     handles.  Unfortunately, we cannot find out (efficiently, or at
     all, for devices) whether two inherited file handles refer to the
     same file.  Therefore, every inherited file handle has its own
     file description. */

  for (i = 0; i < handle_count; ++i)
    new_handle (i);

  /* Initialize other variables. */

  umask_bits = 0022;
  umask_bits1 = 0644;
  ino_number = 0x100000;
}


/* Set HANDLE's OPEN_FLAGS_NOINHERIT iff FD_FLAGS contains FD_CLOEXEC.
   Return zero or the OS/2 error code. */

static ULONG set_no_inherit (ULONG handle, ULONG fd_flags)
{
  ULONG rc, state, new_state;

  rc = DosQueryFHState (handle, &state);
  if (rc != 0) return rc;
  if (fd_flags & FD_CLOEXEC)
    new_state = state | OPEN_FLAGS_NOINHERIT;
  else
    new_state = state & ~OPEN_FLAGS_NOINHERIT;
  if (new_state != state)
    rc = DosSetFHState (handle, new_state & 0x7f88);
  return rc;
}


/* Make all user file handles inheritable and save the original
   FD_CLOEXEC state in the HF_NOINHERIT bit. */

void fileio_fork_parent_init (void)
{
  ULONG i, state;

  for (i = 0; i < handle_count; ++i)
    if ((handle_flags[i] & (HF_OPEN|HF_SOCKET)) == HF_OPEN
        && DosQueryFHState (i, &state) == 0
        && (state & OPEN_FLAGS_NOINHERIT))
      {
        handle_flags[i] |= HF_NOINHERIT;
        set_no_inherit (i, 0);
      }
    else
      handle_flags[i] &= ~HF_NOINHERIT;
}


/* Undo fileio_fork_parent_init(), that is, restore the file handle
   state from the HF_NOINHERIT bit. */

void fileio_fork_parent_restore (void)
{
  ULONG i;

  for (i = 0; i < handle_count; ++i)
    if ((handle_flags[i] & (HF_OPEN|HF_SOCKET|HF_NOINHERIT))
        == (HF_OPEN|HF_NOINHERIT))
      set_no_inherit (i, FD_CLOEXEC);
}


/* Fill-in file-related fields of structure for the child process. */

void fileio_fork_parent_fillin (struct fork_data_done *p)
{
  ULONG i, f;

  if (p->size + handle_count * sizeof (ULONG) <= FORK_OBJ_SIZE)
    p->handle_count = handle_count;
  else
    p->handle_count = (FORK_OBJ_SIZE - p->size) / sizeof (ULONG);
  p->fd_flags_array = (ULONG *)((char *)p + p->size);
  p->size += p->handle_count * sizeof (ULONG);

  for (i = 0; i < p->handle_count; ++i)
    {
      f = 0;
      if (IS_SOCKET (i))
        {
          if (IS_VALID_FILE (i))
            f = (ULONG)GET_FILE (i)->x.socket.fd_flags;
        }
      else if ((handle_flags[i] & (HF_OPEN|HF_NOINHERIT))
               == (HF_OPEN|HF_NOINHERIT))
        f = FD_CLOEXEC;
      p->fd_flags_array[i] = f;
    }
}


/* Initialize file handles in a forked process. */

void fileio_fork_child (struct fork_data_done *p)
{
  ULONG i, n;

  n = (p->handle_count < handle_count ? p->handle_count : handle_count);
  for (i = 0; i < n; ++i)
    if ((p->fd_flags_array[i] & FD_CLOEXEC) && IS_VALID_FILE (i))
      {
        if (IS_SOCKET (i))
          GET_FILE (i)->x.socket.fd_flags = (int)p->fd_flags_array[i];
        else
          set_no_inherit (i, FD_CLOEXEC);
      }
}


/* Call this function instead of DosQueryPathInfo, to be able to work
   around OS/2 bugs.

   If DEBUG_QPINFO2 is set (-!64), DosQueryPathInfo will be called
   twice: Sometimes, DosQueryPathInfo reports directories on CDROMs as
   files.  Calling DosQueryPathInfo seems to help.  We don't call
   DosQueryPathInfo twice if ulInfoLevel is not FIL_STANDARD, as
   pInfoBuffer is an input/output parameter if ulInfoLevel is
   FIL_QUERYEASFROMLIST.  (Currently, ulInfoLevel is always
   FIL_STANDARD.) */

ULONG query_path_info (PCSZ pszPathName, ULONG ulInfoLevel,
                       PVOID pInfoBuffer, ULONG ulInfoLength)
{
  ULONG rc;

  rc = DosQueryPathInfo (pszPathName, ulInfoLevel, pInfoBuffer, ulInfoLength);
  if (ulInfoLevel == FIL_STANDARD && (debug_flags & DEBUG_QPINFO2))
    rc = DosQueryPathInfo (pszPathName, ulInfoLevel, pInfoBuffer,
                           ulInfoLength);
  return rc;
}


/* Compare the first element of the path PATH to DIR.  DIR must not
   contain upper case letters. */

static int dir_check (const char *path, const char *dir)
{
  while (*path != 0 && *dir != 0 && tolower (*path) == *dir)
    ++path, ++dir;
  return *dir == 0 && (*path == '/' || *path == '\\');
}


/* Avoid OS/2 internal processing error: If the path name S starts
   with \pipe\ or /pipe/, return true.  */

int check_npipe (const char *s)
{
  return (*s == '/' || *s == '\\') && dir_check (s + 1, "pipe");
}


/* Truncate the path name SRC, placing the result into the buffer
   pointed to by DST.  At most 512 characters (including the
   terminating null character) are copied to DST. */

void truncate_name (char *dst, const char *src)
{
  int dst_count;
  ULONG trunc_bit;

  dst_count = 512 - 1;          /* Buffer size sans null character */

  /* Prepend drive letter if the -r option is used, the path name
     starts with a slash but is not a UNC path, and the first path
     element is not `dev' or `pipe'.  */

  trunc_bit = TRUNC_ALL;
  if (src[0] == '/' || src[0] == '\\')
    {
      if (src[1] == '/' || src[1] == '\\')
        {
          /* It's a UNC path: don't prepend drive letter; check
             `-t/'. */
          trunc_bit = TRUNC_UNC;
        }
      else if (dir_check (src + 1, "dev") || dir_check (src + 1, "pipe"))
        {
          /* It's a device or pipe name.  Do not prepend drive letter;
             do not truncate. */
          trunc_bit = 0;
        }
      else if (opt_drive != 0)
        {
          /* Prepend drive letter; check `-t' for that drive
             letter. */
          *dst++ = opt_drive;
          *dst++ = ':';
          dst_count -= 2;
          trunc_bit = TRUNC_DRV (opt_drive);
        }
    }
  else if (((src[0] >= 'A' && src[0] <= 'Z')
            || (src[0] >= 'a' && src[0] <= 'z'))
           && src[1] == ':')
    trunc_bit = TRUNC_DRV (src[0]);

  if (trunc_bit == TRUNC_ALL && opt_trunc != 0 && opt_trunc != TRUNC_ALL)
    {
      ULONG drv, map;

      DosQueryCurrentDisk (&drv, &map);
      trunc_bit = TRUNC_DRV (drv);
    }

  if (opt_trunc & trunc_bit)
    {
      unsigned char rest, dot;

      /* The -t option applies; truncate all elements of the pathname
         to 8.3 format. */

      rest = 8; dot = FALSE;
      while (*src != 0 && dst_count != 0)
        {
          if (_nls_is_dbcs_lead ((unsigned char)*src) && src[1] != 0)
            {
              /* We encountered a valid DBCS character (note that a
                 DBCS lead byte followed by zero is invalid and will
                 be handled as an ordinary character).  Avoid
                 truncating between the two bytes. */

              if (rest >= 2 && dst_count >= 2)
                {
                  *dst++ = *src++;
                  *dst++ = *src;
                  dst_count -= 2; rest -= 2;
                }
              else
                ++src;
            }
          else if (*src == ':' || *src == '/' || *src == '\\')
            {
              *dst++ = *src;
              --dst_count;
              rest = 8; dot = FALSE;
            }
          else if (*src == '.')
            {
              if (rest == 8)
                {
                  /* An element starts with a dot -- process it like
                     an ordinary character.  This makes ".." work.
                     ".emacs", for instance, is left alone. */

                  for (;;)
                    {
                      *dst++ = *src;
                      --dst_count; --rest;
                      if (src[1] != '.' || dst_count == 0)
                        break;
                      ++src;
                    }
                }
              else if (dot)
                {
                  /* If the element has two or more dots, truncate at
                     the second dot.  This does not apply to ".."
                     which is handled above. */

                  rest = 0;
                }
              else
                {
                  /* Start of a suffix. */

                  rest = 3; dot = TRUE;
                  *dst++ = *src;
                  --dst_count;
                }
            }
          else if (rest != 0)
            {
              /* Copy an ordinary character unless the truncation
                 limit (8 or 3 characters) has been reached. */

              *dst++ = *src;
              --dst_count; --rest;
            }
          ++src;
        }
    }
  else
    {
      /* The -t option is not used, don't truncate the file name to
         8.3 format (however, truncate it to avoid overflowing the
         destination buffer. */

      while (*src != 0 && dst_count != 0)
        {
          *dst++ = *src++;
          --dst_count;
        }
    }

  /* Add the terminating null character. */

  *dst = 0;
}


/* Update the flag bits of file handle HANDLE, the new value is FLAGS.
   Don't crash if HANDLE is out of range. */

void set_handle_flags (ULONG handle, ULONG flags)
{
  if (handle < handle_count)
    {
      handle_flags[handle] = flags;
      if (IS_VALID_FILE (handle))
        GET_FILE (handle)->flags = flags;
    }
}


/* Allocate a file description for HANDLE. */

void alloc_file_description (ULONG handle)
{
  ULONG i;

  LOCK_FILES;

  /* File 0 is reserved for the CON device. */

  for (i = 1; i < handle_count; ++i)
    if (files[i].ref_count == 0)
      {
        handle_file[handle] = i;
        files[i].ref_count = 1;
        UNLOCK_FILES;
        return;
      }
  SET_INVALID_FILE (handle);
  UNLOCK_FILES;
}


/* Relocate the OS/2 file handle HANDLE if there is already a socket
   with that handle; then, if CALL_NEW_HANDLE is true, apply
   new_handle().  Update the object pointed to by TARGET with the new
   handle.  Return errno. */

static int reloc_handle (HFILE handle, HFILE *target, int call_new_handle)
{
  ULONG i, rc, htype, hflags;

  *target = handle;
  if (!IS_SOCKET (handle))
    return (call_new_handle ? new_handle (handle) : 0);

  LOCK_FILES;
  for (i = 0; i < handle_count; ++i)
    if (!(handle_flags[i] & HF_OPEN)
        && DosQueryHType (i, &htype, &hflags) == ERROR_INVALID_HANDLE)
      break;
  if (i >= handle_count)
    {
      UNLOCK_FILES;
      return EMFILE;
    }
  *target = i;
  xf86sup_maybe_enadup (handle, TRUE);
  rc = DosDupHandle (handle, &i);
  xf86sup_maybe_enadup (handle, FALSE);
  if (rc != 0)
    {
      UNLOCK_FILES;
      return set_error (rc);
    }
  if (i != *target)             /* Can this happen? */
    {
      UNLOCK_FILES;
      DosClose (i);
      return EMFILE;
    }
  rc = (call_new_handle ? new_handle (i) : 0);
  UNLOCK_FILES;
  if (rc != 0)
    {
      DosClose (i);
      return rc;
    }
  DosClose (handle);
  return 0;
}


/* Update our tables when closing a file handle. */

void close_handle (ULONG handle)
{
  if (handle >= handle_count) return;
  if (IS_VALID_FILE (handle))
    {
      my_file *d;

      LOCK_FILES;
      d = GET_FILE (handle);
      SET_INVALID_FILE (handle);
      if (d->ref_count != 0)
        {
          --d->ref_count;
          if (d->ref_count == 0)
            {
              /* File description no longer referenced. */
              if (d->tio_buf != NULL)
                {
                  private_free (d->tio_buf);
                  d->tio_buf = NULL;
                }
            }
        }
      UNLOCK_FILES;
    }
  handle_flags[handle] = 0;
}


/* Find out the type of the new file handle HANDLE, update the flag
   bits in `handle_flags', and add a file description.  Return the
   OS/2 error code. */

static ULONG new_handle (ULONG handle)
{
  ULONG rc, htype, hflags, pstate, flags;

  if (handle >= handle_count) return ERROR_INVALID_HANDLE;
  close_handle (handle);
  rc = DosQueryHType (handle, &htype, &hflags);
  if (rc != 0)
    return rc;

  switch (htype & 0xff)       /* Class */
    {
    case HANDTYPE_DEVICE:
      if (hflags & 3)         /* KBD or VIO */
        {
          /* All handles for the CON device (KBD and VIO) share the
             same file description: file table entry 0.  Do not use
             alloc_file_description(). */

          ++files[0].ref_count;
          handle_file[handle] = 0;
          flags = HF_OPEN | HF_DEV | HF_CON;
          init_termio (handle);
        }
      else
        {
          alloc_file_description (handle);
          if (hflags & 4)
            flags = HF_OPEN | HF_DEV | HF_NUL;
          else if (hflags & 8)
            flags = HF_OPEN | HF_DEV | HF_CLK;
          else if (!IS_VALID_FILE (handle))
            flags = HF_OPEN | HF_DEV;
          else if (!(debug_flags & DEBUG_NOXF86SUP)
                   && xf86sup_query (handle, &flags) == 0)
            flags |= HF_OPEN | HF_DEV;
          else if (query_async (handle) == 0
                   && translate_async (handle) == 0)
            flags = HF_OPEN | HF_DEV | HF_ASYNC;
          else
            flags = HF_OPEN | HF_DEV;
        }
      break;

    case HANDTYPE_PIPE:
      alloc_file_description (handle);
      rc = DosQueryNPHState (handle, &pstate);
      if (rc == ERROR_PIPE_NOT_CONNECTED)
        flags = HF_OPEN | HF_NPIPE;
      else if (rc != 0)       /* Unnamed pipe */
        flags = HF_OPEN | HF_UPIPE;
      else if (pstate & NP_NOWAIT)
        flags = HF_OPEN | HF_NPIPE | HF_NDELAY;
      else
        flags = HF_OPEN | HF_NPIPE;
      break;

    case HANDTYPE_FILE:
      alloc_file_description (handle);
      flags = HF_OPEN | HF_FILE;
      break;

    default:
      /* Future extension. */
      alloc_file_description (handle);
      flags = HF_OPEN;
      break;
    }
  set_handle_flags (handle, flags);
  return 0;
}


/* Return the number of bytes available for reading from pipe HANDLE.
   Return -1 and set *ERRNOP on error.  Set *ERRNOP to zero on
   success. */

static int npipe_avail (ULONG handle, int *errnop)
{
  ULONG rc, state, nread;
  AVAILDATA avail;
  char buffer;

  rc = DosPeekNPipe (handle, &buffer, 0, &nread, &avail, &state);
  if (rc != 0)
    {
      *errnop = set_error (rc);
      return -1;
    }
  *errnop = 0;
  return avail.cbpipe;
}


/* Find the COUNT lowest numbered, unused file handles and store them
   to the array pointed to by DST, in increasing order.  Return the
   number of unused file handles found. */

int find_unused_handles (ULONG *dst, int count)
{
  ULONG i, htype, hflags;
  int n;

  n = 0;
  for (i = 0; i < handle_count && n < count; ++i)
    if (DosQueryHType (i, &htype, &hflags) == ERROR_INVALID_HANDLE)
      dst[n++] = i;
  return n;
}


/* This function implements the __open() system call of emx 0.8e and
   later.  Return the file handle, or -1 on error. */

int sys_open (const char *path, unsigned flags, unsigned long size,
              int *errnop)
{
  char fname[512];
  ULONG rc;
  ULONG open_mode, attr, action, open_flag;
  HFILE handle, nh;
  int fail_errno, e;
  char is_pty;

  /* Interpret "/dev/null" and "/dev/tty" as the name of the null
     device "NUL" or of the console "CON", respectively.  Copy the
     path name to `fname', with modifications according to the -r and
     -t options. */

  is_pty = FALSE;
  if (strcmp (path, "/dev/null") == 0)
    strcpy (fname, "nul");
  else if (strcmp (path, "/dev/tty") == 0)
    strcpy (fname, "con");
  else if (!(debug_flags & DEBUG_NOXF86SUP) && IS_PTY_NAME (path))
    {
      is_pty = TRUE;
      strcpy (fname, path);
    }
  else
    truncate_name (fname, path);

  /* Extract the access mode and sharing mode bits. */

  open_mode = flags & 0x77;

  /* Handle O_NOINHERIT and O_SYNC. */

  if (flags & _SO_NOINHERIT)
    open_mode |= OPEN_FLAGS_NOINHERIT;
  if (flags & _SO_SYNC)
    open_mode |= OPEN_FLAGS_WRITE_THROUGH;

  /* Set OPEN_FLAGS_FAIL_ON_ERROR for /dev/ptyXX as otherwise opening
     such a device would generate an error popup if the pty is already
     in use.  With OPEN_FLAGS_FAIL_ON_ERROR, we'll get return code
     ERROR_NOT_READY, which will be translated to EIO. */

  if (is_pty)
    open_mode |= OPEN_FLAGS_FAIL_ON_ERROR;

  /* Extract the file attribute bits. */

  attr = (flags >> 8) & 0xff;
  if (umask_bits & 0200)
    attr |= 1;

  /* Translate ERROR_OPEN_FAILED to ENOENT unless O_EXCL is set (see
     below). */

  fail_errno = ENOENT;

  /* Compute `open_flag' depending on `flags'.  Note that _SO_CREAT is
     set for O_CREAT. */

  if (flags & _SO_CREAT)
    {
      if (flags & _SO_EXCL)
        {
          open_flag = OPEN_ACTION_FAIL_IF_EXISTS | OPEN_ACTION_CREATE_IF_NEW;
          fail_errno = EEXIST;
        }
      else if (flags & _SO_TRUNC)
        open_flag = OPEN_ACTION_REPLACE_IF_EXISTS | OPEN_ACTION_CREATE_IF_NEW;
      else
        open_flag = OPEN_ACTION_OPEN_IF_EXISTS | OPEN_ACTION_CREATE_IF_NEW;
    }
  else if (flags & _SO_TRUNC)
    open_flag = OPEN_ACTION_REPLACE_IF_EXISTS | OPEN_ACTION_FAIL_IF_NEW;
  else
    open_flag = OPEN_ACTION_OPEN_IF_EXISTS | OPEN_ACTION_FAIL_IF_NEW;

  /* Ignore SIZE unless _SO_SIZE is set. */

  if (!(flags & _SO_SIZE))
    size = 0;

  /* Try to open the file and handle errors. */

  rc = DosOpen (fname, &handle, &action, size, attr, open_flag, open_mode,
                NULL);
  if (rc == ERROR_OPEN_FAILED)
    {
      set_error (rc);
      *errnop = fail_errno;
      return -1;
    }
  if (rc != 0)
    {
      *errnop = set_error (rc);
      return -1;
    }

  /* Relocate the handle and update the flag bits and the file
     description for the new file handle. */

  e = reloc_handle (handle, &nh, TRUE);
  if (e != 0)
    {
      DosClose (handle);
      *errnop = e;
      return -1;
    }

  /* Return the handle and tell the caller not to modify `errno'. */

  *errnop = 0;
  return nh;
}


/* This function emulates DOS function 0x3c.  That function may still
   be used by ancient programs. */

int old_creat (const char *path, ULONG attr, int *errnop)
{
  char fname[512];
  ULONG rc, action;
  HFILE hf, nh;
  int e;

  /* Copy the path name to `fname', with modifications according to
     the -r and -t options. */

  truncate_name (fname, path);

  /* Extract the file attribute bits. */

  attr &= 0x27;
  if (umask_bits & S_IWRITE)
    attr |= 1;                  /* Set read-only attribute */

  /* Try to open the file and handle errors. */

  rc = DosOpen (fname, &hf, &action, 0, attr,
                OPEN_ACTION_CREATE_IF_NEW | OPEN_ACTION_REPLACE_IF_EXISTS,
                OPEN_ACCESS_READWRITE | OPEN_SHARE_DENYREADWRITE,
                NULL);
  if (rc != 0)
    {
      *errnop = set_error (rc);
      return -1;
    }

  /* Relocate the handle and update the flag bits and the file
     description for the new file handle. */

  e = reloc_handle (hf, &nh, TRUE);
  if (e != 0)
    {
      DosClose (hf);
      *errnop = e;
      return -1;
    }

  /* Return the handle and tell the caller not to modify `errno'. */

  *errnop = 0;
  return nh;
}


/* This function emulates DOS function 0x3d.  That function may still
   be used by ancient programs. */

int old_open (const char *path, ULONG mode, int *errnop)
{
  char fname[512];
  ULONG rc, action, new_mode;
  HFILE hf, nh;
  int e;

  /* Copy the path name to `fname', with modifications according to
     the -r and -t options. */

  truncate_name (fname, path);

  /* Extract the access mode and the sharing mode. */

  if (mode & 0x70)              /* Sharing mode */
    new_mode = mode & 0x70;
  else
    new_mode = 0x40;            /* Compatibility -> DENYNONE */
  new_mode |= mode & 0x83;      /* Inheritance, access code */

  /* Try to open the file and handle errors. */

  rc = DosOpen (fname, &hf, &action, 0, FILE_NORMAL,
                OPEN_ACTION_FAIL_IF_NEW | OPEN_ACTION_OPEN_IF_EXISTS,
                new_mode, NULL);
  if (rc != 0)
    {
      *errnop = set_error (rc);
      return -1;
    }

  /* Relocate the handle and update the flag bits and the file
     description for the new file handle. */

  e = reloc_handle (hf, &nh, TRUE);
  if (e != 0)
    {
      DosClose (hf);
      *errnop = e;
      return -1;
    }

  /* Return the handle and tell the caller not to modify `errno'. */

  *errnop = 0;
  return nh;
}


/* This function implements the __fcntl() system call. */

int do_fcntl (ULONG handle, ULONG request, ULONG arg, int *errnop)
{
  ULONG rc, ht, diff, state;
  int result;

  switch (request)
    {
    case F_GETFD:

      /* Return the close-on-exec flag.  It's the complement of OS/2's
         OPEN_FLAGS_NOINHERIT bit.  For sockets, the flag is stored in
         the `my_file' structure. */

      if (IS_SOCKET (handle))
        {
          if (!IS_VALID_FILE (handle))
            {
              *errnop = EBADF;
              return -1;
            }
          *errnop = 0;
          return GET_FILE (handle)->x.socket.fd_flags;
        }
        
      rc = DosQueryFHState (handle, &state);
      if (rc != 0)
        {
          *errnop = set_error (rc);
          return -1;
        }
      *errnop = 0;
      return ((state & OPEN_FLAGS_NOINHERIT) ? FD_CLOEXEC : 0);

    case F_SETFD:

      /* Set the close-on-exec flag. */

      if (IS_SOCKET (handle))
        {
          if (!IS_VALID_FILE (handle))
            {
              *errnop = EBADF;
              return -1;
            }
          GET_FILE (handle)->x.socket.fd_flags = arg;
          *errnop = 0;
          return 0;
        }

      rc = set_no_inherit (handle, arg);
      if (rc != 0)
        {
          *errnop = set_error (rc);
          return -1;
        }
      break;

    case F_GETFL:

      /* Return O_APPEND and/or O_NDELAY. */

      if (handle >= handle_count || !(handle_flags[handle] & HF_OPEN))
        {
          *errnop = EBADF;
          return -1;
        }
      ht = handle_flags[handle];
      result = 0;
      if (ht & HF_NDELAY)
        result |= O_NDELAY;
      if (ht & HF_APPEND)
        result |= O_APPEND;
      *errnop = 0;
      return result;

    case F_SETFL:

      /* Copy O_NDELAY and O_APPEND as HF_NDELAY and HF_APPEND to
         `handle_flags'.  If O_NDELAY changed for a named pipe, change
         pipe state.  If O_NDELAY is changed for a socket or pty,
         perform a FIONBIO ioctl. */

      if (arg & ~(O_NDELAY | O_APPEND))
        {
          *errnop = EINVAL;
          return -1;
        }
      if (handle >= handle_count || !(handle_flags[handle] & HF_OPEN))
        {
          *errnop = EBADF;
          return -1;
        }
      ht = handle_flags[handle];
      ht &= ~(HF_NDELAY | HF_APPEND);
      if (arg & O_NDELAY)
        ht |= HF_NDELAY;
      if (arg & O_APPEND)
        ht |= HF_APPEND;

      diff = handle_flags[handle] ^ ht; /* Changed flags */
      handle_flags[handle] = ht;

      /* Change the handle state if HF_NDELAY changed for a named
         pipe. */

      if ((diff & HF_NDELAY) && (ht & HF_NPIPE))
        {
          state = 0;
          if (ht & HF_NDELAY)
            state |= NP_NOWAIT;
          rc = DosSetNPHState (handle, state);
          if (rc != 0)
            {
              *errnop = set_error (rc);
              return -1;
            }
        }

      /* Change the handle state if HF_NDELAY changed for a socket. */

      if ((diff & HF_NDELAY) && (ht & HF_SOCKET))
        if (tcpip_fcntl (handle, F_SETFL, arg, errnop) != 0)
          return -1;

      /* Change the handle state if HF_NDELAY changed for a xf86sup
         device. */

      if ((diff & HF_NDELAY) && (ht & HF_XF86SUP))
        if (xf86sup_fcntl (handle, F_SETFL, arg, errnop) != 0)
          return -1;
      break;

    default:

      /* Set errno to EINVAL for unknown request codes. */

      *errnop = EINVAL;
      return -1;
    }

  /* Function successfully completed. */

  *errnop = 0;
  return 0;
}


/* This function emulates the DOS function 0x4400 (IOCTL, get device
   data). */

int do_ioctl1 (ULONG handle, int *errnop)
{
  ULONG rc, htype, hflags;
  int result;

  /* This function is not implemented for sockets. */

  if (IS_SOCKET (handle))
    {
      *errnop = EINVAL;
      return -1;
    }

  /* First, get the type of the file handle (pipe, device, or
     file). */

  rc = DosQueryHType (handle, &htype, &hflags);
  if (rc != 0)
    {
      *errnop = set_error (rc);
      return -1;
    }

  /* Set `result' depending on the file type. */

  result = 0;
  switch (htype & 0xff)
    {
    case HANDTYPE_PIPE:
      result = 0x80;            /* Device or pipe */
      break;

    case HANDTYPE_DEVICE:
      result = (hflags & 0x0f) | 0x80; /* kbd, scr, nul, clock */
      break;

    case HANDTYPE_FILE:
      /* Insert drive letter (not implemented). */
      break;

    default:
      /* Future extension. */
      break;
    }

  *errnop = 0;
  return result;
}


/* This function implements the __ioctl2() system call. */

int do_ioctl2 (ULONG handle, ULONG request, ULONG arg, int *errnop)
{
  ULONG ht;
  int n;

  /* This function is the first one called by the startup code which
     requires init_fileio().  If emx.dll is used by a DLL which is
     used by a program that does not use emx.dll, init_fileio() has
     not yet been called.  Do that now. */

  if (handle_flags == NULL)
    init_fileio ();

  /* Reject invalid handles and handles which are not open. */

  if (handle >= handle_count || !(handle_flags[handle] & HF_OPEN))
    {
      *errnop = EBADF;
      return -1;
    }

  /* Get the flag bits for the handle. */

  ht = handle_flags[handle];

  /* Handle sockets specially. */

  if (ht & HF_SOCKET)
    return tcpip_ioctl (handle, request, arg, errnop);

  /* Handle xf86sup devices specially. */

  if (ht & HF_XF86SUP)
    return xf86sup_ioctl (handle, request, arg, errnop);

  switch (request)
    {
    case TCGETA:

      /* Get `struct termio' of the file handle. */

      if (!(ht & (HF_CON|HF_ASYNC)) || !IS_VALID_FILE (handle))
        {
          *errnop = EINVAL;
          return -1;
        }
      termio_get (handle, (void *)arg);
      break;

    case TCSETA:
    case TCSETAF:
    case TCSETAW:

      /* Set `struct termio' of the file handle. */

      if (!(ht & (HF_CON|HF_ASYNC)) || !IS_VALID_FILE (handle))
        {
          *errnop = EINVAL;
          return -1;
        }
      if (request == TCSETAF)
        termio_flush (handle);
      termio_set (handle, (void *)arg);
      break;

    case TCFLSH:

      /* Flush the input and/or output queues. */

      if (!(ht & (HF_CON|HF_ASYNC)) || !IS_VALID_FILE (handle) || arg > 2)
        {
          *errnop = EINVAL;
          return -1;
        }
      if (arg == 0 || arg == 2)
        termio_flush (handle);  /* Flush input queue. */
#if 0
      if (arg == 1 || arg == 2)
        {
          /* Flush output queue; not implemented. */
        }
#endif
      break;

    case TCSBRK:

      /* Send break. */

      *errnop = 0;
      return 0;

    case TCXONC:

      /* Flow control. */

      *errnop = 0;
      return 0;

    case _TCGA:

      /* Get `struct termios' of the file handle, for tcgetattr(). */

      if (!(ht & (HF_CON|HF_ASYNC)) || !IS_VALID_FILE (handle))
        {
          *errnop = xlate_errno (ENOTTY);
          return -1;
        }
      termios_get (handle, (void *)arg);
      break;

    case _TCSANOW:
    case _TCSADRAIN:
    case _TCSAFLUSH:

      /* Set `struct termios' of the file handle, for tcsetattr(). */

      if (!(ht & (HF_CON|HF_ASYNC)) || !IS_VALID_FILE (handle))
        {
          *errnop = xlate_errno (ENOTTY);
          return -1;
        }
      if (request == _TCSAFLUSH)
        termio_flush (handle);
      termios_set (handle, (void *)arg);
      break;

    case FIONREAD:

      /* Get the number of characters available for reading. */

      if ((ht & (HF_CON|HF_ASYNC)) && IS_VALID_FILE (handle))
        n = termio_avail (handle);
      else if (ht & HF_NPIPE)
        {
          n = npipe_avail (handle, errnop);
          if (n == -1)
            return -1;
        }
      else
        {
          *errnop = EINVAL;
          return -1;
        }
      *(int *)arg = n;
      break;

    case FGETHTYPE:

      /* Get the file type. */

      if (ht & HF_UPIPE)
        n = HT_UPIPE;
      else if (ht & HF_NPIPE)
        n = HT_NPIPE;
      else if (ht & HF_SOCKET)
        n = HT_SOCKET;
      else if (!(ht & HF_DEV))
        n = HT_FILE;
      else if (ht & HF_CON)
        n = HT_DEV_CON;
      else if (ht & HF_NUL)
        n = HT_DEV_NUL;
      else if (ht & HF_CLK)
        n = HT_DEV_CLK;
      else
        n = HT_DEV_OTHER;
      *(int *)arg = n;
      break;

    default:
      *errnop = EINVAL;
      return -1;
    }

  /* Function successfully completed. */

  *errnop = 0;
  return 0;
}


/* Duplicate a file handle. */

int do_dup (ULONG src_fd, ULONG req_target, int *errnop)
{
  ULONG rc, state, new_fd;

  /* Handle socket handles specially. */

  if (IS_SOCKET (src_fd))
    return tcpip_dup (src_fd, req_target, errnop);

  if (src_fd == req_target)
    {
      rc = DosQueryFHState (src_fd, &state); /* Check SRC_FD */
      if (rc != 0)
        {
          *errnop = set_error (rc);
          return -1;
        }
      *errnop = 0;
      return req_target;
    }

  new_fd = req_target;
  LOCK_FILES;
  xf86sup_maybe_enadup (src_fd, TRUE);
  rc = DosDupHandle (src_fd, &new_fd);
  xf86sup_maybe_enadup (src_fd, FALSE);
  if (rc != 0)
    {
      UNLOCK_FILES;
      *errnop = set_error (rc);
      return -1;
    }

  /* Reject handles which are out of the supported range. */

  if (new_fd >= handle_count)
    {
      DosClose (new_fd);
      set_error (0);
      UNLOCK_FILES;
      *errnop = EMFILE;
      return -1;
    }

  /* A successful dup2() call closes the target handle.  Update our
     tables. */

  if (new_fd == req_target)
    {
      /* DosDupHandle doesn't know about sockets, so we have to close
         sockets here.  tcpip_close() calls close_handle(). */

      if (IS_SOCKET (new_fd))
        tcpip_close (new_fd);
      else
        close_handle (new_fd);
    }

  /* If the new handle hit a faked socket handle, relocate the new
     handle.  This can happen only for dup(), not for dup2(), because
     the target handle of dup2() has been closed above. */

  if (handle_flags[new_fd] & HF_SOCKET)
    {
      ULONG nf;

      if (reloc_handle (new_fd, &nf, FALSE) != 0)
        {
          DosClose (new_fd);
          UNLOCK_FILES;
          set_error (0);
          *errnop = EMFILE;
          return -1;
        }
      new_fd = nf;
    }

  /* Copy the source file handle's data to the new handle. */

  handle_file[new_fd] = handle_file[src_fd];
  handle_flags[new_fd] = handle_flags[src_fd];

  if (IS_VALID_FILE (src_fd))
    GET_FILE (src_fd)->ref_count += 1;
  else
    {
      SET_INVALID_FILE (new_fd);
      new_handle (new_fd);
    }
  UNLOCK_FILES;
  *errnop = 0;
  return new_fd;
}


/* Close a file handle. */

int do_close (ULONG handle)
{
  ULONG rc;

  if (IS_SOCKET (handle))
    return tcpip_close (handle);

  close_handle (handle);
  rc = DosClose (handle);
  if (rc != 0)
    return set_error (rc);
  return 0;
}


/* Read from a file handle. */

ULONG do_read (ULONG handle, void *dst, ULONG count, int *errnop)
{
  ULONG hflags, rc, nread;

  hflags = (handle < handle_count ? handle_flags[handle] : 0);
  if ((hflags & (HF_CON|HF_ASYNC)) && IS_VALID_FILE (handle)
      && !(GET_FILE (handle)->c_lflag & IDEFAULT))
    return termio_read (handle, dst, count, errnop);
  else if ((hflags & HF_SOCKET) && IS_VALID_FILE (handle))
    return tcpip_read (handle, dst, count, errnop);
  else
    {
      if (exe_heap)
        touch (dst, count);
      rc = DosRead (handle, dst, count, &nread);
      if (rc != 0)
        {
          *errnop = set_error (rc);
          return (ULONG)-1;
        }
      *errnop = 0;
      return nread;
    }
}


/* Write to a file handle. */

ULONG do_write (ULONG handle, void *src, ULONG count, int *errnop)
{
  ULONG rc, nwritten;

  if (IS_SOCKET (handle) && IS_VALID_FILE (handle))
    return tcpip_write (handle, src, count, errnop);

  if (exe_heap)
    touch (src, count);
  rc = DosWrite (handle, src, count, &nwritten);
  if (rc != 0)
    {
      *errnop = set_error (rc);
      return (ULONG)-1;
    }
  *errnop = 0;
  return nwritten;
}


/* Move the file pointer of a file handle. */

ULONG do_seek (ULONG handle, ULONG origin, LONG distance, int *errnop)
{
  ULONG rc, new_pos;

  if (IS_SOCKET (handle))
    {
      *errnop = ESPIPE;
      return (ULONG)-1;
    }

  rc = DosSetFilePtr (handle, distance, origin, &new_pos);
  if (rc != 0)
    {
      *errnop = set_error (rc);
      return (ULONG)-1;
    }
  *errnop = 0;
  return new_pos;
}


/* Create a pipe.  DST points to two int's which will receive the
   handles.  PIPESIZE is the size of the pipe.  Return errno. */

int do_pipe (int *dst, ULONG pipesize)
{
  HPIPE r_handle, w_handle, nh;
  ULONG rc, action, pn;
  char fname[128];
  int e;

  /* Create unique a pipe name (unique on this system). */

  LOCK_COMMON;
  pn = ++pipe_number;
  UNLOCK_COMMON;
  sprintf (fname, "/pipe/emx/pipes/%.8x.pip", (unsigned)pn);

  /* Create the pipe. */

  rc = DosCreateNPipe (fname, &r_handle,
                       NP_ACCESS_INBOUND,
                       1 | NP_NOWAIT | NP_TYPE_BYTE | NP_READMODE_BYTE,
                       pipesize, pipesize, 0);
  if (rc != 0)
    return set_error (rc);

  /* Connect to the pipe. */

  rc = DosConnectNPipe (r_handle);
  if (rc != 0 && rc != ERROR_PIPE_NOT_CONNECTED)
    {
      DosClose (r_handle);
      return set_error (rc);
    }

  /* Turn on NP_WAIT. */

  rc = DosSetNPHState (r_handle, NP_WAIT | NP_READMODE_BYTE);
  if (rc != 0)
    {
      DosClose (r_handle);
      return set_error (rc);
    }

  /* Open the write side of the pipe. */

  rc = DosOpen (fname, &w_handle, &action, 0, FILE_NORMAL,
                OPEN_ACTION_OPEN_IF_EXISTS | OPEN_ACTION_FAIL_IF_NEW,
                OPEN_ACCESS_WRITEONLY | OPEN_SHARE_DENYREADWRITE,
                NULL);
  if (rc != 0)
    {
      DosClose (r_handle);
      return set_error (rc);
    }

  /* Update the file handle table. */

  e = reloc_handle (r_handle, &nh, TRUE);
  if (e != 0)
    {
      DosClose (r_handle);
      DosClose (w_handle);
      return e;
    }
  r_handle = nh;

  e = reloc_handle (w_handle, &nh, TRUE);
  if (e != 0)
    {
      DosClose (r_handle);
      DosClose (w_handle);
      return e;
    }
  w_handle = nh;

  /* Set DST handles. */

  dst[0] = r_handle;
  dst[1] = w_handle;
  return 0;
}


/* Truncate file HANDLE to FSIZE bytes.  Return errno. */

int do_ftruncate (ULONG handle, ULONG fsize)
{
  FILESTATUS3 info;
  ULONG rc;

  if (IS_SOCKET (handle))
    return EINVAL;

  rc = DosQueryFileInfo (handle, FIL_STANDARD, &info, sizeof (info));
  if (rc != 0)
    return set_error (rc);
  if (fsize >= info.cbFile)
    return 0;
  rc = DosSetFileSize (handle, fsize);
  if (rc != 0)
    return set_error (rc);
  return 0;
}


/* Change size of file HANDLE to FSIZE bytes.  Return errno. */

int do_chsize (ULONG handle, ULONG fsize)
{
  ULONG rc;

  if (IS_SOCKET (handle))
    return EINVAL;

  rc = DosSetFileSize (handle, fsize);
  if (rc != 0)
    return set_error (rc);
  return 0;
}


/* Flush the buffers of a file. */

int do_fsync (ULONG handle, int *errnop)
{
  ULONG rc;

  if (IS_SOCKET (handle))
    return EINVAL;

  rc = DosResetBuffer (handle);
  if (rc != 0)
    {
      *errnop = set_error (rc);
      return -1;
    }
  *errnop = 0;
  return 0;
}


/* Store the name of the file-system driver of drive DRV_NAME to DST
   (up to DST_SIZE bytes, including the terminating null character).
   Store errno to *ERRNOP.  Return 0 on success, -1 on error. */

int do_filesys (char *dst, ULONG dst_size, const char *drv_name, int *errnop)
{
  char buf[sizeof (PFSQBUFFER2) + 128];
  PFSQBUFFER2 pfsq = (PFSQBUFFER2)buf;
  ULONG rc, len;

  len = sizeof (buf);
  rc = DosQueryFSAttach (drv_name, 1, FSAIL_QUERYNAME, pfsq, &len);
  if (rc != 0)
    {
      *errnop = set_error (rc);
      return -1;
    }
  if (pfsq->iType != FSAT_LOCALDRV && pfsq->iType != FSAT_REMOTEDRV)
    {
      *errnop = EINVAL;
      return -1;
    }
  if (pfsq->cbFSDName >= dst_size)
    {
      *errnop = E2BIG;
      return -1;
    }
  strcpy (dst, pfsq->szFSDName + pfsq->cbName);
  *errnop = 0;
  return 0;
}


/* Create a directory. */

int do_mkdir (const char *path)
{
  char fname[512];
  ULONG rc;

  /* Copy the path name to `fname', with modifications according to
     the -r and -t options. */

  truncate_name (fname, path);

  /* Create the directory and handle errors. */

  rc = DosCreateDir (fname, NULL);
  if (rc != 0)
    return set_error (rc);
  return 0;
}


/* Remove a directory. */

int do_rmdir (const char *path)
{
  char fname[512];
  ULONG rc;

  /* Copy the path name to `fname', with modifications according to
     the -r and -t options. */

  truncate_name (fname, path);

  /* Remove the directory and handle errors. */

  rc = DosDeleteDir (fname);
  if (rc != 0)
    return set_error (rc);
  return 0;
}


/* Change the current working directory (of a disk drive). */

int do_chdir (const char *path)
{
  char fname[512];
  ULONG rc;

  /* Copy the path name to `fname', with modifications according to
     the -r and -t options. */

  truncate_name (fname, path);

  /* Change the current working directory and handle errors. */

  rc = DosSetCurrentDir (fname);
  if (rc != 0)
    return set_error (rc);
  return 0;
}


/* Delete a file. */

int do_delete (const char *path)
{
  char fname[512];
  ULONG rc;

  /* Copy the path name to `fname', with modifications according to
     the -r and -t options. */

  truncate_name (fname, path);

  /* Delete the file and handle errors. */

  rc = DosDelete (fname);
  if (rc != 0)
    return set_error (rc);
  return 0;
}


/* Rename or move a file. */

int do_rename (const char *old_path, const char *new_path)
{
  char old_fname[512], new_fname[512];
  ULONG rc;

  /* Copy the path names to `fname1' and `fname2', respectively, with
     modifications according to the -r and -t options. */

  truncate_name (old_fname, old_path);
  truncate_name (new_fname, new_path);

  /* Rename the file handle errors. */

  rc = DosMove (old_fname, new_fname);
  if (rc != 0)
    return set_error (rc);
  return 0;
}


/* Return the index of the current disk, 0 = A:. */

UCHAR do_getdrive (void)
{
  ULONG drv;
  ULONG map;

  DosQueryCurrentDisk (&drv, &map);
  return (UCHAR)(drv - 1);
}


/* Select disk DRV (0 = A:) as current disk. */

ULONG do_selectdisk (ULONG drv)
{
  ULONG rc;

  rc = DosSetDefaultDisk (drv + 1);
  if (rc != 0)
    return set_error (rc);
  return 0;
}


/* Return the file attributes of PATH. */

ULONG get_attr (const char *path, int *errnop)
{
  char fname[512];
  FILESTATUS3 status;
  ULONG rc;

  /* This function cannot be applied to named pipes. */

  if (check_npipe (path))
    {
      *errnop = ENOENT;
      return 0;
    }

  /* Copy the path name to `fname', with modifications according to
     the -r and -t options. */

  truncate_name (fname, path);

  /* Get the file attributes and handle errors. */

  rc = query_path_info (fname, FIL_STANDARD, &status, sizeof (status));
  if (rc != 0)
    {
      *errnop = set_error (rc);
      return 0;
    }
  *errnop = 0;
  return status.attrFile;
}


/* Set the file attributes of PATH. */

ULONG set_attr (const char *path, ULONG attr)
{
  char fname[512];
  FILESTATUS3 status;
  ULONG rc;

  /* This function cannot be applied to named pipes. */

  if (check_npipe (path))
    return ENOENT;

  /* Copy the path name to `fname', with modifications according to
     the -r and -t options. */

  truncate_name (fname, path);

  /* Get the current file attributes and handle errors. */

  rc = query_path_info (fname, FIL_STANDARD, &status, sizeof (status));
  if (rc != 0)
    return set_error (rc);

  /* Update the file attributes and handle errors. */

  status.attrFile = attr;
  rc = DosSetPathInfo (fname, FIL_STANDARD, &status, sizeof (status), 0);
  if (rc != 0)
    return set_error (rc);
  return 0;
}


/* Get the current working directory of disk DRIVE. */

ULONG do_getcwd (char *dst, int drive)
{
  ULONG rc, len;

  len = 512;
  rc = DosQueryCurrentDir (drive, dst, &len);
  if (rc != 0)
    return set_error (rc);
  return 0;
}


static ULONG find_conv (thread_data *td, struct _find *dst);
static void find_close (thread_data *td);

/* This function implements the __findfirst() system call. */

ULONG do_find_first (const char *path, ULONG attr, struct _find *dst)
{
  char fname[512];
  thread_data *td;
  ULONG rc;

  td = get_thread ();

  /* Copy the path name to `fname', with modifications according to
     the -r and -t options. */

  truncate_name (fname, path);

  if (td->find_handle != HDIR_CREATE)
    {
      /* Closing the handle is not strictly required as DosFindFirst
         can reuse an open handle.  However, this simplifies error
         handling below (will DosFindFirst close the handle on error
         if it is open?). */

      DosFindClose (td->find_handle);
      td->find_handle = HDIR_CREATE;
    }

  td->find_count = ((debug_flags & DEBUG_FIND1) ? 1 : FIND_COUNT);
  rc = DosFindFirst (fname, &td->find_handle, attr, &td->find_buf[0],
                     sizeof (td->find_buf), &td->find_count, FIL_STANDARD);
  if (rc != 0)
    {
      td->find_handle = HDIR_CREATE; /* Perhaps modified by DosFindFirst */
      td->find_count = 0;
      td->find_next = NULL;
      return set_error (rc);
    }
  td->find_next = &td->find_buf[0];
  return find_conv (td, dst);
}


/* This function implements the __findnext() system call. */

ULONG do_find_next (struct _find *dst)
{
  thread_data *td;
  ULONG rc;

  td = get_thread ();
  if (td->find_count < 1)
    {
      td->find_count = ((debug_flags & DEBUG_FIND1) ? 1 : FIND_COUNT);
      rc = DosFindNext (td->find_handle, &td->find_buf[0],
                        sizeof (td->find_buf), &td->find_count);
      if (rc != 0)
        {
          find_close (td);
          return set_error (rc);
        }
      td->find_next = &td->find_buf[0];
    }

  return find_conv (td, dst);
}


/* Build a `struct _find' structure from a FILEFINDBUF3 structure and
   move to the next one. */

static ULONG find_conv (thread_data *td, struct _find *dst)
{
  const FILEFINDBUF3 *src;

  /* Check if there are any entries.  Close the handle and return
     ENOENT if there are no entries.  (Checking SRC is redundant.) */

  src = td->find_next;
  if (td->find_count < 1 || src == NULL)
    {
      find_close (td);
      return ENOENT;
    }

  /* Fill-in target object. */

  dst->attr = src->attrFile;
  dst->time = XUSHORT (src->ftimeLastWrite);
  dst->date = XUSHORT (src->fdateLastWrite);
  dst->size_lo = (USHORT)src->cbFile;
  dst->size_hi = (USHORT)(src->cbFile >> 16);
  strcpy (dst->name, src->achName);

  /* Move to the next entry. */

  if (src->oNextEntryOffset == 0)
    {
      td->find_next = NULL;
      td->find_count = 0;
    }
  else
    {
      td->find_next = (FILEFINDBUF3 *)((char *)src + src->oNextEntryOffset);
      td->find_count -= 1;
    }
  return 0;
}


/* Close the directory handle. */

static void find_close (thread_data *td)
{
  if (td->find_handle != HDIR_CREATE)
    {
      DosFindClose (td->find_handle);
      td->find_handle = HDIR_CREATE;
    }
  td->find_count = 0;
  td->find_next = NULL;
}


/* Get the timestamp (last modification) of HANDLE. */

ULONG do_get_timestamp (ULONG handle, ULONG *time, ULONG *date)
{
  FILESTATUS3 status;
  ULONG rc;

  if (IS_SOCKET (handle))
    return EINVAL;

  rc = DosQueryFileInfo (handle, FIL_STANDARD, &status, sizeof (status));
  if (rc != 0)
    return set_error (rc);
  *time = XUSHORT (status.ftimeLastWrite);
  *date = XUSHORT (status.fdateLastWrite);
  return 0;
}


/* Set the timestamp (last modification) of HANDLE. */

ULONG do_set_timestamp (ULONG handle, ULONG time, ULONG date)
{
  FILESTATUS3 status;
  ULONG rc;

  if (IS_SOCKET (handle))
    return EINVAL;

  rc = DosQueryFileInfo (handle, FIL_STANDARD, &status, sizeof (status));
  if (rc != 0)
    return set_error (rc);
  XUSHORT (status.ftimeLastWrite) = (USHORT)time;
  XUSHORT (status.fdateLastWrite) = (USHORT)date;
  rc = DosSetFileInfo (handle, FIL_STANDARD, &status, sizeof (status));
  if (rc != 0)
    return set_error (rc);
  return 0;
}


/* Convert a timestamp in Unix format to DOS format. */

static void unix2dosdt (FDATE *pdate, FTIME *ptime, ULONG ut)
{
  struct my_datetime mdt;

  unix2time (&mdt, ut);
  ptime->twosecs = mdt.seconds / 2;
  ptime->minutes = mdt.minutes;
  ptime->hours = mdt.hours;
  pdate->day = mdt.day;
  pdate->month = mdt.month;
  pdate->year = mdt.year - 1980;
}


/* This function implements the __utimes() system call. */

int do_utimes (const char *path, const struct timeval *tv, int *errnop)
{
  char fname[512];
  FILESTATUS3 info;
  ULONG rc;

  /* This function cannot be applied to named pipes. */

  if (check_npipe (path))
    {
      *errnop = ENOENT;
      return -1;
    }

  /* Copy the path name to `fname', with modifications according to
     the -r and -t options. */

  truncate_name (fname, path);

  /* Get the file status and handle errors. */

  rc = query_path_info (fname, FIL_STANDARD, &info, sizeof (info));
  if (rc != 0)
    {
      *errnop = set_error (rc);
      return -1;
    }

  /* Update the modification time and date. */

  unix2dosdt (&info.fdateLastAccess, &info.ftimeLastAccess, tv[0].tv_sec);
  unix2dosdt (&info.fdateLastWrite, &info.ftimeLastWrite, tv[1].tv_sec);

  /* Set the file status and handle errors. */

  rc = DosSetPathInfo (fname, FIL_STANDARD, &info, sizeof (info), 0);
  if (rc != 0)
    {
      *errnop = set_error (rc);
      return -1;
    }

  /* Function successfully completed. */

  *errnop = 0;
  return 0;
}


/* This function implements the __stat() system call. */

int do_stat (const char *path, struct stat *dst, int *errnop)
{
  FILESTATUS3 info;
  char fname[512];
  ULONG rc;

  /* __stat() cannot be applied to named pipes. */

  if (check_npipe (path))
    {
      *errnop = ENOENT;
      return -1;
    }

  /* Copy the path name to `fname', with modifications according to
     the -r and -t options. */

  truncate_name (fname, path);

  /* Get the file status and handle errors. */

  memset (dst, 0, sizeof (struct stat));
  rc = query_path_info (fname, FIL_STANDARD, &info, sizeof (info));
  if (rc != 0)
    {
      *errnop = set_error (rc);
      return -1;
    }

  dst->st_attr = info.attrFile;
  dst->st_reserved = 0;

  dst->st_mtime = packed2unix (info.fdateLastWrite, info.ftimeLastWrite);
  if (XUSHORT (info.fdateCreation) == 0 && XUSHORT (info.ftimeCreation) == 0)
    dst->st_ctime = dst->st_mtime;
  else
    dst->st_ctime = packed2unix (info.fdateCreation, info.ftimeCreation);
  if (XUSHORT (info.fdateLastAccess) == 0
      && XUSHORT (info.ftimeLastAccess) == 0)
    dst->st_atime = dst->st_mtime;
  else
    dst->st_atime = packed2unix (info.fdateLastAccess, info.ftimeLastAccess);

  if (info.attrFile & 0x10)
    {
      /* It's a directory. */
      dst->st_size = 0;
      dst->st_mode = S_IFDIR | MAKEPERM (S_IREAD|S_IWRITE|S_IEXEC);
    }
  else
    {
      /* It's a regular file. */
      dst->st_size = info.cbFile;
      if (info.attrFile & 0x01)
        dst->st_mode = S_IFREG | MAKEPERM (S_IREAD);
      else
        dst->st_mode = S_IFREG | MAKEPERM (S_IREAD|S_IWRITE);
    }
  dst->st_dev = 0;
  dst->st_uid = 0;              /* root */
  dst->st_gid = 0;              /* root */
  dst->st_ino = ino_number;
  if (++ino_number == 0)
   ino_number = 1;
  dst->st_rdev = dst->st_dev;
  dst->st_nlink = 1;
  *errnop = 0;
  return 0;
}


/* This function implements the __fstat() system call. */

int do_fstat (ULONG handle, struct stat *dst, int *errnop)
{
  FILESTATUS3 info;
  ULONG rc, flags, htype, state;

  if (IS_SOCKET (handle))
    return tcpip_fstat (handle, dst, errnop);

  memset (dst, 0, sizeof (struct stat));
  rc = DosQueryHType (handle, &htype, &flags);
  if (rc != 0)
    {
      *errnop = set_error (rc);
      return -1;
    }

  switch (htype & 0xff)
    {
    case HANDTYPE_DEVICE:       /* Character device */
    case HANDTYPE_PIPE:         /* Pipe */
      rc = DosQueryFHState (handle, &state);
      if (rc != 0)
        {
          *errnop = set_error (rc);
          return -1;
        }
      if ((state & 7) == OPEN_ACCESS_READONLY)
        dst->st_mode = MAKEPERM (S_IREAD);
      else
        dst->st_mode = MAKEPERM (S_IREAD|S_IWRITE);
      if ((htype & 0xff) == HANDTYPE_DEVICE)
        dst->st_mode |= S_IFCHR;
      else
        dst->st_mode |= S_IFIFO;
      dst->st_size = 0;
      break;

    default:
      rc = DosQueryFileInfo (handle, FIL_STANDARD, &info, sizeof (info));
      if (rc != 0)
        {
          *errnop = set_error (rc);
          return -1;
        }
      dst->st_attr = info.attrFile;
      dst->st_reserved = 0;

      dst->st_mtime = packed2unix (info.fdateLastWrite, info.ftimeLastWrite);
      if (XUSHORT (info.fdateCreation) == 0
          && XUSHORT (info.ftimeCreation) == 0)
        dst->st_ctime = dst->st_mtime;
      else
        dst->st_ctime = packed2unix (info.fdateCreation, info.ftimeCreation);
      if (XUSHORT (info.fdateLastAccess) == 0
          && XUSHORT (info.ftimeLastAccess) == 0)
        dst->st_atime = dst->st_mtime;
      else
        dst->st_atime = packed2unix (info.fdateLastAccess,
                                     info.ftimeLastAccess);

      dst->st_size = info.cbFile;
      if (info.attrFile & 0x01)
        dst->st_mode = S_IFREG | MAKEPERM (S_IREAD);
      else
        dst->st_mode = S_IFREG | MAKEPERM (S_IREAD|S_IWRITE);
      break;
    }
  dst->st_dev = 0;
  dst->st_uid = 0;              /* root */
  dst->st_gid = 0;              /* root */
  dst->st_ino = ino_number;
  if (++ino_number == 0)
   ino_number = 1;
  dst->st_rdev = dst->st_dev;
  dst->st_nlink = 1;
  *errnop = 0;
  return 0;
}


/* Send the character CHR to standard output. */

void conout (UCHAR chr)
{
  ULONG written;

  DosWrite (1, &chr, 1, &written);
}


/* This function implements the __imphandle() system call: Import an
   OS/2 handle. */

ULONG do_imphandle (ULONG handle, int *errnop)
{
  ULONG nh;
  int e;

  e = reloc_handle (handle, &nh, TRUE);
  if (e != 0)
    {
      *errnop = e;
      return (ULONG)-1;
    }
  *errnop = 0;
  return nh;
}


/* Store the name of the device HANDLE to DST (up to DST_SIZE bytes,
   including the terminating null character).  Store errno to *ERRNOP.
   Return 0 on success, -1 on error. */

int do_ttyname (char *dst, ULONG dst_size, ULONG handle, int *errnop)
{
  if (handle >= handle_count || !(handle_flags[handle] & HF_OPEN))
    {
      *errnop = EBADF;
      return -1;
    }
  if (handle_flags[handle] & HF_CON)
    return set_ttyname (dst, dst_size, "/dev/con", errnop);
  else if (handle_flags[handle] & HF_NUL)
    return set_ttyname (dst, dst_size, "/dev/nul", errnop);
  else if (handle_flags[handle] & HF_CLK)
    return set_ttyname (dst, dst_size, "/dev/clock$", errnop);
  else if (handle_flags[handle] & HF_XF86SUP)
    return xf86sup_ttyname (dst, dst_size, handle, errnop);
  else
    {
      *errnop = ENODEV;
      return -1;
    }
}


int set_ttyname (char *dst, ULONG dst_size, const char *name, int *errnop)
{
  if (strlen (name) >= dst_size)
    {
      *errnop = ENAMETOOLONG;
      return -1;
    }
  strcpy (dst, name);
  *errnop = 0;
  return 0;
}
