#include "libc-alias.h"
#include <time.h>
#include <sys/time.h>
#include <errno.h>
#include <emx/time.h>
#include <emx/syscalls.h>
#define INCL_FSMACROS
#define INCL_DOS
#include <os2.h>

/* Convert a Unix time value to a DATETIME structure. */

static void unix2time (DATETIME *dst, ULONG t)
{
  ULONG lo, hi, i;
  const USHORT *p;

  dst->seconds = t % 60; t /= 60;
  dst->minutes = t % 60; t /= 60;
  dst->hours = t % 24; t /= 24;

  /* Find an i such that _year_day[i] <= t < _year_day[i+1]. */

  lo = 0; hi = _YEARS;
  for (;;)
    {
      i = (lo + hi) / 2;
      if (_year_day[i] > (int)t)
        hi = i - 1;
      else if (_year_day[i+1] <= (int)t)
        lo = i + 1;
      else
        break;
    }
  dst->year = i + 1900;
  t -= _year_day[i];

  p = (_leap_year (dst->year) ? _month_day_leap : _month_day_non_leap);
  for (i = 0; t >= p[i+1]; ++i)
    ;
  dst->month = i + 1;
  dst->day = t - p[i] + 1;
}

int __settime (const struct timeval *tp)
{
  DATETIME dt;
  FS_VAR();

  /* Preserve the value of DATETIME.timezone. */

  FS_SAVE_LOAD();
  if (DosGetDateTime (&dt))
    {
      FS_RESTORE();
      return -1;
    }

  unix2time (&dt, tp->tv_sec + tp->tv_usec / 1000000);

  /* Keep dt.timezone! */

  dt.weekday = 0;
  dt.hundredths = (tp->tv_usec / 10000) % 100;

  if (DosSetDateTime (&dt))
    {
      FS_RESTORE();
      return -1;
    }

  FS_RESTORE();
  return 0;
}
