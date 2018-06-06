/*
    Integer to ASCII conversion routines.
    Copyright (c) 2003 InnoTek Systemberatung GmbH

    For conditions of distribution and use, see the file COPYING.

    unsigned long (AKA unsigned int) -> ascii conversion routine.
*/

#include "libc-alias.h"
#include <stdlib.h>
#include <386/builtin.h>

extern const char *__digits;

char *_STD(ultoa) (unsigned long value, char *string, int radix)
{
  int i;
  char *dst;
  char digits[32];
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
    value = __uldivmod (value, radix, &rem);
    digits [i++] = __digits [rem];
  } while (value);

  while (i > 0)
    *dst++ = digits[--i];

  *dst = 0;
  return string;
}
