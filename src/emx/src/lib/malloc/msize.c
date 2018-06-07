/* msize.c (emx+gcc) -- Copyright (c) 1996 by Eberhard Mattes */

#include "libc-alias.h"
#include <stdlib.h>
#include <stddef.h>
#include <umalloc.h>
#include <sys/builtin.h>        /* For <sys/fmutex.h> */
#include <sys/fmutex.h>
#include <emx/umalloc.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_HEAP
#include <InnoTekLIBC/logstrict.h>

size_t _msize(const void *block)
{
    LIBCLOG_ENTER("block=%p\n", block);
    size_t cb = block == NULL ? _HDR_FROM_BLOCK(block)->size : 0;
    LIBCLOG_RETURN_INT(cb);
}
