/* sys/execname.c (emx+gcc) -- Copyright (c) 1993-1996 by Eberhard Mattes */

#include "libc-alias.h"
#define INCL_DOS
#define INCL_FSMACROS
#include <os2emx.h>
#include <stdlib.h>
#include <string.h>
#include <emx/syscalls.h>
#include <InnoTekLIBC/FastInfoBlocks.h>
#include "syscalls.h"

int _execname (char *dst, size_t size)
{
  ULONG rc;
  FS_VAR();

  if (size == 0)
    return -1;

  FS_SAVE_LOAD();
  rc = DosQueryModuleName (fibGetExeHandle(), size, dst);
  FS_RESTORE();
  if (rc != 0)
    {
      *dst = 0;
      return -1;
    }
  return 0;
}
