/* sys/brk.c (emx+gcc) -- Copyright (c) 1992-1996 by Eberhard Mattes */

#include "libc-alias.h"
#include <os2emx.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/builtin.h>
#include <sys/fmutex.h>
#include <emx/syscalls.h>
#include "syscalls.h"

int _STD(brk) (const void *brkp)
{
  ULONG base;

  if (_fmutex_request (&_sys_heap_fmutex, _FMR_IGNINT) != 0)
    return -1;

  if (_sys_heap_obj_count == 0)
    base = 0;
  else if ((ULONG)brkp >= _sys_top_heap_obj->brk
           && (ULONG)brkp <= _sys_top_heap_obj->end)
    base = _sys_expand_heap_obj_by ((ULONG)brkp - _sys_top_heap_obj->brk);
  else if ((ULONG)brkp >= _sys_top_heap_obj->base
           && (ULONG)brkp < _sys_top_heap_obj->brk)
    base = _sys_shrink_heap_obj_by (_sys_top_heap_obj->brk - (ULONG)brkp);
  else
    base = _sys_shrink_heap_to ((ULONG)brkp);

  if (_fmutex_release (&_sys_heap_fmutex) != 0)
    return -1;

  if (base == 0)
    {
      errno = ENOMEM;
      return -1;
    }
  return 0;
}
