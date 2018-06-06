/* fmutex2.c (emx+gcc) -- Copyright (c) 1996 by Eberhard Mattes */

#include "libc-alias.h"
#include <os2emx.h>
#include <stdlib.h>
#include <string.h>
#include <sys/builtin.h>
#include <sys/fmutex.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_MUTEX
#include <InnoTekLIBC/logstrict.h>


void _fmutex_checked_close(_fmutex *sem)
{
    LIBCLOG_ENTER("sem=%p\n", (void *)sem);
    if (_fmutex_close(sem) != 0)
    {
        LIBC_ASSERT_FAILED();
        _fmutex_abort(NULL, "checked close");
    }
    LIBCLOG_RETURN_VOID();
}


void _fmutex_checked_create(_fmutex *sem, unsigned flags)
{
    LIBCLOG_ENTER("sem=%p flags=%#x\n", (void *)sem, flags);
    if (_fmutex_create(sem, flags) != 0)
    {
        LIBC_ASSERT_FAILED();
        _fmutex_abort(NULL, "checked create");
    }
    LIBCLOG_RETURN_VOID();
}

void _fmutex_checked_create2(_fmutex *sem, unsigned flags, const char *pszDesc)
{
    LIBCLOG_ENTER("sem=%p flags=%#x pszDesc=%s\n", (void *)sem, flags, pszDesc);
    if (_fmutex_create2(sem, flags, pszDesc) != 0)
    {
        LIBC_ASSERT_FAILED();
        _fmutex_abort(NULL, "checked create2");
    }
    LIBCLOG_RETURN_VOID();
}


void _fmutex_checked_open(_fmutex *sem)
{
    LIBCLOG_ENTER("sem=%p\n", (void *)sem);
    if (_fmutex_open(sem) != 0)
    {
        LIBC_ASSERT_FAILED();
        _fmutex_abort(sem, "open");
    }
    LIBCLOG_RETURN_VOID();
}

void _fmutex_abort(_fmutex *pSem, const char *pszMsg)
{
    ULONG ul;
    static const char szMsg1[] = "\r\n_fmutex operation failed: ";
    DosWrite(2, szMsg1, sizeof(szMsg1), &ul);
    if (pSem && pSem->pszDesc)
    {
        DosWrite(2, pSem->pszDesc, strlen(pSem->pszDesc), &ul);
        DosWrite(2, " ", 1, &ul);
    }
    if (pszMsg)
        DosWrite(2, pszMsg, strlen(pszMsg), &ul);

    static const char szMsg2[] = "\r\n";
    DosWrite(2, szMsg2, sizeof(szMsg2), &ul);

    abort();
}

