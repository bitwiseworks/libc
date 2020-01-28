/*
    swprintf.c -- copied from snprintf.c
    Copyright (c) 2020 bww bitwiseworks GmbH
*/

#include "libc-alias.h"
#include <sys/builtin.h>        /* For <sys/fmutex.h> */
#include <sys/fmutex.h>
#include <stdio.h>
#include <stdio.h>
#include <stdarg.h>
#include <limits.h>
#include <emx/io.h>
#include <wchar.h>
#include <errno.h>

int _STD(swprintf) (wchar_t *buffer, size_t n, const wchar_t *format, ...)
{
  va_list arg_ptr;
  FILE trick;
  int result;

  if (n > INT_MAX)
  {
    errno = EOVERFLOW;
    return EOF;
  }
  va_start (arg_ptr, format);
  trick.__uVersion = _FILE_STDIO_VERSION;
  trick._buffer = (char *) buffer;
  trick._ptr = (char *) buffer;
  trick._rcount = 0;
  trick._wcount = n != 0 ? (int)n - 1 : 0;
  trick._handle = -1;
  trick._flags = _IOOPEN|_IOSPECIAL|_IOBUFUSER|_IOWRT|_IOWIDE;
  trick._buf_size = (int)n;
  trick._flush = NULL;
  trick._ungetc_count = 0;
  trick._mbstate = 0;
  _fmutex_dummy(&trick.__u.__fsem);
  result = _woutput (&trick, format, arg_ptr);
  if (n != 0)
    *((wchar_t *)trick._ptr) = '\0';
  va_end (arg_ptr);
  /* As opposed to snprintf, POSIX requires the wide version to return -1
     on buffer overflow and set errno. */
  if (result >= n)
  {
    errno = EOVERFLOW;
    return -1;
  }
  return result;
}
