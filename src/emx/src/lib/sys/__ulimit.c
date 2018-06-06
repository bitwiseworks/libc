/* sys/ulimit.c (emx+gcc) -- Copyright (c) 1992-1996 by Eberhard Mattes */

#include "libc-alias.h"
#include <ulimit.h>
#include <errno.h>
#include <emx/syscalls.h>
#include "syscalls.h"

long __ulimit (int cmd, long new_limit)
{
  switch (cmd)
    {
    case UL_GMEMLIM:
      /* Note: As this is the limit for brk(), not for sbrk(), we
         don't return 512M. */
      return _sys_top_heap_obj != NULL ? _sys_top_heap_obj->end : 0;

    case UL_OBJREST:
      return (_sys_top_heap_obj != NULL
              ? _sys_top_heap_obj->end - _sys_top_heap_obj->brk : 0);

    default:
      errno = EINVAL;
      return -1;
    }
}
