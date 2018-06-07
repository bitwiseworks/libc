/* heapchk.c (emx+gcc) -- Copyright (c) 1996-1998 by Eberhard Mattes */

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

int _heapchk(void)
{
    LIBCLOG_ENTER("\n");
    _UM_MT_DECL
    int rc1, rc2;
    Heap_t    heap_reg = _UM_DEFAULT_REGULAR_HEAP;
    Heap_t    heap_tiled = _UM_DEFAULT_TILED_HEAP;

    /* Initialize the heap pointers, in case _heapchk() is called by a
       new thread before malloc(). */

    if (heap_reg == NULL)
        heap_reg = _um_init_default_regular_heap();
    if (heap_tiled == NULL)
        heap_tiled = _um_init_default_tiled_heap();

    /* First check the regular heap. */

    rc1 = _uheapchk(heap_reg);
    if (rc1 != _HEAPOK && rc1 != _HEAPEMPTY)
        LIBCLOG_ERROR_RETURN_INT(rc1);

    /* If there's no tiled heap or if it's identical to the regular
       heap, return the regular heap's status. */

    if (heap_reg == heap_tiled || heap_tiled == NULL)
        LIBCLOG_RETURN_INT(rc1);

    /* Check the tiled heap.  Do not return _HEAPEMPTY if any of the two
       heaps is non-empty. */

    rc2 = _uheapchk (heap_tiled);
    if (rc2 == _HEAPEMPTY)
        LIBCLOG_RETURN_INT(rc1);
    if (rc2 != _HEAPOK && rc2 != _HEAPEMPTY)
        LIBCLOG_RETURN_INT(rc2);
    LIBCLOG_ERROR_RETURN_INT(rc2);
}
