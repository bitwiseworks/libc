/* fscanf.c (emx+gcc) -- Copyright (c) 1990-1996 by Eberhard Mattes */

#include "libc-alias.h"
#include <sys/builtin.h>        /* For <sys/fmutex.h> */
#include <sys/fmutex.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <emx/io.h>

int _STD(fscanf) (FILE *stream, const char *format, ...)
{
  va_list arg_ptr;
  int result;

  va_start (arg_ptr, format);
  STREAM_LOCK (stream);
  result = _input (stream, format, arg_ptr);
  STREAM_UNLOCK (stream);
  va_end (arg_ptr);
  return result;
}
