/* gcvt.c (emx+gcc) -- Copyright (c) 1994-1996 by Eberhard Mattes */

#include "libc-alias.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>
#include <locale.h>
#include <emx/float.h>
#include <InnoTekLIBC/locale.h>

char *_STD(gcvt) (double x, int digits, char *buffer)
{
  char digstr[DECIMAL_DIG+2], *str, *dst;
  const char *src;
  int xp, i, fpclass, neg;

  dst = buffer;

  /* Handle trivial cases first.  The empty string is produced if
     DIGITS is less than one. */

  if (digits < 1)
    {
      *dst = 0;
      return buffer;
    }

  /* Classify the number and get the sign bit. */

  fpclass = fpclassify (x);
  neg = signbit (x);
  switch (fpclass)
    {
    case FP_ZERO:
      /* "0" is produced if X is zero (the code below doesn't work if
         X is zero). */
      src = "0";
      break;
    case FP_NAN:
      src = "NAN";
      neg = 0;                  /* Don't print -NAN */
      break;
    case FP_INFINITE:
      src = "INF";
      break;
    default:
      src = NULL;
      break;
    }

  /* Handle the sign bit. */

  if (neg)
    *dst++ = '-';

  /* Handle special cases. */

  if (src != NULL)
    {
      strcpy (dst, src);
      return buffer;
    }

  /* Make the number positive. */

  if (neg)
    x = -x;

  /* Compute a string of digits and an exponent for the number. */

  str = __legacy_dtoa (digstr, &xp, x, digits, DTOA_GCVT, DBL_DIG);
  __remove_zeros (str, 1);

  /* Decide whether to use fixed or exponential format. */

  if (xp < -1 || xp >= digits)
    {
      /* Exponential format: #.####e+## */

      *dst++ = str[0];
      if (str[1] != 0)
        *dst++ = __libc_gLocaleLconv.s.decimal_point[0];
      for (src = str+1; *src != 0; ++src)
        *dst++ = *src;
      *dst++ = 'e';
      if (xp < 0)
        {
          *dst++ = '-';
          xp = -xp;
        }
      else
        *dst++ = '+';

      /* Use at least two digits for the exponent. */

      if (xp < 10)
        *dst++ = '0';
      _itoa (xp, dst, 10);
    }
  else
    {
      /* Fixed format: ####.#### */

      if (xp == -1)
        *dst++ = '0';

      /* Take digits from the string. */

      for (i = 0; str[i] != 0; ++i)
        {
          if (i == xp + 1)
            *dst++ = __libc_gLocaleLconv.s.decimal_point[0];
          *dst++ = str[i];
        }

      /* Append zeros until the exponent is correct. */

      for (; i <= xp; ++i)
        *dst++ = '0';

      *dst = 0;
    }
  return buffer;
}
