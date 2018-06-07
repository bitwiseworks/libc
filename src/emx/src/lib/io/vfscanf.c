/* vfscanf.c (emx+gcc) -- Copyright (c) 1990-1996 by Eberhard Mattes */

#include "libc-alias.h"
#include <sys/builtin.h>        /* For <sys/fmutex.h> */
#include <sys/fmutex.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <emx/io.h>

int _STD(vfscanf) (FILE *stream, const char *format, va_list arg_ptr)
{
  int r;

  STREAM_LOCK (stream);
  r = _input (stream, format, arg_ptr);
  STREAM_UNLOCK (stream);
  return r;
}
