/* mktime.c (emx+gcc) -- Copyright (c) 1990-1999 by Eberhard Mattes */

#include "libc-alias.h"
#include <time.h>
#include <limits.h>
#include <emx/time.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_TIME
#include <InnoTekLIBC/logstrict.h>

time_t _STD(mktime) (struct tm *t)
{
    LIBCLOG_ENTER("t=%p:{.tm_sec=%d, .tm_min=%d, .tm_hour=%d, .tm_mday=%d, .tm_mon=%d, .tm_year=%d, .tm_wday=%d, .tm_yday=%d, .tm_isdst=%d, .tm_gmtoff=%d, .tm_zone=%p:{%s}}\n",
                  (void *)t, t->tm_sec,  t->tm_min, t->tm_hour, t->tm_mday, t->tm_mon, t->tm_year, t->tm_wday, t->tm_yday,
                  t->tm_isdst, /*t->tm_gmtoff*/-2, /*t->tm_zone*/(void *)NULL, /*t->tm_zone*/"");
    time64_t t64 = _mktime64(t);
    if (t64 != -1 && t64 >= TIME_T_MIN && t64 <= TIME_T_MAX)
    {
        time_t t32 = (time_t)t64;
        LIBCLOG_RETURN_MSG(t32, "ret %ld (%#lx) t=%p:{.tm_sec=%d, .tm_min=%d, .tm_hour=%d, .tm_mday=%d, .tm_mon=%d, .tm_year=%d, .tm_wday=%d, .tm_yday=%d, .tm_isdst=%d, .tm_gmtoff=%d, .tm_zone=%p:{%s}}\n",
                           (long)t32, (long)t32, (void *)t, t->tm_sec,  t->tm_min, t->tm_hour, t->tm_mday, t->tm_mon, t->tm_year, t->tm_wday, t->tm_yday,
                           t->tm_isdst, /*t->tm_gmtoff*/-2, /*t->tm_zone*/(void *)NULL, /*t->tm_zone*/"");
    }
    LIBCLOG_ERROR_RETURN_MSG(-1, "ret -1 (t64=%lld) t=%p:{.tm_sec=%d, .tm_min=%d, .tm_hour=%d, .tm_mday=%d, .tm_mon=%d, .tm_year=%d, .tm_wday=%d, .tm_yday=%d, .tm_isdst=%d, .tm_gmtoff=%d, .tm_zone=%p:{%s}}\n",
                             t64, (void *)t, t->tm_sec,  t->tm_min, t->tm_hour, t->tm_mday, t->tm_mon, t->tm_year, t->tm_wday,
                             t->tm_yday, t->tm_isdst, /*t->tm_gmtoff*/-2, /*t->tm_zone*/(void *)NULL, /*t->tm_zone*/"");
}


time64_t _mktime64(struct tm *t)
{
    time64_t t1, t2;
    struct tm tmp;
    int dst;

    if (!_tzset_flag)
        tzset();

    /* mktime() requires that tm_mon is in range.  The other members
       are not restricted. */

    if (t->tm_mon < 0)
    {
        /* Avoid applying the `/' and `%' operators to negative numbers
           as the results are implementation-defined for negative
           numbers. */

        t->tm_year -= 1 + ((-t->tm_mon) / 12);
        t->tm_mon = 12 - ((-t->tm_mon) % 12);
    }
    if (t->tm_mon >= 12)
    {
        t->tm_year += (t->tm_mon / 12);
        t->tm_mon %= 12;
    }

    t1 = __mktime64(t);
    if (t1 == -1)
        return -1;
    dst = _loc2gmt64(&t1, t->tm_isdst);
    if (dst == -1)
        return -1;
    t2 = t1;
    dst = _gmt2loc64(&t2);
    if (dst == -1)
        return -1;

    if (_gmtime64_r(&t2, &tmp) == NULL)
        return -1;
    *t = tmp;
    t->tm_isdst = dst;
    return t1;
}


/* year >= 1582, 1 <= month <= 12, 1 <= day <= 31 */

int _day (int year, int month, int day)
{
  int result;

  if (year < 1582 || year >= INT_MAX / 365)
    return -1;
  result = 365 * year + day + 31 * (month - 1);
  if (month <= 2)
    --year;
  else
    result -= (4 * month + 23) / 10;
  result += year / 4 - (3 * (year / 100 + 1)) / 4;
  return result;
}

time64_t __mktime64(struct tm *t)
{
  time64_t x;
  time64_t r;

  x = _day(t->tm_year + 1900, t->tm_mon + 1, t->tm_mday);
  /* 719528 = day (1970, 1, 1) */
  r = (x - 719528) * 24 * 60 * 60;

  /* This expression is not checked for overflow. */
  x = t->tm_sec
      + 60 * t->tm_min
      + 60 *60 * t->tm_hour;

  r += x;
  return r;
}

/* mkstd.awk: NOUNDERSCORE(mktime) */

unsigned long _mktime(struct tm *t)
{
    LIBCLOG_ENTER("t=%p:{.tm_sec=%d, .tm_min=%d, .tm_hour=%d, .tm_mday=%d, .tm_mon=%d, .tm_year=%d, .tm_wday=%d, .tm_yday=%d, .tm_isdst=%d, .tm_gmtoff=%d, .tm_zone=%p:{%s}}\n",
                  (void *)t, t->tm_sec,  t->tm_min, t->tm_hour, t->tm_mday, t->tm_mon, t->tm_year, t->tm_wday, t->tm_yday,
                  t->tm_isdst, /*t->tm_gmtoff*/-2, /*t->tm_zone*/(void *)NULL, /*t->tm_zone*/"");
    long x;
    long long r;

    x = _day (t->tm_year+1900, t->tm_mon+1, t->tm_mday);
    /* 719528 = day (1970, 1, 1) */
    r = (long long)(x - 719528) * 24 * 60 * 60;

    /* This expression is not checked for overflow. */
    x = t->tm_sec + 60*t->tm_min + 60*60*t->tm_hour;

    r += x;
    if (r < 0 || r > ULONG_MAX || r == (time_t)-1)
        LIBCLOG_RETURN_MSG((time_t)-1, "ret -1 - r=%lld\n", r);
    LIBCLOG_RETURN_MSG((time_t)r, "ret %lu (%#lx)\n", (unsigned long)r, (unsigned long)r);
}

