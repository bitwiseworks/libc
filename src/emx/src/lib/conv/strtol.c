/* strtol.c (emx+gcc) -- Copyright (c) 1990-2001 by Eberhard Mattes */

#include "libc-alias.h"
#include <stdlib.h>
#include <limits.h>
#include <ctype.h>
#include <errno.h>

#ifdef LONG_LONG
#define UTYPE   unsigned long long
#ifdef UNSIGNED
#define NAME    _STD(strtoull)
#define TYPE    unsigned long long
#define MAX     ULLONG_MAX
#else
#define NAME    _STD(strtoll)
#define TYPE    long long
#define MAX     LLONG_MAX
#define MIN     LLONG_MIN
#endif
#else
#define UTYPE   unsigned long
#ifdef UNSIGNED
#define NAME    _STD(strtoul)
#define TYPE    unsigned long
#define MAX     ULONG_MAX
#else
#define NAME    _STD(strtol)
#define TYPE    long
#define MAX     LONG_MAX
#define MIN     LONG_MIN
#endif
#endif

TYPE NAME (const char *string, char **end_ptr, int radix)
{
  const unsigned char *s;
  char neg, pfx;
  TYPE result;

  s = (const unsigned char *)string;
  while (isspace (*s))
    ++s;

  neg = 0; pfx = 0;
  if (*s == '-')
    {
      neg = 1; ++s;
    }
  else if (*s == '+')
    ++s;

  if ((radix == 0 || radix == 16) && s[0] == '0'
      && (s[1] == 'x' || s[1] == 'X'))
    {
      radix = 16; s += 2; pfx = 1;
    }
  if (radix == 0)
    radix = (s[0] == '0' ? 8 : 10);

  result = 0;                   /* Keep the compiler happy */
  if (radix >= 2 && radix <= 36)
    {
      UTYPE n, max1, max2, lim;
      enum {no_number, ok, overflow} state;
      unsigned char c;

#ifdef UNSIGNED
      lim = MAX;
#else
      lim = (neg ? -(UTYPE)MIN : MAX);
#endif
      max1 = lim / radix;
      max2 = lim - max1 * radix;
      n = 0; state = no_number;
      for (;;)
        {
          c = *s;
          if (c >= '0' && c <= '9')
            c = c - '0';
          else if (c >= 'A' && c <= 'Z')
            c = c - 'A' + 10;
          else if (c >= 'a' && c <= 'z')
            c = c - 'a' + 10;
          else
            break;
          if (c >= radix)
            break;
          if (n >= max1 && (n > max1 || (UTYPE)c > max2))
            state = overflow;
          if (state != overflow)
            {
              n = n * radix + (UTYPE)c;
              state = ok;
            }
          ++s;
        }
      switch (state)
        {
        case no_number:
          result = 0;
          s = (const unsigned char *)string;
          if (pfx)
            ++s;                /* At least eat the 0 of "0x" */
          /* Don't set errno!? */
          break;
        case ok:
          result = (neg ? -n : n);
          break;
        case overflow:
#ifdef UNSIGNED
          result = MAX;
#else
          result = (neg ? MIN : MAX);
#endif
          errno = ERANGE;
          break;
        }
    }
  else
    {
      result = 0;
      errno = EDOM;
    }
  if (end_ptr != NULL)
    *end_ptr = (char *)s;
  return result;
}
