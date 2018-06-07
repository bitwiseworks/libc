/* sys/scrsize.c (emx+gcc) -- Copyright (c) 1992-1996 by Eberhard Mattes */

#include "libc-alias.h"
#define INCL_VIO
#define INCL_FSMACROS
#include <os2emx.h>
#include <stdlib.h>
#include <os2thunk.h>
#include <emx/syscalls.h>

void _scrsize (int *dst)
{
  VIOMODEINFO vmi1, vmi2, *pvmi;
  FS_VAR();

  /* At most one of vmi1, vmi2 crosses a 64Kbyte boundary.  Use one
     which doesn't. */

  pvmi = _THUNK_PTR_STRUCT_OK (&vmi1) ? &vmi1 : &vmi2;
  pvmi->cb = sizeof (*pvmi);
  FS_SAVE_LOAD();
  VioGetMode (pvmi, 0);
  FS_RESTORE();
  dst[0] = pvmi->col;
  dst[1] = pvmi->row;
}
