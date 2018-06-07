/* strncat.c (emx+gcc) -- Copyright (c) 1990-1995 by Eberhard Mattes */

#include "libc-alias.h"
#include <string.h>

char *_STD(strncat) (char *string1, const char *string2, size_t count)
{
  char *dst;

  dst = string1;
  while (*dst != 0)
    ++dst;
  while (count > 0 && *string2 != 0)
    {
      *dst++ = *string2++;
      --count;
    }
  *dst = 0;
  return string1;
}
