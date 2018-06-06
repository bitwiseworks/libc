/* strnset.c (emx+gcc) -- Copyright (c) 1990-1995 by Eberhard Mattes */

#include "libc-alias.h"
#include <string.h>

char *_STD(strnset) (char *string, int c, size_t count)
{
  char *dst;

  dst = string;
  while (count > 0 && *dst != 0)
    {
      *dst++ = (char)c;
      --count;
    }
  return string;
}
