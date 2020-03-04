/*
    vfwscanf.c -- copied from vfscanf.c
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

int _STD(vfwscanf) (FILE *stream, const wchar_t *format, va_list arg_ptr)
{
  int r;

  STREAM_LOCK (stream);
  r = _winput (stream, format, arg_ptr);
  STREAM_UNLOCK (stream);
  return r;
}
