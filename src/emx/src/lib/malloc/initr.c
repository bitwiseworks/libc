/* initr.c (emx+gcc) -- Copyright (c) 1996 by Eberhard Mattes */


/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#include "libc-alias.h"
#include <umalloc.h>
#include <emx/umalloc.h>
#include <sys/smutex.h>
#include <InnoTekLIBC/thread.h>
#include <InnoTekLIBC/fork.h>
#define __LIBC_LOG_GROUP    __LIBC_LOG_GRP_HEAP
#include <InnoTekLIBC/logstrict.h>


/*******************************************************************************
*   Global Variables                                                           *
*******************************************************************************/
/** This is the default regular heap. */
Heap_t      _um_regular_heap;

/** Fork cleanup indicator. */
static int  gfForkCleanupDone;


/*******************************************************************************
*   Internal Functions                                                         *
*******************************************************************************/
static int  umForkParent1(__LIBC_PFORKHANDLE pForkHandle, __LIBC_FORKOP enmOperation);
static void umForkCompletion(void *pvArg, int rc, __LIBC_FORKCTX enmCtx);



/**
 * Sets the default regular heap for the current thread and perhaps
 * even the system.
 *
 * Now because of the voting procedure we do not set the _um_regular_heap nor
 * the per thread variable until the voting is completed. There for make sure
 * to use the returned value not the one in the per thread or system variables!
 *
 * @returns Default heap for the thread.
 */
Heap_t _um_init_default_regular_heap(void)
{
    LIBCLOG_ENTER("\n");
    _UM_MT_DECL
    static _smutex  lock;
    Heap_t          heap_reg = _UM_DEFAULT_REGULAR_HEAP;

    /*
     * Is there actually anything left to be done?
     */
    if (!heap_reg)
    {
        int     vote = __libc_HeapGetResult();
        if (vote >= 0)
        {
            /*
             * Is system wide default heap initiated?
             */
            _smutex_request(&lock);
            heap_reg = _um_regular_heap;
            if (!heap_reg)
            {
                if (vote)
                {
                    /*
                     * Use high memory heap.
                     */
                    _um_regular_heap = heap_reg = _hinitheap();
                }
                else
                {
                    /*
                     * Use low memory heap.
                     */
                    _um_regular_heap = heap_reg = _linitheap();
                }
            }
            _smutex_release(&lock);

            /*
             * Set the per thread default.
             */
            _UM_DEFAULT_REGULAR_HEAP = heap_reg;
        }
        else
        {
            /*
             * The voting is not yet over, so we must return the low heap.
             * We do not set either the per thread or system defaults!
             */
            heap_reg = _um_low_heap;
            if (!heap_reg)
                heap_reg = _linitheap();
        }
    }

    LIBCLOG_RETURN_P(heap_reg);
}


#undef  __LIBC_LOG_GROUP
#define __LIBC_LOG_GROUP    __LIBC_LOG_GRP_FORK

_FORK_PARENT1(0xffffff01, umForkParent1)

/**
 * Parent fork callback.
 *
 * The purpose of this is to lock the three heaps we knows about while
 * we're doing the fork() operation.
 *
 * @returns 0 on success.
 * @returns -errno on failure.
 * @param   pForkHandle     Pointer to fork handle.
 * @param   enmOperation    Fork operation.
 */
static int umForkParent1(__LIBC_PFORKHANDLE pForkHandle, __LIBC_FORKOP enmOperation)
{
    LIBCLOG_ENTER("pForkHandle=%p enmOperation=%d\n", (void *)pForkHandle, enmOperation);
    int rc;

    switch (enmOperation)
    {
        /*
         * Lock the heaps before fork() scheduling an unlocking
         * completion callback after fork() is done.
         */
        case __LIBC_FORK_OP_EXEC_PARENT:
            gfForkCleanupDone = 0;
            rc = pForkHandle->pfnCompletionCallback(pForkHandle, umForkCompletion, NULL, __LIBC_FORK_CTX_BOTH);
            if (rc >= 0)
            {
                LIBCLOG_MSG("Locking the heaps.\n");
                if (_um_high_heap)
                    _um_heap_lock(_um_high_heap);
                if (_um_low_heap && _um_low_heap != _um_high_heap)
                    _um_heap_lock(_um_low_heap);
                if (_um_tiled_heap && _um_tiled_heap != _um_low_heap && _um_tiled_heap != _um_high_heap)
                    _um_heap_lock(_um_tiled_heap);
            }
            break;

        default:
            rc = 0;
            break;
    }
    LIBCLOG_RETURN_INT(rc);
}


/**
 * Fork completion callback used to release the locks on the default heaps.
 *
 * @param   pvArg   NULL.
 * @param   rc      The fork() result. Negative on failure.
 * @param   enmCtx  The calling context.
 */
static void umForkCompletion(void *pvArg, int rc, __LIBC_FORKCTX enmCtx)
{
    LIBCLOG_ENTER("pvArg=%p rc=%d enmCtx=%d - gfForkCleanupDone=%d\n", pvArg, rc, enmCtx, gfForkCleanupDone);

    if (!gfForkCleanupDone)
    {
        LIBCLOG_MSG("Unlocking the heaps.\n");
        if (enmCtx == __LIBC_FORK_CTX_PARENT)
        {
            if (_um_high_heap)
                _um_heap_unlock(_um_high_heap);
            if (_um_low_heap && _um_low_heap != _um_high_heap)
                _um_heap_unlock(_um_low_heap);
            if (_um_tiled_heap && _um_tiled_heap != _um_low_heap && _um_tiled_heap != _um_high_heap)
                _um_heap_unlock(_um_tiled_heap);
        }
        else
        {
            if (_um_high_heap)
                _fmutex_release_fork(&_um_high_heap->fsem);
            if (_um_low_heap && _um_low_heap != _um_high_heap)
                _fmutex_release_fork(&_um_low_heap->fsem);
            if (_um_tiled_heap && _um_tiled_heap != _um_low_heap && _um_tiled_heap != _um_high_heap)
                _fmutex_release_fork(&_um_tiled_heap->fsem);
        }
        gfForkCleanupDone = 1;
    }
    LIBCLOG_RETURN_VOID();
}

