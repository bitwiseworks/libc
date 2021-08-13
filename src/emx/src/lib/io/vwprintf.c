/*
    vwprintf.c -- copied from vprintf.c
    Copyright (c) 2020 bww bitwiseworks GmbH
*/

#include "libc-alias.h"
#include <sys/builtin.h>        /* For <sys/fmutex.h> */
#include <sys/fmutex.h>
#include <stdio.h>
#include <stdlib.h>
#include <emx/io.h>
#include <wchar.h>

int _STD(vwprintf) (const wchar_t *format, va_list arg_ptr)
{
  int result;
  void *tb;

  STREAM_LOCK (stdout);
  if (_nbuf (stdout))
    _fbuf (stdout);
  _tmpbuf (stdout, tb);
  result = _woutput (stdout, format, arg_ptr);
  if (_endbuf (stdout) != 0)
    result = -1;
  STREAM_UNLOCK (stdout);
  return result;
}
