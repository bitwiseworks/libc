/*
    Locale support implementation through OS/2 Unicode API.
    Copyright (c) 1992-1996 by Eberhard Mattes
    Copyright (c) 2003 InnoTek Systemberatung GmbH
    Copyright (c) 2005 knut st. osmundsen <bird@anduin.net>

    For conditions of distribution and use, see the file COPYING.

    Parse a time specification in the input string pointed to by buffer
    according to the format string pointed to by format, updating the
    structure pointed to by t.

    Whitespace (any number of spaces) in the format string matches whitespace
    (any number of spaces, including zero) in the input string. All other
    characters except for % are compared to the input. Parsing ends (with an
    error being indicated) when a mismatch is encountered. %% in the format
    string matches % in the input string. A % which is not followed by another
    % or the end of the string starts a field specification. The field in the
    input is interpreted according to the field specification. For all field
    types, whitespace is ignored at the start of a field. Field specifications
    matching numbers skip leading zeros. Case is ignored when comparing strings.
    The field width is not restricted, that is, as many characters as possible
    are matched.

    If successful, strptime() returns a pointer to the character following the
    last character parsed. On error, strptime() returns NULL.
*/

#define __INTERNAL_DEFS
#include "libc-alias.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <InnoTekLIBC/locale.h>
#include <ctype.h>
#include <emx/time.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_TIME
#include <InnoTekLIBC/logstrict.h>


#define RECURSE(FMT) \
  if (!(s = parse_fmt (s, FMT, tm, &mask))) \
    return NULL;
#define NUMBER(DST,MIN,MAX,ADD,LEN) \
  if (!(s = parse_number (s, DST, MIN, MAX, ADD, LEN))) \
    return NULL;
#define STRING(STR) \
  if (!(s = parse_string (s, STR))) \
    return NULL;
#define TABLE(DST,TAB,N) \
  if (!(s = parse_table (s, DST, TAB, N))) \
    return NULL;

static const char *parse_number (const char *s, int *dst,
  int min, int max, int add, int len)
{
  int n;

  while (*s != 0 && isspace (*s))
    ++s;
  if (*s == 0 || !isdigit (*s))
    return NULL;
  n = 0;
  while (isdigit (*s) && len-- > 0)
    n = n * 10 + (*s++ - '0');  /* TODO: check for overflow */
  if (n < min || (max >= 0 && n > max))
    return NULL;
  *dst = n + add;
  return s;
}

static const char *parse_string (const char *s,
  const char *str)
{
  size_t len;

  while (*s != 0 && isspace (*s))
    ++s;
  len = strlen (str);
  if (len == 0 || strnicmp (s, str, len) != 0)
    return NULL;
  return s + len;
}

static const char *parse_table (const char *s, int *dst,
  char **tab, int n)
{
  int i;
  size_t len;

  while (*s != 0 && isspace (*s))
    ++s;
  for (i = 0; i < n; ++i)
  {
    len = strlen (tab[i]);
    if (strnicmp (s, tab[i], len) == 0)
    {
      *dst = i;
      return s + len;
    }
  }
  return NULL;
}

static const char *parse_fmt (const char *s,
  const char *f, struct tm *tm, unsigned *retmask)
{
  const char *t;
  int week = -1;
  int century = -1;
  /* The mask of found fields */
  unsigned mask = 0;

#define MASK_CENTURY	0x00000001
#define MASK_YEAR_ANY   0x10000000	
#define MASK_YEAR2	0x10000002
#define MASK_YEAR4	0x10000004
#define MASK_YEARDAY	0x00000008
#define MASK_MONTH	0x00000010
#define MASK_MONTHDAY	0x00000020
#define MASK_WEEK_ANY	0x20000000
#define MASK_WEEKS	0x20000040
#define MASK_WEEKM	0x20000080
#define MASK_WEEKI	0x20001000
#define MASK_WEEKDAY	0x00000100
#define MASK_HOUR	0x00000200
#define MASK_MINUTE	0x00000400
#define MASK_SECOND	0x00000800

  while (*f != 0)
  {
    if (isspace (*f))
    {
      while (*s != 0 && isspace (*s))
        ++s;
      ++f;
    }
    else if (*f != '%')
    {
      if (*s != *f)
        return NULL;
      ++s; ++f;
    }
    else
    {
      ++f;
nextformat:
      switch (*f++)
      {
        case 0:
          if (*s != '%')
            return NULL;
          return s + 1;
          break;

        case '%': /* A percent is just a percent */
          if (*s != '%')
            return NULL;
          ++s;
          break;

        /* Handle prefixes first */
        case 'E':
          /* Alternative date/time formats not supported for now */
        case 'O':
          /* Alternative numeric formats not supported for now */
        case '1':
          /* Numeric length does not have sense here */
          goto nextformat;

        case 'a': /* Short weekday name */
          TABLE (&tm->tm_wday, __libc_gLocaleTime.swdays, 7);
          mask |= MASK_WEEKDAY;
          break;

        case 'A': /* Long weekday name */
          TABLE (&tm->tm_wday, __libc_gLocaleTime.lwdays, 7);
          mask |= MASK_WEEKDAY;
          break;

        case 'b': /* Short month name */
        case 'h':
          TABLE (&tm->tm_mon, __libc_gLocaleTime.smonths, 12);
          mask |= MASK_MONTH;
          break;

        case 'B': /* Long month name */
          TABLE (&tm->tm_mon, __libc_gLocaleTime.lmonths, 12);
          mask |= MASK_MONTH;
          break;

        case 'c': /* Locale's defined time and date format */
          RECURSE (__libc_gLocaleTime.date_time_fmt);
          break;

        case 'C': /* Century number */
          NUMBER (&century, 0, 99, 0, 2);
          mask |= MASK_CENTURY;
          break;

        case 'd': /* Day of the month (1-31) */
        case 'e':
          NUMBER (&tm->tm_mday, 1, 31, 0, 2);
          mask |= MASK_MONTHDAY;
          break;

        case 'D': /* MM/DD/YY */
          RECURSE ("%m/%d/%y");
          break;

        case 'F': /* ISO Date - C99 */
          RECURSE ("%Y-%m-%d");
          break;

        case 'H': /* Hour (00-23) */
        case 'k':
          NUMBER (&tm->tm_hour, 0, 23, 0, 2);
          mask |= MASK_HOUR;
          break;

        case 'I': /* Hour (01-12) */
        case 'l':
          NUMBER (&tm->tm_hour, 1, 12, 0, 2);
          mask |= MASK_HOUR;
          break;

        case 'j': /* Day of the year (1-366) */
          NUMBER (&tm->tm_yday, 1, 366, -1, 3);
          mask |= MASK_YEARDAY;
          break;

        case 'm': /* Month (01-12) */
          NUMBER (&tm->tm_mon, 1, 12, -1, 2);
          mask |= MASK_MONTH;
          break;

        case 'M': /* Minutes (00-59) */
          NUMBER (&tm->tm_min, 0, 59, 0, 2);
          mask |= MASK_MINUTE;
          break;

        case 'n': /* Newline */
          if (*s != '\n')
            return NULL;
          ++s;
          break;

        case 'p': /* AM or PM */
          if (!(mask & MASK_HOUR)
           || (tm->tm_hour < 1)
           || (tm->tm_hour > 12))
            return NULL;
          if (   (t = parse_string (s, "AM")) != NULL
              || (t = parse_string (s, __libc_gLocaleTime.am)) != NULL)
          {
            if (tm->tm_hour == 12)
              tm->tm_hour = 0;
          }
          else if (   (t = parse_string (s, "PM")) != NULL
                   || (t = parse_string (s, __libc_gLocaleTime.pm)) != NULL)
          {
            if (tm->tm_hour != 12)
              tm->tm_hour += 12;
          }
          else
            return NULL;
          s = t;
          break;

        case 'P': /* am or pm */
          if (!(mask & MASK_HOUR)
           || (tm->tm_hour < 1)
           || (tm->tm_hour > 12))
            return NULL;
          if (   (t = parse_string (s, "am")) != NULL
              || (t = parse_string (s, __libc_gLocaleTime.am)) != NULL)
          {
            if (tm->tm_hour == 12)
              tm->tm_hour = 0;
          }
          else if (   (t = parse_string (s, "pm")) != NULL
                   || (t = parse_string (s, __libc_gLocaleTime.pm)) != NULL)
          {
            if (tm->tm_hour != 12)
              tm->tm_hour += 12;
          }
          else
            return NULL;
          s = t;
          break;

        case 'r': /* HH:MM:SS am/pm */
          RECURSE ("%I:%M:%S %p");
          break;

        case 'R': /* HH:MM */
          RECURSE ("%H:%M");
          break;

        case 'S': /* Seconds (00-61(?)) */
          NUMBER (&tm->tm_sec, 0, 61, 0, 2);
          mask |= MASK_SECOND;
          break;

        case 't': /* Tabulation */
          if (*s != '\t')
            return NULL;
          ++s;
          break;

        case 'T': /* HH:MM:SS */
          RECURSE ("%H:%M:%S");
          break;

        case 'w': /* Weekday (0-6), 0 = Sunday */
          NUMBER (&tm->tm_wday, 0, 6, 0, 1);
          mask |= MASK_WEEKDAY;
          break;

        case 'U': /* Week number (0-53), 0 = Sunday */
          NUMBER (&week, 0, 53, 0, 2);
          mask |= MASK_WEEKS;
          break;

        case 'W': /* Week number (0-53), 0 = Monday */
          NUMBER (&week, 0, 53, 0, 2);
          mask |= MASK_WEEKM;
          break;

        case 'x':
          RECURSE (__libc_gLocaleTime.date_fmt);
          break;

        case 'X':
          RECURSE (__libc_gLocaleTime.time_fmt);
          break;

        case 'y':
          NUMBER (&tm->tm_year, 0, 99, 0, 2);
          if (tm->tm_year < 69)
            tm->tm_year += 100;
          mask |= MASK_YEAR2;
          break;

        case 'Y':
          NUMBER (&tm->tm_year, 1900, -1, -1900, 4);
          mask |= MASK_YEAR4;
          /* Ignore century since it was explicitely given */
          century = -1;
          mask &= ~MASK_CENTURY;
          break;

        default:
          return NULL;
      }
    }
  }

  if (mask & MASK_CENTURY)
  {
    if (!(mask & MASK_YEAR_ANY))
      tm->tm_year = 0;
    tm->tm_year = (century - 19) * 100 + (tm->tm_year % 100);
    mask |= MASK_YEAR4;
  }

  /* We should know which fields are already correctly filled */
  if (retmask)
    mask |= *retmask;

  if ((mask & MASK_YEAR_ANY) && (mask & MASK_YEARDAY)
   && (mask & (MASK_MONTH | MASK_MONTHDAY)) != (MASK_MONTH | MASK_MONTHDAY))
  {
    /* Compute month and day of the month if any of them is not given */
    const unsigned short *md = _leap_year (tm->tm_year + 1900) ?
      _month_day_leap : _month_day_non_leap;
    for (tm->tm_mon = 11; tm->tm_mon > 0 && tm->tm_yday < md [tm->tm_mon]; tm->tm_mon--)
        /* nothing */;
    tm->tm_mday = 1 + tm->tm_yday - md [tm->tm_mon];
    mask |= MASK_MONTH | MASK_MONTHDAY;
  }

  if (!(mask & MASK_YEARDAY))
  {
    if ((mask & MASK_WEEK_ANY) && (mask & MASK_WEEKDAY) && (mask & MASK_YEAR_ANY)
     && (tm->tm_year >= 70) && (tm->tm_year <= 206))
    {
      /* Compute day of the year given week number and weekday */
      int dow = (mask & MASK_WEEKM) == MASK_WEEKM ? 3 : 4;
      dow = (dow + _year_day [tm->tm_year]) % 7;
      if (!dow)
          week--;
      dow = (tm->tm_wday - ((mask & MASK_WEEKM) == MASK_WEEKM) - dow) % 7;
      if (dow < 0)
        dow += 7;
      tm->tm_yday = week * 7 + dow;
      mask |= MASK_YEARDAY;
    }
    else if ((mask & MASK_YEAR_ANY) && (mask & MASK_MONTH) && (mask & MASK_MONTHDAY))
    {
      /* Compute year day from month and day of the month */
      const unsigned short *md = _leap_year (tm->tm_year + 1900) ?
        _month_day_leap : _month_day_non_leap;
      tm->tm_yday = tm->tm_mday - 1 + md [tm->tm_mon];
      mask |= MASK_YEARDAY;
    }
  }

  if (!(mask & MASK_WEEKDAY) && (mask & MASK_YEAR_ANY) && (mask & MASK_YEARDAY))
  {
    /* Compute day of the week if it was not given */
    if ((tm->tm_year < 70) || (tm->tm_year > 206))
      tm->tm_wday = 0; /* Unknown */
    else
    {
      int absday = _year_day [tm->tm_year] + tm->tm_yday;
      /* 1st January 1970 was Thursday (4) */
      tm->tm_wday = ((4 + absday) % 7);
      mask |= MASK_WEEKDAY;
    }
  }

  if (((mask & (MASK_WEEK_ANY | MASK_WEEKDAY | MASK_YEAR_ANY)) == (MASK_WEEK_ANY | MASK_WEEKDAY | MASK_YEAR_ANY))
   && ((mask & (MASK_MONTH | MASK_MONTHDAY)) != (MASK_MONTH | MASK_MONTHDAY)))
  {
    /* Compute month and day of the month given weekday and week number */
    const unsigned short *md = _leap_year (tm->tm_year + 1900) ?
      _month_day_leap : _month_day_non_leap;

    for (tm->tm_mon = 11; tm->tm_mon > 0 && tm->tm_yday < md [tm->tm_mon]; tm->tm_mon--)
        /* nothing */;
    tm->tm_mday = 1 + tm->tm_yday - md [tm->tm_mon];
    mask |= MASK_MONTH | MASK_MONTHDAY;
  }

  /* Communicate to the caller which fields are filled, except fields
     that are not located inside the tm structure. */
  if (retmask)
    *retmask |= mask & ~(MASK_WEEKI | MASK_WEEKM | MASK_WEEKS | MASK_CENTURY);

  return s;
}

char *_STD(strptime) (const char *buf, const char *format, struct tm *tm)
{
    LIBCLOG_ENTER("buf=%p:{%s} format=%p:{%s} tm=%p:{.tm_sec=%d, .tm_min=%d, .tm_hour=%d, .tm_mday=%d, .tm_mon=%d, .tm_year=%d, .tm_wday=%d, .tm_yday=%d, .tm_isdst=%d, .tm_gmtoff=%d, .tm_zone=%p:{%s}}\n",
                  (void *)buf, buf, (void *)format, format, (void *)tm,
                  tm ? tm->tm_sec : -1,  tm ? tm->tm_min : -1, tm ? tm->tm_hour : -1, tm ? tm->tm_mday : -1, tm ? tm->tm_mon : -1,
                  tm ? tm->tm_year : -1, tm ? tm->tm_wday : -1, tm ? tm->tm_yday : -1, tm ? tm->tm_isdst : -1, tm ? /*tm->tm_gmtoff*/-2 : -1,
                  (void *)(tm ? /*tm->tm_zone*/NULL : NULL), tm ? /*tm->tm_zone*/"" : "");
    char *pszRet = (char *)parse_fmt((const char *)buf, (const char *)format, tm, NULL);
    if (pszRet)
        LIBCLOG_RETURN_MSG(pszRet, "ret %p:{%s} - tm=%p:{.tm_sec=%d, .tm_min=%d, .tm_hour=%d, .tm_mday=%d, .tm_mon=%d, .tm_year=%d, .tm_wday=%d, .tm_yday=%d, .tm_isdst=%d, .tm_gmtoff=%d, .tm_zone=%p:{%s}}\n",
                           (void *)pszRet, pszRet, (void *)tm,
                           tm ? tm->tm_sec : -1,  tm ? tm->tm_min : -1, tm ? tm->tm_hour : -1, tm ? tm->tm_mday : -1, tm ? tm->tm_mon : -1,
                           tm ? tm->tm_year : -1, tm ? tm->tm_wday : -1, tm ? tm->tm_yday : -1, tm ? tm->tm_isdst : -1, tm ? /*tm->tm_gmtoff*/-2 : -1,
                           (void *)(tm ? /*tm->tm_zone*/NULL : NULL), tm ? /*tm->tm_zone*/"" : "");
    LIBCLOG_ERROR_RETURN_MSG(pszRet, "ret NULL - tm=%p:{.tm_sec=%d, .tm_min=%d, .tm_hour=%d, .tm_mday=%d, .tm_mon=%d, .tm_year=%d, .tm_wday=%d, .tm_yday=%d, .tm_isdst=%d, .tm_gmtoff=%d, .tm_zone=%p:{%s}}\n",
                             (void *)tm,
                             tm ? tm->tm_sec : -1,  tm ? tm->tm_min : -1, tm ? tm->tm_hour : -1, tm ? tm->tm_mday : -1, tm ? tm->tm_mon : -1,
                             tm ? tm->tm_year : -1, tm ? tm->tm_wday : -1, tm ? tm->tm_yday : -1, tm ? tm->tm_isdst : -1, tm ? /*tm->tm_gmtoff*/-2 : -1,
                             (void *)(tm ? /*tm->tm_zone*/NULL : NULL), tm ? /*tm->tm_zone*/"" : "");
}

