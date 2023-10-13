/* gettimeo.c (emx+gcc) -- Copyright (c) 1993-1996 by Eberhard Mattes */

#include "libc-alias.h"
#include <sys/timeb.h>
#include <time.h>
#include <sys/time.h>
#include <emx/time.h>
#include <emx/syscalls.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_TIME
#include <InnoTekLIBC/logstrict.h>

int _STD(gettimeofday)(struct timeval *tp, struct timezone *tzp)
{
    LIBCLOG_ENTER("tp=%p tzp=%p\n", (void *)tp, (void *)tzp);
    struct timeb tb;
    int dst;

    if (!_tzset_flag)
        tzset();
    __ftime(&tb);
    dst = _loc2gmt(&tb.time, -1);
    if (tp != NULL)
    {
        tp->tv_sec = tb.time;
        tp->tv_usec = tb.millitm * 1000;
    }
    if (tzp != NULL)
    {
        tzp->tz_minuteswest = _tzi.tz / 60;
        tzp->tz_dsttime = dst;
    }


    if (tp && tzp)
        LIBCLOG_RETURN_MSG(0, "ret 0 - tp=%p:{.tv_sec=%d, .tv_usec=%ld} tzp=%p:{.tz_minuteswest=%d, .tz_dsttime=%d}\n",
                           (void *)tp, tp->tv_sec, tp->tv_usec,
                           (void *)tzp, tzp->tz_minuteswest, tzp->tz_dsttime);
    else if (tp)
        LIBCLOG_RETURN_MSG(0, "ret 0 - tp=%p:{.tv_sec=%d, .tv_usec=%ld}\n",
                           (void *)tp, tp->tv_sec, tp->tv_usec);
    else
        LIBCLOG_RETURN_MSG(0, "ret 0 - tzp=%p:{.tz_minuteswest=%d, .tz_dsttime=%d}\n",
                           (void *)tzp, tzp->tz_minuteswest, tzp->tz_dsttime);
}
