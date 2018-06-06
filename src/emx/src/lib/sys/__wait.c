/* sys/wait.c (emx+gcc) -- Copyright (c) 1992-1996 by Eberhard Mattes */

#include "libc-alias.h"
#define INCL_DOSPROCESS
#define INCL_FSMACROS
#include <os2emx.h>
#include <emx/syscalls.h>
#include "syscalls.h"

int __wait (int *status)
{
  ULONG rc;
  RESULTCODES res;
  PID pid;
  FS_VAR();

  FS_SAVE_LOAD();
  rc = DosWaitChild (DCWA_PROCESS, DCWW_WAIT, &res, &pid, 0);
  FS_RESTORE();
  if (rc != 0)
    {
      _sys_set_errno (rc);
      return -1;
    }
  switch (res.codeTerminate)
    {
    case TC_EXIT:
      *status = res.codeResult << 8;
      break;
    case TC_HARDERROR:
    case TC_KILLPROCESS:
      *status = SIGTERM;
      break;
    default:
      *status = SIGSEGV;
      break;
    }
  return pid;
}
