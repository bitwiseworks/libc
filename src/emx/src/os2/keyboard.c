/* keyboard.c -- Read characters from the keyboard
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


#define INCL_KBD
#include <os2emx.h>
#include "emxdll.h"
#include "clib.h"


static UCHAR kbd_input_more;
static UCHAR kbd_input_next;
static KBDINFO kbd_info;        /* Must not cross a 64K boundary */
static KBDKEYINFO kbd_inp;      /* Must not cross a 64K boundary */
static UCHAR cgets_buf[256];    /* Must not cross a 64K boundary */


#define MASK1 (KEYBOARD_BINARY_MODE | KEYBOARD_ASCII_MODE)
#define MASK2 (MASK1 | KEYBOARD_ECHO_ON | KEYBOARD_ECHO_OFF)

int kbd_input (UCHAR *dst, int binary_p, int echo_p, int wait_p,
               int check_avail_p)
{
  int mask, old_mask = 0, status_changed, result;

  if (kbd_input_more)
    {
      if (check_avail_p)
        return TRUE;
      kbd_input_more = FALSE;
      if (dst != NULL) *dst = kbd_input_next;
      return TRUE;
    }

  kbd_info.cb = sizeof (kbd_info);
  status_changed = FALSE;
  if (KbdGetStatus (&kbd_info, 0) == 0)
    {
      old_mask = kbd_info.fsMask;
      mask = (binary_p ? KEYBOARD_BINARY_MODE : KEYBOARD_ASCII_MODE);
      if ((old_mask & MASK1) != mask)
        {
          status_changed = TRUE;
          kbd_info.fsMask = (kbd_info.fsMask & ~MASK2) | mask;
          kbd_info.cb = sizeof (kbd_info);
          KbdSetStatus (&kbd_info, 0);
        }
    }

  do
    {
      if (check_avail_p)
        KbdPeek (&kbd_inp, 0);
      else
        KbdCharIn (&kbd_inp, (wait_p ? IO_WAIT : IO_NOWAIT), 0);
        break;
    } while (!(check_avail_p || !wait_p
               || (kbd_inp.fbStatus & KBDTRF_FINAL_CHAR_IN)));

  if (kbd_inp.fbStatus & KBDTRF_FINAL_CHAR_IN)
    {
      result = TRUE;
      if ((kbd_inp.chChar == 0 || kbd_inp.chChar == 0xe0)
          && (kbd_inp.fbStatus & KBDTRF_EXTENDED_CODE))
        {
          kbd_inp.chChar = 0;   /* Override 0xe0 */
          kbd_input_next = kbd_inp.chScan;
          kbd_input_more = TRUE;
        }
      else if (echo_p)
        {
          UCHAR c;
          ULONG nwritten;

          c = kbd_inp.chChar;
          DosWrite (1, &c, 1, &nwritten);
        }
      if (dst != NULL) *dst = kbd_inp.chChar;
    }
  else
    result = FALSE;

  if (status_changed)
    {
      kbd_info.cb = sizeof (kbd_info);
      if (KbdGetStatus (&kbd_info, 0) == 0)
        {
          kbd_info.fsMask = (kbd_info.fsMask & ~MASK2) | (old_mask & MASK1);
          kbd_info.cb = sizeof (kbd_info);
          KbdSetStatus (&kbd_info, 0);
        }
    }

  return result;
}


void conin_string (UCHAR *buf)
{
  int i, buf_len;
  STRINGINBUF sib;
  USHORT rc;

  buf_len = buf[0];
  memcpy (cgets_buf, buf + 2, buf_len);
  for (i = 0; i < buf_len; ++i)
    if (cgets_buf[i] == 0 || cgets_buf[i] == '\r')
      break;
  if (i >= buf_len)
    i = 0;
  cgets_buf[i] = '\r';
  sib.cchIn = (USHORT)i;
  sib.cb = (USHORT)buf_len;
  rc = KbdStringIn (cgets_buf, &sib, IO_WAIT, 0);
  i = (rc == 0 ? sib.cchIn : 0);
  buf[1] = (UCHAR)i;
  memcpy (buf + 2, cgets_buf, buf_len);
  buf[i + 2] = '\r';
}
