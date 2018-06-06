/* snprintf.c (emx+gcc) -- Copyright (c) 1995-1998 by Eberhard Mattes */

#include "libc-alias.h"
#include <sys/builtin.h>        /* For <sys/fmutex.h> */
#include <sys/fmutex.h>
#include <stdio.h>
#include <stdio.h>
#include <stdarg.h>
#include <limits.h>
#include <emx/io.h>

int _STD(snprintf) (char *buffer, size_t n, const char *format, ...)
{
  va_list arg_ptr;
  FILE trick;
  int result;

  if (n > INT_MAX)
    return EOF;
  va_start (arg_ptr, format);
  trick.__uVersion = _FILE_STDIO_VERSION;
  trick._buffer = buffer;
  trick._ptr = buffer;
  trick._rcount = 0;
  trick._wcount = n != 0 ? (int)n - 1 : 0;
  trick._handle = -1;
  trick._flags = _IOOPEN|_IOSPECIAL|_IOBUFUSER|_IOWRT;
  trick._buf_size = (int)n;
  trick._flush = NULL;
  trick._ungetc_count = 0;
  trick._mbstate = 0;
  _fmutex_dummy(&trick.__u.__fsem);
  result = _output (&trick, format, arg_ptr);
  if (n != 0)
    *trick._ptr = 0;
  va_end (arg_ptr);
  return result;
}
