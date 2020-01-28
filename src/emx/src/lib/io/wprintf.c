/*
    wprintf.c -- copied from printf.c
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

int _STD(wprintf) (const wchar_t *format, ...)
{
  va_list arg_ptr;
  int result;
  void *tb;

  va_start (arg_ptr, format);
  STREAM_LOCK (stdout);
  if (nbuf (stdout))
    _fbuf (stdout);
  _tmpbuf (stdout, tb);
  result = _woutput (stdout, format, arg_ptr);
  if (_endbuf (stdout) != 0)
    result = -1;
  STREAM_UNLOCK (stdout);
  va_end (arg_ptr);
  return result;
}
