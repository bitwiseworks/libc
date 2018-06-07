/* sprintf.c (emx+gcc) -- Copyright (c) 1990-1996 by Eberhard Mattes */

#include "libc-alias.h"
#include <sys/builtin.h>        /* For <sys/fmutex.h> */
#include <sys/fmutex.h>
#include <stdio.h>
#include <stdio.h>
#include <stdarg.h>
#include <limits.h>
#include <emx/io.h>

int _STD(sprintf) (char *buffer, const char *format, ...)
{
  va_list arg_ptr;
  FILE trick;
  int result;

  va_start (arg_ptr, format);
  trick.__uVersion = _FILE_STDIO_VERSION;
  trick._buffer = buffer;
  trick._ptr = buffer;
  trick._rcount = 0;
  trick._wcount = INT_MAX;
  trick._handle = -1;
  trick._flags = _IOOPEN|_IOSPECIAL|_IOBUFUSER|_IOWRT;
  trick._buf_size = INT_MAX;
  trick._flush = NULL;
  trick._ungetc_count = 0;
  trick._mbstate = 0;
  _fmutex_dummy(&trick.__u.__fsem);
  result = _output (&trick, format, arg_ptr);
  putc (0, &trick);
  va_end (arg_ptr);
  return result;
}
