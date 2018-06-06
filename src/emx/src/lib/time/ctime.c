/* ctime.c (emx+gcc) -- Copyright (c) 1990-1999 by Eberhard Mattes
                     -- Copyright (c) 2003 by Knut Stange Osmundsen */
#include "libc-alias.h"
#include <time.h>
#include <emx/time.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_TIME
#include <InnoTekLIBC/logstrict.h>

char *_STD(ctime)(const time_t *t)
{
    LIBCLOG_ENTER("t=%p:{%lld (%#llx)}\n", (void *)t, (long long)*t, (long long)*t);
    struct tm tmp, *pTm;

    pTm = localtime_r(t, &tmp);
    if (pTm != NULL)
    {
        char *pszRet = asctime(pTm);
        LIBCLOG_RETURN_MSG(pszRet, "ret %p:{%s}\n", pszRet, pszRet);
    }
    LIBCLOG_ERROR_RETURN_P(NULL);
}

char *_STD(ctime_r)(const time_t * t, char *buf)
{
    LIBCLOG_ENTER("t=%p:{%lld (%#llx)} buf=%p\n", (void *)t, (long long)*t, (long long)*t, (void *)buf);
    struct tm tmp, *pTm;
    pTm = localtime_r(t, &tmp);
    if (pTm != NULL)
    {
        char *pszRet = asctime_r(pTm, buf);
        LIBCLOG_RETURN_MSG(pszRet, "ret %p:{%s}\n", pszRet, pszRet);
    }
    LIBCLOG_ERROR_RETURN_P(NULL);
}
