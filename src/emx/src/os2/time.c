/* time.c -- Time and date
   Copyright (c) 1994-1995 by Eberhard Mattes

This file is part of emx.

emx is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

emx is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with emx; see the file COPYING.  If not, write to
the Free Software Foundation, 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.

As special exception, emx.dll can be distributed without source code
unless it has been changed.  If you modify emx.dll, this exception
no longer applies and you must remove this paragraph from all source
files for emx.dll.  */


#define INCL_DOSMISC
#include <os2emx.h>
#include "emxdll.h"
#include <limits.h>
#include <sys/timeb.h>
#include <sys/time.h>


/* Day number, relative to 01-Jan-1970, of 01-Jan for the years 1970
   through 2106 */

static ULONG const year_table[] =
{
  0, 365, 730, 1096, 1461, 1826, 2191, 2557, 2922, 3287, 3652, 4018,
  4383, 4748, 5113, 5479, 5844, 6209, 6574, 6940, 7305, 7670, 8035,
  8401, 8766, 9131, 9496, 9862, 10227, 10592, 10957, 11323, 11688,
  12053, 12418, 12784, 13149, 13514, 13879, 14245, 14610, 14975,
  15340, 15706, 16071, 16436, 16801, 17167, 17532, 17897, 18262,
  18628, 18993, 19358, 19723, 20089, 20454, 20819, 21184, 21550,
  21915, 22280, 22645, 23011, 23376, 23741, 24106, 24472, 24837,
  25202, 25567, 25933, 26298, 26663, 27028, 27394, 27759, 28124,
  28489, 28855, 29220, 29585, 29950, 30316, 30681, 31046, 31411,
  31777, 32142, 32507, 32872, 33238, 33603, 33968, 34333, 34699,
  35064, 35429, 35794, 36160, 36525, 36890, 37255, 37621, 37986,
  38351, 38716, 39082, 39447, 39812, 40177, 40543, 40908, 41273,
  41638, 42004, 42369, 42734, 43099, 43465, 43830, 44195, 44560,
  44926, 45291, 45656, 46021, 46387, 46752, 47117, 47482, 47847,
  48212, 48577, 48942, 49308, 49673, ULONG_MAX
};

/* Day number, relative to 01-Jan, of day 01 for January through
   December. */

static ULONG const month_table_non_leap[] = 
  {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, ULONG_MAX};

static ULONG const month_table_leap[] = 
  {0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335, ULONG_MAX};


/* Convert a my_datetime structure to Unix time format. */

static time_t time2unix (const struct my_datetime *src)
{
  ULONG t, y;

  if (src->day < 1 || src->day > 31  || src->month < 1 || src->month > 12
      || src->year < 1970 || src->year > 2105
      || src->seconds >= 60 || src->minutes >= 60 || src->hours >= 24)
    return 0;

  t = src->year * 365 + (src->month - 1) * 31 + src->day;
  if (src->month <= 2)
    {
      /* Jan and Feb. */
      y = src->year - 1;
    }
  else
    {
      /* Mar through Dec. */
      y = src->year;
      t -= (4 * src->month + 23) / 10;
    }
  t += y / 4;
  t -= (3 * (y / 100 + 1)) / 4;
  t -= 719528;                  /* 01-Jan-1970 */
  return ((t * 24 + src->hours) * 60 + src->minutes) * 60 + src->seconds;
}


/* Return 1 if Y is a leap year, return 0 otherwise. */

static int leap_year (ULONG y)
{
  if (y % 4 != 0)
    return 0;
  else if (y % 100 != 0)
    return 1;
  else if (y % 400 != 0)
    return 0;
  else
    return 1;
}


/* Convert a Unix time value to a my_datetime structure. */

void unix2time (struct my_datetime *dst, ULONG t)
{
  ULONG lo, hi, i;
  const ULONG *p;

  dst->seconds = t % 60; t /= 60;
  dst->minutes = t % 60; t /= 60;
  dst->hours = t % 24; t /= 24;

  /* Find an i such that year_table[i] <= t < year_table[i+1]. */

  lo = 0; hi = sizeof (year_table) / sizeof (year_table[0]) - 2;
  for (;;)
    {
      i = (lo + hi) / 2;
      if (year_table[i] > (int)t)
        hi = i - 1;
      else if (year_table[i+1] <= (int)t)
        lo = i + 1;
      else
        break;
    }
  dst->year = i + 1970;
  t -= year_table[i];

  p = (leap_year (dst->year) ? month_table_leap : month_table_non_leap);
  for (i = 0; t >= p[i+1]; ++i)
    ;
  dst->month = i + 1;
  dst->day = t - p[i] + 1;
}


/* Get current time and store it in Unix format to the TIMEB structure
   pointed to by DST. */

void do_ftime (struct timeb *dst)
{
  DATETIME dt;
  struct my_datetime tmp;

  DosGetDateTime (&dt);
  tmp.seconds = dt.seconds;
  tmp.minutes = dt.minutes;
  tmp.hours = dt.hours;
  tmp.day = dt.day;
  tmp.month = dt.month;
  tmp.year = dt.year;
  dst->time = time2unix (&tmp);
  dst->dstflag = 0;
  dst->millitm = dt.hundredths * 10;
  dst->timezone = dt.timezone;  /* This will be overridden by ftime() */
}


/* Convert packed time and date to Unix format. */

ULONG packed2unix (FDATE date, FTIME time)
{
  struct my_datetime tmp;

  tmp.seconds = time.twosecs * 2;
  tmp.minutes = time.minutes;
  tmp.hours   = time.hours;
  tmp.day     = date.day;
  tmp.month   = date.month;
  tmp.year    = date.year + 1980;
  return time2unix (&tmp);
}


/* Return the number of 1/100 seconds elapsed since the process has
   been started.

   This function no longer tries to use QSV_TIME_LOW and QSV_TIME_HIGH
   to increase the time before the return value wraps around.  Now, it
   will wrap around after 49 days. */

unsigned long long get_clock (int init_flag)
{
  ULONG ms;
  static ULONG clock0_ms;

  ms = querysysinfo (QSV_MS_COUNT);
  if (init_flag)
    {
      clock0_ms = ms;
      return 0;
    }
  else
    return (ms - clock0_ms) / 10;
}


int do_settime (const struct timeval *tp, int *errnop)
{
  DATETIME dt;
  struct my_datetime mdt;
  ULONG rc;

  /* Preserve the value of DATETIME.timezone. */

  rc = DosGetDateTime (&dt);
  if (rc != 0)
    {
      *errnop = set_error (rc);
      return -1;
    }
  
  unix2time (&mdt, tp->tv_sec + tp->tv_usec / 1000000);

  /* Keep dt.timezone! */

  dt.day     = mdt.day;
  dt.month   = mdt.month;
  dt.year    = mdt.year;
  dt.weekday = 0;

  dt.hours   = mdt.hours;
  dt.minutes = mdt.minutes;
  dt.seconds = mdt.seconds;
  dt.hundredths = (tp->tv_usec / 10000) % 100;

  rc = DosSetDateTime (&dt);
  if (rc != 0)
    {
      *errnop = set_error (rc);
      return -1;
    }
  *errnop = 0;
  return 0;
}
