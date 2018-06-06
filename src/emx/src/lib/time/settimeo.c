/* settimeo.c (emx+gcc) -- Copyright (c) 1995-1996 by Eberhard Mattes */

#include "libc-alias.h"
#include <time.h>
#include <sys/time.h>
#include <errno.h>
#include <emx/time.h>
#include <emx/syscalls.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_TIME
#include <InnoTekLIBC/logstrict.h>

int _STD(settimeofday)(const struct timeval *tp, const struct timezone *tzp)
{
    LIBCLOG_ENTER("tp=%p:{.tv_sec=%lld, .tv_usec=%ld} tzp=%p:{.tz_minuteswest=%d, .tz_dsttime=%d}\n",
                  (void *)tp, tp ? (long long)tp->tv_sec : -1, tp ? (long)tp->tv_usec : -1,
                  (void *)tzp, tzp ? tzp->tz_minuteswest : -1, tzp ? tzp->tz_dsttime : -1);
    struct timeval local;
    time_t t;

    if (tzp != NULL)
    {
        errno = EINVAL;
        LIBCLOG_ERROR_RETURN(-1, "ret -1 - tzp is NULL!\n");
    }
    if (!_tzset_flag)
        tzset();
    t = tp->tv_sec;
    _gmt2loc(&t);
    local.tv_sec = t;
    local.tv_usec = tp->tv_usec;
    int rc = __settime(&local);
    if (!rc)
        LIBCLOG_RETURN_INT(rc);
    LIBCLOG_ERROR_RETURN_INT(rc);
}
