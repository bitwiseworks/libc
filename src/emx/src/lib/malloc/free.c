/* free.c (emx+gcc) -- Copyright (c) 1996 by Eberhard Mattes */

#include "libc-alias.h"
#include <stdlib.h>
#include <stddef.h>
#include <umalloc.h>
#include <sys/builtin.h>        /* For <sys/fmutex.h> */
#include <sys/fmutex.h>
#include <emx/umalloc.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_HEAP
#include <InnoTekLIBC/logstrict.h>


void _STD(free)(void *block)
{
    LIBCLOG_ENTER("block=%p\n", block);
    _um_free_maybe_lock(block, 1);
    LIBCLOG_RETURN_VOID();
}
