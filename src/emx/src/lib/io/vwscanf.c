/*
    vwscanf.c -- copied from vscanf.c
    Copyright (c) 2020 bww bitwise works GmbH
*/

#include "libc-alias.h"
#include <sys/builtin.h>        /* For <sys/fmutex.h> */
#include <sys/fmutex.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <emx/io.h>
#include <wchar.h>

int _STD(vwscanf) (const wchar_t *format, va_list arg_ptr)
{
  int r;

  STREAM_LOCK (stdin);
  r = _winput (stdin, format, arg_ptr);
  STREAM_UNLOCK (stdin);
  return r;
}
