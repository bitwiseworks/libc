/* strset.c (emx+gcc) -- Copyright (c) 1990-1995 by Eberhard Mattes */

#include "libc-alias.h"
#include <string.h>

char *_STD(strset) (char *string, int c)
{
  char *dst;

  dst = string;
  while (*dst != 0)
    *dst++ = (char)c;
  return string;
}
