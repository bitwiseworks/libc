/* asctime.c (emx+gcc) -- Copyright (c) 1990-1996 by Eberhard Mattes */

#include "libc-alias.h"
#include <string.h>
#include <time.h>
#include <InnoTekLIBC/thread.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_TIME
#include <InnoTekLIBC/logstrict.h>

static char const months[] = "JanFebMarAprMayJunJulAugSepOctNovDec";
static char const wdays[] = "SunMonTueWedThuFriSat";

#define digit(i) (char)(((i)%10)+'0')

char *_STD(asctime) (const struct tm *t)
{
    LIBCLOG_ENTER("t=%p:{.tm_sec=%d, .tm_min=%d, .tm_hour=%d, .tm_mday=%d, .tm_mon=%d, .tm_year=%d, .tm_wday=%d, .tm_yday=%d, .tm_isdst=%d, .tm_gmtoff=%d, .tm_zone=%p:{%s}}\n",
                  (void *)t, t ? t->tm_sec : -1,  t ? t->tm_min : -1, t ? t->tm_hour : -1, t ? t->tm_mday : -1, t ? t->tm_mon : -1,
                  t ? t->tm_year : -1, t ? t->tm_wday : -1, t ? t->tm_yday : -1, t ? t->tm_isdst : -1, t ? /*t->tm_gmtoff*/-2 : -1,
                  (void *)(t ? /*t->tm_zone*/NULL : NULL), t ? /*t->tm_zone*/"" : "");
    __LIBC_PTHREAD pThrd = __libc_threadCurrent();
    char *pszRet = asctime_r(t, pThrd->szAscTimeAndCTimeBuf);
    LIBCLOG_RETURN_MSG(pszRet, "ret %p:{%s}\n", pszRet, pszRet);
}

char *_STD(asctime_r)(const struct tm *t, char *result)
{
    LIBCLOG_ENTER("t=%p:{.tm_sec=%d, .tm_min=%d, .tm_hour=%d, .tm_mday=%d, .tm_mon=%d, .tm_year=%d, .tm_wday=%d, .tm_yday=%d, .tm_isdst=%d, .tm_gmtoff=%d, .tm_zone=%p:{%s}} result=%p\n",
                  (void *)t, t ? t->tm_sec : -1,  t ? t->tm_min : -1, t ? t->tm_hour : -1, t ? t->tm_mday : -1, t ? t->tm_mon : -1,
                  t ? t->tm_year : -1, t ? t->tm_wday : -1, t ? t->tm_yday : -1, t ? t->tm_isdst : -1, t ? /*t->tm_gmtoff*/-2 : -1,
                  (void *)(t ? /*t->tm_zone*/NULL : NULL), t ? /*t->tm_zone*/"" : "", result);
    memcpy(result + 0, wdays + t->tm_wday * 3, 3);
    result[3] = ' ';
    memcpy(result + 4, months + t->tm_mon * 3, 3);
    result[7] = ' ';
    result[8] = digit(t->tm_mday / 10);
    result[9] = digit(t->tm_mday / 1);
    result[10] = ' ';
    result[11] = digit(t->tm_hour / 10);
    result[12] = digit(t->tm_hour / 1);
    result[13] = ':';
    result[14] = digit(t->tm_min / 10);
    result[15] = digit(t->tm_min / 1);
    result[16] = ':';
    result[17] = digit(t->tm_sec / 10);
    result[18] = digit(t->tm_sec / 1);
    result[19] = ' ';
    result[20] = digit((t->tm_year+1900) / 1000);
    result[21] = digit((t->tm_year+1900) / 100);
    result[22] = digit((t->tm_year+1900) / 10);
    result[23] = digit((t->tm_year+1900) / 1);
    result[24] = '\n';
    result[25] = '\0';
    if (result[8] == '0') result[8] = ' ';
    return result;
    LIBCLOG_RETURN_MSG(result, "ret %p:{%s}\n", result, result);
}

