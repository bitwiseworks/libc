/*
    Integer to ASCII conversion routines.
    Copyright (c) 2003 InnoTek Systemberatung GmbH

    For conditions of distribution and use, see the file COPYING.

    long long -> ascii conversion routine.
*/

#include "libc-alias.h"
#include <stdlib.h>
#include <386/builtin.h>

extern const char *__digits;

char *_STD(lltoa) (long long value, char *string, int radix)
{
  char *dst;
  char digits[64];
  unsigned long long x;
  int i;
  long rem;

  dst = string;
  if (radix < 2 || radix > 36)
  {
    *dst = 0;
    return string;
  }
  if ((value < 0) && (radix == 10))
  {
    *dst++ = '-';
    x = -value;
  }
  else
    x = value;

  i = 0;
  do
  {
    x = __ulldivmod (x, radix, &rem);
    digits [i++] = __digits [rem];
  } while (x != 0);

  while (i > 0)
    *dst++ = digits[--i];

  *dst = 0;
  return string;
}
