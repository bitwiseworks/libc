/* doscall.c -- DOS-compatible system calls
   Copyright (c) 1994-1995 by Eberhard Mattes

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


#define INCL_DOSMISC
#define INCL_KBD
#include <os2emx.h>
#include <sys/errno.h>
#include "emxdll.h"
#include "clib.h"

static void bad_func (syscall_frame *f);
static void do_get_date (syscall_frame *f);
static void do_get_time (syscall_frame *f);
static void do_get_version (syscall_frame *f);


void dos_call (syscall_frame *f)
{
  int err_no, i;

  if (debug_flags & DEBUG_SYSCALL)
    oprintf ("%u: syscall %.2x\r\n", my_pid, (unsigned)f->b.ah);

  f->e.eflags &= ~(FLAG_C | FLAG_Z);

  switch (f->b.ah)
    {
    case 0x01:

      /* Read keyboard with echo. */

      kbd_input (&f->b.al, FALSE, TRUE, TRUE, FALSE);
      break;

    case 0x02:

      /* Display character. */

      conout (f->b.dl);
      break;

    case 0x06:

      /* Direct console I/O. */

      if (f->b.dl != 0xff)
        conout (f->b.dl);
      else
        {
          if (!kbd_input (&f->b.al, TRUE, FALSE, FALSE, FALSE))
            f->e.eflags |= FLAG_Z;
        }
      break;

    case 0x07:

      /* Direct console input. */

      kbd_input (&f->b.al, TRUE, FALSE, TRUE, FALSE);
      break;

    case 0x08:

      /* Read keyboard. */

      kbd_input (&f->b.al, FALSE, FALSE, TRUE, FALSE);
      break;

    case 0x0a:

      /* Buffered keyboard input. */

      conin_string ((char *)f->e.edx);
      break;

    case 0x0b:

      /* Check keyboard status. */

      f->b.al = kbd_input (NULL, FALSE, FALSE, FALSE, TRUE);
      break;

    case 0x0d:

      /* Reset disk. */

      break;

    case 0x0e:

      /* Select disk. */

      do_selectdisk (f->b.dl);
      f->b.al = 0;
      break;

    case 0x19:

      /* Get current disk. */

      f->b.al = do_getdrive ();
      break;

    case 0x2a:

      /* Get date. */

      do_get_date (f);
      break;

    case 0x2c:

      /* Get time. */

      do_get_time (f);
      break;

    case 0x30:

      /* Get version number. */

      do_get_version (f);
      break;

    case 0x37:

      /* Get or set the switch character. */

      f->b.al = 0xff;           /* No switch character defined. */
      break;

    case 0x39:

      /* Create directory. */

      f->e.eax = do_mkdir ((const char *)f->e.edx);
      if (f->e.eax != 0)
        f->e.eflags |= FLAG_C;
      break;

    case 0x3a:

      /* Remove directory. */

      f->e.eax = do_rmdir ((const char *)f->e.edx);
      if (f->e.eax != 0)
        f->e.eflags |= FLAG_C;
      break;

    case 0x3b:

      /* Change current directory. */

      f->e.eax = do_chdir ((const char *)f->e.edx);
      if (f->e.eax != 0)
        f->e.eflags |= FLAG_C;
      break;

    case 0x3c:

      /* Create handle. */

      i = old_creat ((const char *)f->e.edx, f->e.ecx, &err_no);
      if (i != -1)
        f->e.eax = i;
      else
        {
          f->e.eax = err_no;
          f->e.eflags |= FLAG_C;
        }
      break;

    case 0x3d:

      /* Open handle. */

      i = old_open ((const char *)f->e.edx, f->b.al, &err_no);
      if (i != -1)
        f->e.eax = i;
      else
        {
          f->e.eax = err_no;
          f->e.eflags |= FLAG_C;
        }
      break;

    case 0x3e:

      /* Close handle. */

      f->e.eax = do_close (f->e.ebx);
      if (f->e.eax != 0)
        f->e.eflags |= FLAG_C;
      break;

    case 0x3f:

      /* Read handle. */

      f->e.eax = do_read (f->e.ebx, (void *)f->e.edx, f->e.ecx, &err_no);
      if (err_no != 0)
        {
          f->e.eax = err_no;
          f->e.eflags |= FLAG_C;
        }
      break;

    case 0x40:

      /* Write handle. */

      f->e.eax = do_write (f->e.ebx, (void *)f->e.edx, f->e.ecx, &err_no);
      if (err_no != 0)
        {
          f->e.eax = err_no;
          f->e.eflags |= FLAG_C;
        }
      break;

    case 0x41:

      /* Delete directory entry. */

      f->e.eax = do_delete ((const char *)f->e.edx);
      if (f->e.eax != 0)
        f->e.eflags |= FLAG_C;
      break;

    case 0x42:

      /* Move file pointer. */

      f->e.eax = do_seek (f->e.ebx, f->b.al, f->e.edx, &err_no);
      if (err_no != 0)
        {
          f->e.eax = err_no;
          f->e.eflags |= FLAG_C;
        }
      break;

    case 0x43:

      /* Get/set file attributes. */

      if (f->b.al == 0)
        {
          f->e.ecx = get_attr ((const char *)f->e.edx, &err_no);
          f->e.eax = err_no;
          if (err_no != 0)
            f->e.eflags |= FLAG_C;
        }
      else if (f->b.al == 1)
        {
          f->e.eax = set_attr ((const char *)f->e.edx, f->e.ecx);
          if (f->e.eax != 0)
            f->e.eflags |= FLAG_C;
        }
      else
        bad_func (f);
      break;

    case 0x44:

      /* IOCTL */

      if (f->b.al != 0)
        bad_func (f);
      f->e.edx = do_ioctl1 (f->e.ebx, (int *)&f->e.eax);
      if (f->e.eax != 0)
        f->e.eflags |= FLAG_C;
      break;

    case 0x45:

      /* Duplicate file handle. */

      f->e.eax = do_dup (f->e.ebx, (ULONG)(-1), &err_no);
      if (err_no != 0)
        {
          f->e.eax = err_no;
          f->e.eflags |= FLAG_C;
        }
      break;

    case 0x46:

      /* Force duplicate file handle. */

      f->e.eax = do_dup (f->e.ebx, f->e.ecx, &err_no);
      if (err_no != 0)
        {
          f->e.eax = err_no;
          f->e.eflags |= FLAG_C;
        }
      break;

    case 0x47:

      /* Get current directory. */

      f->e.eax = do_getcwd ((char *)f->e.esi, f->b.dl);
      if (f->e.eax != 0)
        f->e.eflags |= FLAG_C;
      break;

    case 0x4c:

      /* End process. */

      quit (f->b.al);
      break;

    case 0x4e:

      f->e.eax = do_find_first ((const char *)f->e.edx, f->e.ecx & 0x37,
                                (struct _find *)f->e.esi);
      if (f->e.eax != 0)
        f->e.eflags |= FLAG_C;
      break;

    case 0x4f:

      f->e.eax = do_find_next ((struct _find *)f->e.esi);
      if (f->e.eax != 0)
        f->e.eflags |= FLAG_C;
      break;

    case 0x56:

      /* Change directory entry. */

      f->e.eax = do_rename ((const char *)f->e.edx, (const char *)f->e.edi);
      if (f->e.eax != 0)
        f->e.eflags |= FLAG_C;
      break;

    case 0x57:

      /* Get/set time/date of file. */

      if (f->b.al == 0)
        {
          f->e.eax = do_get_timestamp (f->e.ebx, &f->e.ecx, &f->e.edx);
          if (f->e.eax != 0)
            f->e.eflags |= FLAG_C;
        }
      else if (f->b.al == 1)
        {
          f->e.eax = do_set_timestamp (f->e.ebx, f->e.ecx, f->e.edx);
          if (f->e.eax != 0)
            f->e.eflags |= FLAG_C;
        }
      else
        {
          f->e.eax = EINVAL;
          f->e.eflags |= FLAG_C;
        }
      break;

    default:
      bad_func (f);
    }
}


static void bad_func (syscall_frame *f)
{
  oprintf ("Invalid syscall function code: %.2x\r\n", (unsigned)f->b.ah);
  quit (255);
}


/* Get date. */

static void do_get_date (syscall_frame *f)
{
  DATETIME dt;

  DosGetDateTime (&dt);
  f->x.cx = dt.year;
  f->b.dh = dt.month;
  f->b.dl = dt.day;
  f->b.al = dt.weekday;
}


/* Get time. */

static void do_get_time (syscall_frame *f)
{
  DATETIME dt;

  DosGetDateTime (&dt);
  f->b.ch = dt.hours;
  f->b.cl = dt.minutes;
  f->b.dh = dt.seconds;
  f->b.dl = dt.hundredths;
}


/* Get version number. */

static void do_get_version (syscall_frame *f)
{
  f->e.eax = ((version_major & 0xff) | ((version_minor & 0xff) << 8)
              | 0x6d650000);
  f->x.bx = 0;
  f->x.cx = 0;
}
