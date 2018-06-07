/*
    Locale support implementation through OS/2 Unicode API.
    Copyright (c) 1992-1996 by Eberhard Mattes
    Copyright (c) 2003 InnoTek Systemberatung GmbH

    For conditions of distribution and use, see the file COPYING.

    Format time. The output string is written to the array of size characters
    pointed to by string, including the terminating null character. Like
    sprintf(), strftime() copies the string pointed to by format to the
    array pointed to by string, replacing format specifications with formatted
    data from t. Ordinary characters are copied unmodified.

    On success, strftime() returns the number of characters copied to the
    array pointed to by string, excluding the terminating null character.
    On failure (size exceeded), strftime() returns 0.
*/

#define __INTERNAL_DEFS
#include "libc-alias.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <limits.h>
#include <InnoTekLIBC/locale.h>
#include <emx/time.h>
#include <sys/builtin.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_TIME
#include <InnoTekLIBC/logstrict.h>

#define INS(STR) \
  if (!(i = ins (string, size, STR))) \
    return 0; \
  else \
    string += i, size -= i
#define NUM2(X) \
  if (!(i = num2 (string, size, X, minlen))) \
    return 0; \
  else \
    string += i, size -= i
#define NUM3(X) \
  if (!(i = num3 (string, size, X, minlen))) \
    return 0; \
  else \
    string += i, size -= i
#define FMT(F) \
  if (!(i = strftime (string, size, F, t))) \
    return 0; \
  else \
    string += i, size -= i
#define CHR(C) \
  if (!size) \
    return 0; \
  else \
    *string++ = C, size--

static size_t ins (char *string, size_t size, const char *s)
{
  size_t len = strlen (s);

  if (len >= size)
    return 0;

  memcpy (string, s, len);
  return len;
}

static size_t num2 (char *string, size_t size, int x, unsigned char minlen)
{
  char *orgstring = string;
  long r;

  if (size < 2)
    return 0;

  if (x < 0)
    x = 0;

  x = __ldivmod (x, 10, &r);
  if (x > 9)
    x = x % 10;

  if (minlen > 1 || x > 0)
    *string++ = x + '0';
  *string++ = r + '0';
  return string - orgstring;
}

static size_t num3 (char *string, size_t size, int x, unsigned char minlen)
{
  char *orgstring = string;
  long r1, r2;

  if (size < 3)
    return 0;

  if (x < 0)
    x = 0;

  x = __ldivmod (x, 10, &r1);
  x = __ldivmod (x, 10, &r2);
  if (x > 9)
    x = x % 10;

  if (minlen >= 3 || x > 0)
    *string++ = x + '0', minlen = 2;
  if (minlen >= 2 || r2 > 0)
    *string++ = r2 + '0';
  *string++ = r1 + '0';
  return string - orgstring;
}

static int week (const struct tm *t, int first)
{
  int wd, tmp;

  wd = t->tm_wday - first;    /* wd = relative day in week (w.r.t. first) */
  if (wd < 0) wd += 7;
  tmp = t->tm_yday - wd;      /* Start of current week */
  if (tmp <= 0) return 0;     /* In partial first week */
  return (tmp + 6) / 7;       /* Week number */
}

size_t _STD(strftime) (char *string, size_t size, const char *format,
  const struct tm *t)
{
  LIBCLOG_ENTER("string=%p size=%d format=%p:{%s} t=%p:{.tm_sec=%d, .tm_min=%d, .tm_hour=%d, .tm_mday=%d, .tm_mon=%d, .tm_year=%d, .tm_wday=%d, .tm_yday=%d, .tm_isdst=%d, .tm_gmtoff=%d, .tm_zone=%p:{%s}}\n",
                (void *)string, size, (void *)format, format, (void *)t,
                 t ? t->tm_sec : -1,  t ? t->tm_min : -1, t ? t->tm_hour : -1, t ? t->tm_mday : -1, t ? t->tm_mon : -1,
                 t ? t->tm_year : -1, t ? t->tm_wday : -1, t ? t->tm_yday : -1, t ? t->tm_isdst : -1, t ? /*t->tm_gmtoff*/-2 : -1,
                 (void *)(t ? /*t->tm_zone*/NULL : NULL), t ? /*t->tm_zone*/"" : "");
  size_t i;
  unsigned char c, minlen;
  char *orgstring = string;

  if (!_tzset_flag) tzset ();

  while ((c = *format) != 0)
  {
    if (c == '%')
    {
      minlen = 9;
nextformat:
      ++format;
      switch (*format)
      {
        case 0:
        case '%':
          CHR ('%');
          if (*format == 0)
            --format;
          break;

        /* Handle prefixes first */
        case 'E':
          /* Alternative date/time formats not supported for now */
        case 'O':
          /* Alternative numeric formats not supported for now */
          goto nextformat;

        case '1':
          /* Undocumented in Unicode API reference, encountered in Russian
             locale. I think it should mean that the next number should occupy
             such much characters how it should (e.g. %1H:%M = 1:20 or 23:00). */
          minlen = 1;
          goto nextformat;

        case 'a':
          INS (__libc_gLocaleTime.swdays [t->tm_wday]);
          break;
        case 'A':
          INS (__libc_gLocaleTime.lwdays [t->tm_wday]);
          break;
        case 'b':
        case 'h':
          INS (__libc_gLocaleTime.smonths [t->tm_mon]);
          break;
        case 'B':
          INS (__libc_gLocaleTime.lmonths [t->tm_mon]);
          break;
        case 'c':
          FMT (__libc_gLocaleTime.date_time_fmt);
          break;
        case 'C':
          /* 2000 A.D. is still 20th century (not 21st) */
          NUM2 (1 + (t->tm_year - 1) / 100);
          break;
        case 'd':
          NUM2 (t->tm_mday);
          break;
        case 'D':
          FMT ("%m/%d/%y");
          break;
        case 'e':
          NUM2 (t->tm_mday);
          if (string [-2] == '0')
            string [-2] = ' ';
          break;
        case 'F': /* ISO Date - C99 */
          FMT ("%Y-%m-%d");
          break;
        case 'H':
          NUM2 (t->tm_hour);
          break;
        case 'I':
          NUM2 ((t->tm_hour + 11) % 12 + 1);
          break;
        case 'j':
          NUM3 (t->tm_yday + 1);
          break;
        case 'm':
          NUM2 (t->tm_mon + 1);
          break;
        case 'M':
          NUM2 (t->tm_min);
          break;
        case 'n':
          CHR ('\n');
          break;
        case 'p':
          INS (t->tm_hour >= 12 ? __libc_gLocaleTime.pm : __libc_gLocaleTime.am);
          break;
        case 'r':
          FMT ("%I:%M:%S %p");
          break;
        case 'S':
          NUM2 (t->tm_sec);
          break;
        case 't':
          CHR ('\t');
          break;
        case 'T':
          FMT ("%H:%M:%S");
          break;
        case 'U':
          NUM2 (week (t, 0));
          break;
        case 'w':
          NUM2 (t->tm_wday);
          break;
        case 'W':
          NUM2 (week (t, 1));
          break;
        case 'x':
          FMT (__libc_gLocaleTime.date_fmt);
          break;
        case 'X':
          FMT (__libc_gLocaleTime.time_fmt);
          break;
        case 'y':
          NUM2 (t->tm_year % 100);
          break;
        case 'Y':
          CHR ('0' + (1900 + t->tm_year) / 1000);
          NUM3 ((t->tm_year + 1900) % 1000);
          break;
        case 'Z':
          if (t->tm_isdst >= 0)
          {
            INS (_tzname [(t->tm_isdst ? 1 : 0)]);
          }
          break;
      }
    }
    else if (!CHK_MBCS_PREFIX (&__libc_GLocaleCtype, c, i))
      CHR (c);
    else
    {
      if (i > size)
        LIBCLOG_ERROR_RETURN_INT(0);
      memcpy (string, format, i);
      string += i;
      format += i - 1;
    }
    ++format;
  }

  if (size)
  {
    *string = '\0';
    LIBCLOG_RETURN_MSG(string - orgstring, "ret %d (%#x) - string={%s}\n", string - orgstring, string - orgstring, string);
  }

  LIBCLOG_ERROR_RETURN_INT(0);
}
