/*
    fwprintf.c -- copied from fprintf.c
    Copyright (c) 2020 bww bitwiseworks GmbH
*/

#include "libc-alias.h"
#include <sys/builtin.h>        /* For <sys/fmutex.h> */
#include <sys/fmutex.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <emx/io.h>
#include <wchar.h>

int _STD(fwprintf) (FILE *stream, const wchar_t *format, ...)
{
  va_list arg_ptr;
  int result;
  void *tb;

  va_start (arg_ptr, format);
  STREAM_LOCK (stream);
  if (nbuf (stream))
    _fbuf (stream);
  _tmpbuf (stream, tb);
  result = _woutput (stream, format, arg_ptr);
  if (_endbuf (stream) != 0)
    result = -1;
  STREAM_UNLOCK (stream);
  va_end (arg_ptr);
  return result;
}
