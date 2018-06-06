/* sys/fsync.c (emx+gcc) -- Copyright (c) 1992-1996 by Eberhard Mattes */

#include "libc-alias.h"
#define INCL_FSMACROS
#include <os2emx.h>
#include <unistd.h>
#include <emx/syscalls.h>
#include "syscalls.h"

int _STD(fsync) (int handle)
{
  ULONG rc;
  FS_VAR();

  FS_SAVE_LOAD();
  rc = DosResetBuffer (handle);
  FS_RESTORE();
  if (rc != 0)
    {
      _sys_set_errno (rc);
      return -1;
    }
  return 0;
}
