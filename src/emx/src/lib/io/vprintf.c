/* vprintf.c (emx+gcc) -- Copyright (c) 1990-1996 by Eberhard Mattes */

#include "libc-alias.h"
#include <sys/builtin.h>        /* For <sys/fmutex.h> */
#include <sys/fmutex.h>
#include <stdio.h>
#include <stdlib.h>
#include <emx/io.h>

int _STD(vprintf) (const char *format, va_list arg_ptr)
{
  int result;
  void *tb;

  STREAM_LOCK (stdout);
  if (_nbuf (stdout))
    _fbuf (stdout);
  _tmpbuf (stdout, tb);
  result = _output (stdout, format, arg_ptr);
  if (_endbuf (stdout) != 0)
    result = -1;
  STREAM_UNLOCK (stdout);
  return result;
}
