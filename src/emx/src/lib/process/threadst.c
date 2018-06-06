/* threadst.c (emx+gcc) -- Copyright (c) 1993-1996 by Eberhard Mattes */

#include "libc-alias.h"
#include <stdlib.h>
#include <InnoTekLIBC/thread.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_THREAD
#include <InnoTekLIBC/logstrict.h>

/**
 * @obsolete
 */
void **_threadstore(void)
{
    LIBCLOG_ENTER("\n");
    void **ppv = &__libc_threadCurrent()->pvThreadStoreVar;
    LIBCLOG_RETURN_P(ppv);
}
