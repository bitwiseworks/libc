/* sys/read_kbd.c (emx+gcc) -- Copyright (c) 1993-1996 by Eberhard Mattes */

#include "libc-alias.h"
#define INCL_DOSFILEMGR
#define INCL_KBD
#define INCL_FSMACROS
#include <os2emx.h>
#include <emx/syscalls.h>
#include "syscalls.h"

static KBDINFO info;
static KBDKEYINFO key;

int __read_kbd (int echo, int wait, int sig)
{
  USHORT info_rc, rc;
  USHORT info_mask;
  ULONG n;
  int ret;
  char c;
  static int more = -1;
  FS_VAR();

  if (more >= 0)
    {
      ret = more;
      more = -1;
      return ret;
    }
  FS_SAVE_LOAD();
  info_mask = 0;                /* Keep the compiler happy */
  info.cb = sizeof (info);
  info_rc = KbdGetStatus (&info, 0);
  if (info_rc == 0)
    {
      info_mask = info.fsMask;
      info.fsMask &= ~0x7f;
      if (sig)
        info.fsMask |= KEYBOARD_ASCII_MODE;
      else
        info.fsMask |= KEYBOARD_BINARY_MODE;
      info.cb = sizeof (info);
      KbdSetStatus (&info, 0);
    }

again:
  rc = KbdCharIn (&key, (wait ? IO_WAIT : IO_NOWAIT), 0);
  if (rc != 0)
    ret = -1;
  else if (!(key.fbStatus & KBDTRF_FINAL_CHAR_IN))
    {
      if (wait)
        goto again;
      ret = -1;
    }
  else if ((key.chChar == 0 || key.chChar == 0xe0)
           && key.fbStatus & KBDTRF_EXTENDED_CODE)
    {
      more = key.chScan;
      ret = 0;
    }
  else
    {
      ret = key.chChar;
      if (echo)
        {
          c = (char)ret;
          DosWrite (1, &c, 1, &n);
        }
    }

  if (info_rc == 0)
    {
      info.cb = sizeof (info);
      info.fsMask = info_mask;
      KbdSetStatus (&info, 0);
    }

  FS_RESTORE();
  return ret;
}
