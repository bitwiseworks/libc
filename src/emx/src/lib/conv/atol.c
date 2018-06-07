/* atol.c (emx+gcc) -- Copyright (c) 1990-1993 by Eberhard Mattes */

#include "libc-alias.h"
#include <stdlib.h>

long _STD(atol) (const char *string)
{
  long n;
  char neg;
  
  n = 0; neg = 0;
  while (*string == ' ' || *string == '\t') ++string;
  if (*string == '+')
    ++string;
  else if (*string == '-')
    {
      ++string; neg = 1;
    }
  while (*string >= '0' && *string <= '9')
    {
      n = n * 10 + (*string - '0');
      ++string;
    }
  return (neg ? -n : n);
}
