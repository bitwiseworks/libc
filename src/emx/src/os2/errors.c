/* errors.c -- Translate OS/2 error codes to errno codes
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


#include <os2emx.h>
#include <sys/errno.h>
#include "emxdll.h"

/* This table is used for converting OS/2 error codes to errno values.
   Currently, there are too few errno values.  Therefore, some entries
   are suspect.

   Mapping ERROR_NOT_READY (21) to EIO is required for opening
   ptys. */

static const UCHAR errno_table[] =
{
  EINVAL,  EINVAL,       ENOENT,  ENOENT,  EMFILE,  /* 0..4 */
  EACCES,  EBADF,        EIO,     ENOMEM,  EIO,     /* 5..9 */
  EINVAL,  ENOEXEC,      EINVAL,  EINVAL,  EINVAL,  /* 10..14 */
  ENOENT,  EBUSY,        EXDEV,   ENOENT,  EROFS,   /* 15..19 */
  EIO,     EIO,          EIO,     EIO,     EIO,     /* 20..24 */
  EIO,     EIO,          EIO,     ENOSPC,  EIO,     /* 25..29 */
  EIO,     EIO,          EACCES,  EACCES,  EIO,     /* 30..34 */
  EIO,     EIO,          EIO,     EIO,     ENOSPC,  /* 35..39 */
  EIO,     EIO,          EIO,     EIO,     EIO,     /* 40..44 */
  EIO,     EIO,          EIO,     EIO,     EIO,     /* 45..49 */
  EIO,     EIO,          EIO,     EIO,     EBUSY,   /* 50..54 */
  EIO,     EIO,          EIO,     EIO,     EIO,     /* 55..59 */
  EIO,     ENOSPC,       ENOSPC,  EIO,     EIO,     /* 60..64 */
  EACCES,  EIO,          EIO,     EIO,     EIO,     /* 65..69 */
  EIO,     EIO,          EIO,     EROFS,   EIO,     /* 70..74 */
  EIO,     EIO,          EIO,     EIO,     EIO,     /* 75..79 */
  EEXIST,  EIO,          ENOENT,  EIO,     EIO,     /* 80..84 */
  EIO,     EIO,          EINVAL,  EIO,     EAGAIN,  /* 85..89 */
  EIO,     EIO,          EIO,     EIO,     EIO,     /* 90..94 */
  EINTR,   EIO,          EIO,     EIO,     EACCES,  /* 95..99 */
  ENOMEM,  EINVAL,       EINVAL,  ENOMEM,  EINVAL,  /* 100..104 */
  EINVAL,  ENOMEM,       EIO,     EACCES,  EPIPE,   /* 105..109 */
  ENOENT,  E2BIG,        ENOSPC,  ENOMEM,  EBADF,   /* 110..114 */
  EINVAL,  EINVAL,       EINVAL,  EINVAL,  EINVAL,  /* 115..119 */
  EINVAL,  EINVAL,       EINVAL,  ENOENT,  EINVAL,  /* 120..124 */
  ENOENT,  ENOENT,       ENOENT,  ECHILD,  ECHILD,  /* 125..129 */
  EACCES,  EINVAL,       ESPIPE,  EINVAL,  EINVAL,  /* 130..134 */
  EINVAL,  EINVAL,       EINVAL,  EINVAL,  EINVAL,  /* 135..139 */
  EINVAL,  EINVAL,       EBUSY,   EINVAL,  EINVAL,  /* 140..144 */
  EINVAL,  EINVAL,       EINVAL,  EBUSY,   EINVAL,  /* 145..149 */
  EINVAL,  EINVAL,       ENOMEM,  EINVAL,  EINVAL,  /* 150..154 */
  EINVAL,  EINVAL,       EINVAL,  EINVAL,  EINVAL,  /* 155..159 */
  EINVAL,  EINVAL,       EINVAL,  EINVAL,  EAGAIN,  /* 160..164 */
  EINVAL,  EINVAL,       EACCES,  EINVAL,  EINVAL,  /* 165..169 */
  EBUSY,   EINVAL,       EINVAL,  EINVAL,  EINVAL,  /* 170..174 */
  EINVAL,  EINVAL,       EINVAL,  EINVAL,  EINVAL,  /* 175..179 */
  EINVAL,  EINVAL,       EINVAL,  EINVAL,  ECHILD,  /* 180..184 */
  EINVAL,  EINVAL,       ENOENT,  EINVAL,  EINVAL,  /* 185..189 */
  ENOEXEC, ENOEXEC,      ENOEXEC, ENOEXEC, ENOEXEC, /* 190..194 */
  ENOEXEC, ENOEXEC,      ENOEXEC, ENOEXEC, ENOEXEC, /* 195..199 */
  ENOEXEC, ENOEXEC,      ENOEXEC, ENOENT,  EINVAL,  /* 200..204 */
  EINVAL,  ENAMETOOLONG, EINVAL,  EINVAL,  EINVAL,  /* 205..209 */
  EINVAL,  EINVAL,       EACCES,  ENOEXEC, ENOEXEC, /* 210..214 */
  EINVAL,  EINVAL,       EINVAL,  EINVAL,  EINVAL,  /* 215..219 */
  EINVAL,  EINVAL,       EINVAL,  EINVAL,  EINVAL,  /* 220..224 */
  EINVAL,  EINVAL,       EINVAL,  ECHILD,  EINVAL,  /* 225..229 */
  EINVAL,  EBUSY,        EAGAIN,  ENOTCONN, EINVAL, /* 230..234 */
  EINVAL,  EINVAL,       EINVAL,  EINVAL,  EINVAL,  /* 235..239 */
  EINVAL,  EINVAL,       EINVAL,  EINVAL,  EINVAL,  /* 240..244 */
  EINVAL,  EINVAL,       EINVAL,  EINVAL,  EINVAL,  /* 245..249 */
  EACCES,  EACCES,       EINVAL,  ENOENT,  EINVAL,  /* 250..254 */
  EINVAL,  EINVAL,       EINVAL,  EINVAL,  EINVAL,  /* 255..259 */
  EINVAL,  EINVAL,       EINVAL,  EINVAL,  EINVAL,  /* 260..264 */
  EINVAL,  EINVAL,       EINVAL,  EINVAL,  EINVAL,  /* 265..269 */
  EINVAL,  EINVAL,       EINVAL,  EINVAL,  EINVAL,  /* 270..274 */
  EINVAL,  EINVAL,       EINVAL,  EINVAL,  EINVAL,  /* 275..279 */
  EINVAL,  EINVAL,       EINVAL,  EINVAL,  EEXIST,  /* 280..284 */
  EEXIST,  EINVAL,       EINVAL,  EINVAL,  EINVAL,  /* 285..289 */
  ENOMEM,  EMFILE,       EINVAL,  EINVAL,  EINVAL,  /* 290..294 */
  EINVAL,  EINVAL,       EINVAL,  EINVAL,  EINVAL,  /* 295..299 */
  EINVAL,  EBUSY,        EINVAL,  ESRCH,   EINVAL,  /* 300..304 */
  ESRCH,   EINVAL,       EINVAL,  EINVAL,  ESRCH,   /* 305..309 */
  EINVAL,  ENOMEM,       EINVAL,  EINVAL,  EINVAL,  /* 310..314 */
  EINVAL,  E2BIG,        ENOENT,  EIO,     EIO,     /* 315..319 */
  EINVAL,  EINVAL,       EINVAL,  EINVAL,  EAGAIN,  /* 320..324 */
  EINVAL,  EINVAL,       EINVAL,  EIO,     ENOENT,  /* 325..329 */
  EACCES,  EACCES,       EACCES,  ENOENT,  ENOMEM   /* 330..334 */
};


/* This table translates errno values for `interface 0'.  Changed
   entries are marked (*). */

static const UCHAR errno_interface_0[] =
{

  0,                            /* 0:  0            */
  EACCES,                       /* 1:  EPERM (*)    */
  ENOENT,                       /* 2:  ENOENT       */
  ESRCH,                        /* 3:  ESRCH        */
  EINTR,                        /* 4:  EINTR        */
  EIO,                          /* 5:  EIO          */
  EINVAL,                       /* 6:  ENXIO (*)    */
  E2BIG,                        /* 7:  E2BIG        */
  ENOEXEC,                      /* 8:  ENOEXEC      */
  EBADF,                        /* 9:  EBADF        */
  ECHILD,                       /* 10: ECHILD       */
  EAGAIN,                       /* 11: EAGAIN       */
  ENOMEM,                       /* 12: ENOMEM       */
  EACCES,                       /* 13: EACCES       */
  EINVAL,                       /* 14: EFAULT (*)   */
  EINVAL,                       /* 15: ENOLCK (*)   */
  EACCES,                       /* 16: EBUSY (*)    */
  EEXIST,                       /* 17: EEXIST       */
  EXDEV,                        /* 18: EXDEV        */
  EINVAL,                       /* 19: ENODEV (*)   */
  EACCES,                       /* 20: ENOTDIR (*)  */
  EACCES,                       /* 21: EISDIR (*)   */
  EINVAL,                       /* 22: EINVAL       */
  EMFILE,                       /* 23: ENFILE (*)   */
  EMFILE,                       /* 24: EMFILE       */
  EINVAL,                       /* 25: ENOTTY (*)   */
  EINVAL,                       /* 26: EDEADLK (*)  */
  EINVAL,                       /* 27: EFBIG (*)    */
  ENOSPC,                       /* 28: ENOSPC       */
  EINVAL,                       /* 29: ESPIPE (*)   */
  EACCES,                       /* 30: EROFS (*)    */
  EINVAL,                       /* 31: EMLINK (*)   */
  EPIPE,                        /* 32: EPIPE        */
  EINVAL,                       /* 33: EDOM (*)     */
  ERANGE,                       /* 34: ERANGE       */
  EACCES,                       /* 35: ENOTEMPTY (*)    */
  EINVAL,                       /* 36: EINPROGRESS (*)  */
  ENOSYS,                       /* 37: ENOSYS           */
  EINVAL                        /* 38: ENAMETOOLONG (*) */
};


/* This table translates errno values for `interface 1' (up to and
   including 0.8h).  Changed entries are marked (*). */

static const UCHAR errno_interface_1[] =
{

  0,                            /* 0:  0            */
  EPERM,                        /* 1:  EPERM        */
  ENOENT,                       /* 2:  ENOENT       */
  ESRCH,                        /* 3:  ESRCH        */
  EINTR,                        /* 4:  EINTR        */
  EIO,                          /* 5:  EIO          */
  EINVAL,                       /* 6:  ENXIO (*)    */
  E2BIG,                        /* 7:  E2BIG        */
  ENOEXEC,                      /* 8:  ENOEXEC      */
  EBADF,                        /* 9:  EBADF        */
  ECHILD,                       /* 10: ECHILD       */
  EAGAIN,                       /* 11: EAGAIN       */
  ENOMEM,                       /* 12: ENOMEM       */
  EACCES,                       /* 13: EACCES       */
  EINVAL,                       /* 14: EFAULT (*)   */
  EINVAL,                       /* 15: ENOLCK (*)   */
  EACCES,                       /* 16: EBUSY (*)    */
  EEXIST,                       /* 17: EEXIST       */
  EXDEV,                        /* 18: EXDEV        */
  EINVAL,                       /* 19: ENODEV (*)   */
  ENOTDIR,                      /* 20: ENOTDIR      */
  EISDIR,                       /* 21: EISDIR       */
  EINVAL,                       /* 22: EINVAL       */
  EMFILE,                       /* 23: ENFILE (*)   */
  EMFILE,                       /* 24: EMFILE       */
  EINVAL,                       /* 25: ENOTTY (*)   */
  EINVAL,                       /* 26: EDEADLK (*)  */
  EINVAL,                       /* 27: EFBIG (*)    */
  ENOSPC,                       /* 28: ENOSPC       */
  ESPIPE,                       /* 29: ESPIPE       */
  EROFS,                        /* 30: EROFS        */
  EINVAL,                       /* 31: EMLINK (*)   */
  EPIPE,                        /* 32: EPIPE        */
  EDOM,                         /* 33: EDOM         */
  ERANGE,                       /* 34: ERANGE       */
  EACCES,                       /* 35: ENOTEMPTY (*)   */
  EINVAL,                       /* 36: EINPROGRESS (*) */
  ENOSYS,                       /* 37: ENOSYS       */
  ENAMETOOLONG                  /* 38: ENAMETOOLONG */
};


/* Set last_sys_errno according to RC and return an errno value. */

ULONG set_error (ULONG rc)
{
  thread_data *td;

  td = get_thread ();
  if (td != NULL)
    td->last_sys_errno = rc;

  if (rc >= sizeof (errno_table))
    return EINVAL;

  switch (interface)
    {
    case 0:
      return errno_interface_0[errno_table[rc]];
    case 1:
      return errno_interface_1[errno_table[rc]];
    default:
      return errno_table[rc];
    }
}


/* Translate an errno value according to the interface.  This function
   should be called when returning an errno value marked (*) in the
   errno_interface_0 table. */

int xlate_errno (int e)
{
  switch (interface)
    {
    case 0:
      if ((unsigned)e >= sizeof (errno_interface_0))
        return EINVAL;
      return errno_interface_0[e];
    case 1:
      if ((unsigned)e >= sizeof (errno_interface_1))
        return EINVAL;
      return errno_interface_1[e];
    default:
      return e;
    }
}
