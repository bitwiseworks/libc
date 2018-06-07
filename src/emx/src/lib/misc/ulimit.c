/* ulimit.c (emx+gcc) -- Copyright (c) 1990-1996 by Eberhard Mattes */

#include "libc-alias.h"
#include <stdarg.h>
#include <stdlib.h>
#include <ulimit.h>
#include <errno.h>
#include <emx/syscalls.h>

long _STD(ulimit) (int cmd, ...)
{
  va_list arg_ptr;
  long newlimit;

  switch (cmd)
    {
    case UL_GFILLIM:
      return 1 << 21;
    case UL_SFILLIM:
      va_start (arg_ptr, cmd);
      newlimit = va_arg (arg_ptr, long);
      va_end (arg_ptr);
      return newlimit;
    case UL_GMEMLIM:
      return __ulimit (cmd, 0);
    case UL_NOFILES:
      return 40;
    case UL_OBJREST:
      return __ulimit (cmd, 0);
    default:
      errno = EINVAL;
      return -1;
    }
}
