/*
    wscanf.c -- copied from scanf.c
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

int _STD(wscanf) (const wchar_t *format, ...)
{
  va_list arg_ptr;
  int result;

  va_start (arg_ptr, format);
  STREAM_LOCK (stdin);
  result = _winput (stdin, format, arg_ptr);
  STREAM_UNLOCK (stdin);
  va_end (arg_ptr);
  return result;
}
