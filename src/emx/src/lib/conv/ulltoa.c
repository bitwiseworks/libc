/*
    Integer to ASCII conversion routines.
    Copyright (c) 2003 InnoTek Systemberatung GmbH

    For conditions of distribution and use, see the file COPYING.

    unsigned long long -> ascii conversion routine.
*/

#include "libc-alias.h"
#include <stdlib.h>
#include <386/builtin.h>

extern const char *__digits;

char *_STD(ulltoa) (unsigned long long value, char *string, int radix)
{
  char *dst;
  char digits[64];
  int i;
  unsigned long rem;

  dst = string;
  if (radix < 2 || radix > 36)
  {
    *dst = 0;
    return string;
  }

  i = 0;
  do
  {
    value = __ulldivmod (value, radix, &rem);
    digits [i++] = __digits [rem];
  } while (value != 0);

  while (i > 0)
    *dst++ = digits[--i];

  *dst = 0;
  return string;
}
