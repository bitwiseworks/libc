/* sys/sleep2.c (emx+gcc) -- Copyright (c) 1993-1996 by Eberhard Mattes */

#include "libc-alias.h"
#define INCL_DOSPROCESS
#define INCL_FSMACROS
#include <os2emx.h>
#include <stdlib.h>
#include <emx/syscalls.h>
#include "syscalls.h"

unsigned _sleep2 (unsigned millisec)
{
  FS_VAR();

  FS_SAVE_LOAD();
  DosSleep (millisec);
  FS_RESTORE();
  return 0;
}
