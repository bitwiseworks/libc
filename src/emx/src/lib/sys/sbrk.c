/* sys/sbrk.c (emx+gcc) -- Copyright (c) 1992-1996 by Eberhard Mattes */

#include "libc-alias.h"
#include <os2emx.h>
#include <errno.h>
#include <sys/builtin.h>
#include <sys/fmutex.h>
#include <sys/uflags.h>
#include <sys/types.h>
#include <stdlib.h>
#include <emx/syscalls.h>
#include "syscalls.h"

void *_STD(sbrk) (intptr_t incr)
{
  ULONG base;

  if (_fmutex_request (&_sys_heap_fmutex, _FMR_IGNINT) != 0)
    return (void *)-1;

  if (incr >= 0)
    base = _sys_expand_heap_by (incr, _sys_uflags & _UF_SBRK_MODEL);
  else
    base = _sys_shrink_heap_by (-incr, _sys_uflags & _UF_SBRK_MODEL);

  if (_fmutex_release (&_sys_heap_fmutex) != 0)
    return (void *)-1;

  if (base == 0)
    {
      errno = ENOMEM;
      return (void *)-1;
    }
  return (void *)base;
}
