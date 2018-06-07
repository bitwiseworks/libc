/* sys/p2t.c (emx+gcc) -- Copyright (c) 1992-1996 by Eberhard Mattes */

#include "libc-alias.h"
#include <os2emx.h>
#include <time.h>
#include <emx/time.h>
#include <emx/syscalls.h>
#include "syscalls.h"

long _sys_p2t (FTIME t, FDATE d)
{
  struct tm tm;

  tm.tm_sec = t.twosecs * 2;
  tm.tm_min = t.minutes;
  tm.tm_hour = t.hours;
  tm.tm_mday = d.day;
  tm.tm_mon = d.month - 1;
  tm.tm_year = d.year + 1980 - 1900;
  tm.tm_isdst = -1;             /* unknown */
  return _mktime (&tm);
}
