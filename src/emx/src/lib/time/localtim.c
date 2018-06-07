/* localtim.c (emx+gcc) -- Copyright (c) 1990-1999 by Eberhard Mattes */

#include "libc-alias.h"
#include <time.h>
#include <InnoTekLIBC/thread.h>
#include <emx/time.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_TIME
#include <InnoTekLIBC/logstrict.h>

struct tm *_localtime64_r(const time64_t *t, struct tm *dst)
{
    LIBCLOG_ENTER("t=%p:{%lld (%#llx)} dst=%p\n", (void *)t, (long long)*t, (long long)*t, (void *)dst);
    if (!_tzset_flag)
        tzset();
    time64_t lt = *t;
    int isdst = _gmt2loc64(&lt);
    struct tm *pTm = _gmtime64_r(&lt, dst);
    pTm->tm_isdst = isdst;
    LIBCLOG_RETURN_MSG(pTm, "ret %p:{.tm_sec=%d, .tm_min=%d, .tm_hour=%d, .tm_mday=%d, .tm_mon=%d, .tm_year=%d, .tm_wday=%d, .tm_yday=%d, .tm_isdst=%d, .tm_gmtoff=%d, .tm_zone=%p:{%s}}\n",
                       (void *)pTm, pTm->tm_sec,  pTm->tm_min, pTm->tm_hour, pTm->tm_mday, pTm->tm_mon, pTm->tm_year,
                       pTm->tm_wday, pTm->tm_yday, pTm->tm_isdst, /*pTm->tm_gmtoff*/-2, /*pTm->tm_zone*/(void *)NULL, /*pTm->tm_zone*/"");
}

struct tm *_STD(localtime_r)(const time_t *t, struct tm *dst)
{
    LIBCLOG_ENTER("t=%p:{%ld} dst=%p\n", (void *)t, (long)*t, (void *)dst);
    time64_t t64 = *t;
    struct tm *pTm = _localtime64_r(&t64, dst);
    LIBCLOG_RETURN_MSG(pTm, "ret %p:{.tm_sec=%d, .tm_min=%d, .tm_hour=%d, .tm_mday=%d, .tm_mon=%d, .tm_year=%d, .tm_wday=%d, .tm_yday=%d, .tm_isdst=%d, .tm_gmtoff=%d, .tm_zone=%p:{%s}}\n",
                       (void *)pTm, pTm->tm_sec,  pTm->tm_min, pTm->tm_hour, pTm->tm_mday, pTm->tm_mon, pTm->tm_year,
                       pTm->tm_wday, pTm->tm_yday, pTm->tm_isdst, /*pTm->tm_gmtoff*/-2, /*pTm->tm_zone*/(void *)NULL, /*pTm->tm_zone*/"");
}

struct tm *_STD(localtime)(const time_t *t)
{
    LIBCLOG_ENTER("t=%p:{%ld}\n", (void *)t, (long)*t);
    __LIBC_PTHREAD pThrd = __libc_threadCurrent();
    struct tm *pTm = localtime_r(t, &pThrd->GmTimeAndLocalTimeBuf);
    LIBCLOG_RETURN_MSG(pTm, "ret %p:{.tm_sec=%d, .tm_min=%d, .tm_hour=%d, .tm_mday=%d, .tm_mon=%d, .tm_year=%d, .tm_wday=%d, .tm_yday=%d, .tm_isdst=%d, .tm_gmtoff=%d, .tm_zone=%p:{%s}}\n",
                       (void *)pTm, pTm->tm_sec,  pTm->tm_min, pTm->tm_hour, pTm->tm_mday, pTm->tm_mon, pTm->tm_year,
                       pTm->tm_wday, pTm->tm_yday, pTm->tm_isdst, /*pTm->tm_gmtoff*/-2, /*pTm->tm_zone*/(void *)NULL, /*pTm->tm_zone*/"");
}

