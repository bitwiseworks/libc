/* open.c (emx+gcc) -- Copyright (c) 1990-1996 by Eberhard Mattes */

#include "libc-alias.h"
#include <stdarg.h>
#include <io.h>
#include <share.h>
#include <emx/io.h>

/**
 * @remark O_SIZE means a off_t sized argument not unsigned long as emxlib say.
 */
int _STD(open) (const char *name, int oflag, ...)
{
  va_list va;
  int h;

  va_start (va, oflag);
  h = _vsopen (name, oflag, SH_DENYNO, va);
  va_end (va);
  return h;
}
