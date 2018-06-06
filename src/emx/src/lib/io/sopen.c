/* sopen.c (emx+gcc) -- Copyright (c) 1990-1996 by Eberhard Mattes */

#include "libc-alias.h"
#include <stdarg.h>
#include <io.h>
#include <emx/io.h>

/**
 * @remark O_SIZE means a off_t sized argument not unsigned long as emxlib say.
 */
int _STD(sopen) (const char *name, int oflag, int shflag, ...)
{
  va_list va;
  int h;

  va_start (va, shflag);
  h = _vsopen (name, oflag, shflag, va);
  va_end (va);
  return h;
}
