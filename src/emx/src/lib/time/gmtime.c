/* gmtime.c (emx+gcc) -- Copyright (c) 1990-1999 by Eberhard Mattes */

#include "libc-alias.h"
#include <stdlib.h>
#include <time.h>
#include <stdint.h>
#include <InnoTekLIBC/thread.h>
#include <emx/time.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_TIME
#include <InnoTekLIBC/logstrict.h>


struct tm *_gmtime64_r(const time64_t *t, struct tm *dst)
{
    LIBCLOG_ENTER("t=%p:{%lld (%#llx)} dst=%p\n", (void *)t, (long long)*t, (long long)*t, (void *)dst);
    int days;
    int rem;
    time64_t t1 = *t;

    /* calc days relative to Epoch. */
    days = t1 / (60*60*24);
    rem = t1 % (60*60*24);
    while (rem < 0)
    {
        rem += (60*60*24);
        --days;
    }

    dst->tm_hour= rem / (60*60);
    rem %= (60*60);
    dst->tm_min = rem / 60;
    dst->tm_sec = rem % 60;

    dst->tm_wday = (days + 4) % 7;    /* 01-Jan-1970 was Thursday, i.e. 4 */
    if (dst->tm_wday < 0)
        dst->tm_wday += 7;

    {
        /* Find an i such that _year_day[i] <= days < _year_day[i+1]. */
        int hi = _YEARS - 1;
        if ((int)_year_day[hi] < days)
            LIBCLOG_ERROR_RETURN_P(NULL);
        else if ((int)_year_day[0] > days)
            LIBCLOG_ERROR_RETURN_P(NULL);
        else
        {
            int i = 0;
            int lo = 0;
            for (;;)
            {
                i = (lo + hi) / 2;
                if (_year_day[i] > days)
                    hi = i - 1;
                else if (_year_day[i+1] <= days)
                    lo = i + 1;
                else
                    break;
            }
            dst->tm_year = i;
            days -= _year_day[i];
            dst->tm_yday = days;
        }
    }

    {
        int i;
        const unsigned short *p;

        p = (_leap_year (dst->tm_year + 1900)
             ? _month_day_leap : _month_day_non_leap);
        for (i = 0; (int)days >= p[i+1]; ++i)
            ;
        dst->tm_mon = i;
        dst->tm_mday = days - p[i] + 1;
    }
    dst->tm_isdst = -1;
    LIBCLOG_RETURN_MSG(dst, "ret %p:{.tm_sec=%d, .tm_min=%d, .tm_hour=%d, .tm_mday=%d, .tm_mon=%d, .tm_year=%d, .tm_wday=%d, .tm_yday=%d, .tm_isdst=%d, .tm_gmtoff=%d, .tm_zone=%p:{%s}}\n",
                       (void *)dst, dst->tm_sec,  dst->tm_min, dst->tm_hour, dst->tm_mday, dst->tm_mon, dst->tm_year,
                       dst->tm_wday, dst->tm_yday, dst->tm_isdst, /*dst->tm_gmtoff*/-2, /*dst->tm_zone*/(void *)NULL, /*dst->tm_zone*/"");
}

struct tm *_STD(gmtime_r)(const time_t *t, struct tm *dst)
{
    LIBCLOG_ENTER("t=%p:{%ld} dst=%p\n", (void *)t, (long)*t, (void *)dst);
    time64_t t64 = *t;
    struct tm *pTm = _gmtime64_r(&t64, dst);
    LIBCLOG_RETURN_MSG(pTm, "ret %p:{.tm_sec=%d, .tm_min=%d, .tm_hour=%d, .tm_mday=%d, .tm_mon=%d, .tm_year=%d, .tm_wday=%d, .tm_yday=%d, .tm_isdst=%d, .tm_gmtoff=%d, .tm_zone=%p:{%s}}\n",
                       (void *)pTm, pTm->tm_sec,  pTm->tm_min, pTm->tm_hour, pTm->tm_mday, pTm->tm_mon, pTm->tm_year,
                       pTm->tm_wday, pTm->tm_yday, pTm->tm_isdst, /*pTm->tm_gmtoff*/-2, /*pTm->tm_zone*/(void *)NULL, /*pTm->tm_zone*/"");
}

struct tm *_STD(gmtime)(const time_t *t)
{
    LIBCLOG_ENTER("t=%p:{%ld}\n", (void *)t, (long)*t);
    __LIBC_PTHREAD pThrd = __libc_threadCurrent();
    struct tm *pTm = gmtime_r(t, &pThrd->GmTimeAndLocalTimeBuf);
    LIBCLOG_RETURN_MSG(pTm, "ret %p:{.tm_sec=%d, .tm_min=%d, .tm_hour=%d, .tm_mday=%d, .tm_mon=%d, .tm_year=%d, .tm_wday=%d, .tm_yday=%d, .tm_isdst=%d, .tm_gmtoff=%d, .tm_zone=%p:{%s}}\n",
                       (void *)pTm, pTm->tm_sec,  pTm->tm_min, pTm->tm_hour, pTm->tm_mday, pTm->tm_mon, pTm->tm_year,
                       pTm->tm_wday, pTm->tm_yday, pTm->tm_isdst, /*pTm->tm_gmtoff*/-2, /*pTm->tm_zone*/(void *)NULL, /*pTm->tm_zone*/"");
}
