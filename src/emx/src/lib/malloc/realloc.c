/* realloc.c (emx+gcc) -- Copyright (c) 1996 by Eberhard Mattes */

#include "libc-alias.h"
#include <stdlib.h>
#include <stddef.h>
#include <umalloc.h>
#include <sys/builtin.h>        /* For <sys/fmutex.h> */
#include <sys/fmutex.h>
#include <emx/umalloc.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_HEAP
#include <InnoTekLIBC/logstrict.h>

void *_STD(realloc)(void *block, size_t new_size)
{
    LIBCLOG_ENTER("block=%p new_size=%d\n", block, new_size);
    void *pvRet;
    if (block == NULL)
        pvRet = malloc(new_size);
    else
        pvRet = _um_realloc(block, new_size, new_size < 32 ? 4 : 16, 0);
    if (pvRet)
        LIBCLOG_RETURN_P(pvRet);
    LIBCLOG_ERROR_RETURN_P(pvRet);
}
