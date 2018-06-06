/* vfprintf.c (emx+gcc) -- Copyright (c) 1990-1996 by Eberhard Mattes */

#include "libc-alias.h"
#include <sys/builtin.h>        /* For <sys/fmutex.h> */
#include <sys/fmutex.h>
#include <stdio.h>
#include <stdlib.h>
#include <emx/io.h>

int _STD(vfprintf) (FILE *stream, const char *format, va_list arg_ptr)
{
  int result;
  void *tb;

  STREAM_LOCK (stream);
  if (nbuf (stream))
    _fbuf (stream);
  _tmpbuf (stream, tb);
  result = _output (stream, format, arg_ptr);
  if (_endbuf (stream) != 0)
    result = -1;
  STREAM_UNLOCK (stream);
  return result;
}
