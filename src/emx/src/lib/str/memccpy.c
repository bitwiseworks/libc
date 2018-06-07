/* memccpy.c (emx+gcc) -- Copyright (c) 1990-1995 by Eberhard Mattes */

#include "libc-alias.h"
#include <string.h>

void *_STD(memccpy) (void *s1, const void *s2, int c, size_t n)
{
  size_t i;

  for (i = 0; i < n; ++i)
    {
      if ((((char *)s1)[i] = ((char *)s2)[i]) == (char)c)
        return (char *)s1+i+1;
    }
  return NULL;
}
