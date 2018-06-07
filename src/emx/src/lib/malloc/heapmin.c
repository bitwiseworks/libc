/* heapmin.c (emx+gcc) -- Copyright (c) 1996-1998 by Eberhard Mattes */

#include "libc-alias.h"
#include <stdlib.h>
#include <stddef.h>
#include <umalloc.h>
#include <sys/builtin.h>        /* For <sys/fmutex.h> */
#include <sys/fmutex.h>
#include <emx/umalloc.h>
#include <InnoTekLIBC/thread.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_HEAP
#include <InnoTekLIBC/logstrict.h>

int _heapmin(void)
{
    LIBCLOG_ENTER("\n");
    _UM_MT_DECL
    Heap_t    heap_reg = _UM_DEFAULT_REGULAR_HEAP;
    if (heap_reg == NULL)
        heap_reg = _um_init_default_regular_heap();
    int rc = _uheapmin(heap_reg);
    LIBCLOG_RETURN_INT(rc);
}
