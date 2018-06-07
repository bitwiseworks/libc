/* initt.c (emx+gcc) -- Copyright (c) 1996 by Eberhard Mattes */

#include "libc-alias.h"
#include <emx/umalloc.h>
#include <sys/smutex.h>
#include <InnoTekLIBC/thread.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_HEAP
#include <InnoTekLIBC/logstrict.h>

/** This is the default tiled heap. */
Heap_t _um_tiled_heap = NULL;

/**
 * Initialize the default tiled heap for the current thread.
 *
 * Sets _um_tiled_heap to _um_low_heap if zero. The per thread
 * default tiled heap is set to _um_tiled_heap.
 *
 * @returns Heap handle on success.
 */
Heap_t _um_init_default_tiled_heap(void)
{
    LIBCLOG_ENTER("\n");
    _UM_MT_DECL
    Heap_t          heap_tiled;
    static _smutex  lock;

    /*
     * Create the tiled heap if it hasn't been created by another thread
     * between the check of _UM_DEFAULT_TILED_HEAP by our caller and
     * obtaining the semaphore.
     */
    heap_tiled = _UM_DEFAULT_TILED_HEAP;
    if (heap_tiled == NULL)
    {
        _smutex_request(&lock);
        if (_um_tiled_heap == NULL)
        {
            /* Use the low heap also as tiled heap. */
            _linitheap();
            _um_tiled_heap = _um_low_heap;
        }

        _UM_DEFAULT_TILED_HEAP = heap_tiled = _um_tiled_heap;
        _smutex_release(&lock);
    }

    LIBCLOG_RETURN_P(heap_tiled);
}

