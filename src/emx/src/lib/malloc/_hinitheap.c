/* $Id: _hinitheap.c 2254 2005-07-17 12:25:44Z bird $ */
/** @file
 *
 * High Memory Heap - init.
 *
 * Copyright (c) 2003 knut st. osmundsen <bird-srcspam@anduin.net>
 *
 *
 * This file is part of InnoTek LIBC.
 *
 * InnoTek LIBC is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * InnoTek LIBC is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with InnoTek LIBC; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#include "libc-alias.h"
#include <stdlib.h>
#include <emx/umalloc.h>
#include <sys/smutex.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_HEAP
#include <InnoTekLIBC/logstrict.h>


/*******************************************************************************
*   Global Variables                                                           *
*******************************************************************************/
/** Handle to the High Memory Heap.
 * This is NULL till _hinitheap() is called.
 * It may be equal to _um_low_heap if there isn't any high memory on the machine.
 */
Heap_t _um_high_heap;


/**
 * Initializes the heap which resides in high memory.
 *
 * If we do not support high memory a we'll init and use the low memory heap.
 *
 * @returns _um_high_heap on success.
 * @returns don't return on failure.
 */
Heap_t   _hinitheap(void)
{
    LIBCLOG_ENTER("\n");
    /* only one init at a time. */
    static _smutex  lock;
    void *          pvInitial;
    size_t          cbInitial;
    int             fClean;
    Heap_t          Heap;


    /*
     * Request sem and check if we it's already initialized.
     */
    _smutex_request(&lock);
    Heap = _um_high_heap;
    if (Heap)
    {
        _smutex_release(&lock);
        LIBCLOG_RETURN_P(Heap);
    }

    /*
     * Highmemory support?
     */
    if (!__libc_HasHighMem())
    {
        _um_high_heap = Heap = _linitheap();
        _smutex_release(&lock);
        LIBCLOG_RETURN_P(Heap);
    }

    /*
     * Allocate the initial heap block.
     */
    cbInitial = _INITIAL_DEFAULT_HEAP_SIZE;
    fClean = _BLOCK_CLEAN;
    pvInitial = __libc_HimemDefaultAlloc(NULL, &cbInitial, &fClean);
    if (!pvInitial)
    {
        _smutex_release(&lock);
        _um_abort("_hinitheap: __libc_HimemDefaultAlloc failed!\n");
        LIBCLOG_RETURN_P(NULL);
    }

    /*
     * Create and open the heap.
     */
    Heap = _ucreate2(pvInitial, cbInitial, fClean,
                     _HEAP_REGULAR | _HEAP_HIGHMEM,
                     __libc_HimemDefaultAlloc,  __libc_HimemDefaultRelease,
                     NULL, NULL);
    if (Heap == NULL)
    {
        _smutex_release(&lock);
        _um_abort("_hinitheap: _ucreate2 failed!\n");
        LIBCLOG_RETURN_P(NULL);
    }
    if (_uopen(Heap) != 0)
    {
        _smutex_release(&lock);
        _um_abort("_hinitheap: _uopen(%p) failed!\n", Heap);
        LIBCLOG_RETURN_P(NULL);
    }

    /*
     * Set the global low heap handle release mutex and be gone.
     */
    _um_high_heap = Heap;
    _smutex_release(&lock);

    LIBCLOG_RETURN_P(Heap);
}

