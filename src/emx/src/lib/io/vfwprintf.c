/*
    vfwprintf.c -- copied from vfprintf.c
    Copyright (c) 2020 bww bitwiseworks GmbH
*/

#include "libc-alias.h"
#include <sys/builtin.h>        /* For <sys/fmutex.h> */
#include <sys/fmutex.h>
#include <stdio.h>
#include <stdlib.h>
#include <emx/io.h>
#include <wchar.h>

int _STD(vfwprintf) (FILE *stream, const wchar_t *format, va_list arg_ptr)
{
  int result;
  void *tb;

  STREAM_LOCK (stream);
  if (_nbuf (stream))
    _fbuf (stream);
  _tmpbuf (stream, tb);
  result = _woutput (stream, format, arg_ptr);
  if (_endbuf (stream) != 0)
    result = -1;
  STREAM_UNLOCK (stream);
  return result;
}
