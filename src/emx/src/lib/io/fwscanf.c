/*
    fwscanf.c -- copied from fscanf.c
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

int _STD(fwscanf) (FILE *stream, const wchar_t *format, ...)
{
  va_list arg_ptr;
  int result;

  va_start (arg_ptr, format);
  STREAM_LOCK (stream);
  result = _winput (stream, format, arg_ptr);
  STREAM_UNLOCK (stream);
  va_end (arg_ptr);
  return result;
}
