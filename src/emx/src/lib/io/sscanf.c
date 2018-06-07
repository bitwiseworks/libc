/* sscanf.c (emx+gcc) -- Copyright (c) 1990-1996 by Eberhard Mattes */

#include "libc-alias.h"
#include <sys/builtin.h>        /* For <sys/fmutex.h> */
#include <sys/fmutex.h>
#include <stdio.h>
#include <stdio.h>
#include <stdarg.h>
#include <limits.h>
#include <string.h>
#include <emx/io.h>

int _STD(sscanf) (const char *buffer, const char *format, ...)
{
  va_list arg_ptr;
  FILE trick;
  int result;

  va_start (arg_ptr, format);
  trick.__uVersion = _FILE_STDIO_VERSION;
  trick._buffer = (char *)buffer;               /* const -> non-const */
  trick._ptr = (char *)buffer;                  /* const -> non-const */
  trick._rcount = strlen (buffer);
  trick._wcount = 0;
  trick._handle = -1;
  trick._flags = _IOOPEN|_IOSPECIAL|_IOBUFUSER|_IOREAD;
  trick._buf_size = INT_MAX;
  trick._flush = NULL;
  trick._ungetc_count = 0;
  trick._mbstate = 0;
  _fmutex_dummy(&trick.__u.__fsem);
  result = _input (&trick, format, arg_ptr);
  va_end (arg_ptr);
  return result;
}
