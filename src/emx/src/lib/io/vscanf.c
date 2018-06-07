/* vscanf.c (emx+gcc) -- Copyright (c) 1990-1996 by Eberhard Mattes */

#include "libc-alias.h"
#include <sys/builtin.h>        /* For <sys/fmutex.h> */
#include <sys/fmutex.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <emx/io.h>

int _STD(vscanf) (const char *format, va_list arg_ptr)
{
  int r;

  STREAM_LOCK (stdin);
  r = _input (stdin, format, arg_ptr);
  STREAM_UNLOCK (stdin);
  return r;
}
