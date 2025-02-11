/* $Id: _linitheap.c 2254 2005-07-17 12:25:44Z bird $ */
/** @file
 *
 * Low Memory Heap - init.
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
/** Handle to the Low Memory Heap.
 * This is NULL till _linitheap() is called.
 */
Heap_t _um_low_heap;


/**
 * Initializes the heap which resides in low memory.
 *
 * @returns _um_low_heap on success.
 * @returns doens't return on failure.
 */
Heap_t   _linitheap(void)
{
    LIBCLOG_ENTER("\n");
    /* only one init at a time. */
    static _smutex  lock;
    void *          pvInitial;
    Heap_t          Heap;


    /*
     * Request sem and check if we it's already initialized.
     */
    _smutex_request(&lock);
    Heap = _um_low_heap;
    if (Heap)
    {
        _smutex_release(&lock);
        LIBCLOG_RETURN_P(Heap);
    }

    /*
     * Initialize the sbrk model and allocate the initial heap block.
     */
    _uflags(_UF_SBRK_MODEL, _UF_SBRK_ARBITRARY);
    pvInitial = sbrk(_INITIAL_DEFAULT_HEAP_SIZE);
    if (pvInitial == (void *)-1)
    {
        _smutex_release(&lock);
        _um_abort("_linitheap: sbrk failed!\n");
        LIBCLOG_RETURN_P(NULL);
    }

    /*
     * Create and open the heap.
     */
    Heap = _ucreate2(pvInitial, _INITIAL_DEFAULT_HEAP_SIZE, !_BLOCK_CLEAN,
                     _HEAP_REGULAR,
                     _um_default_alloc,  _um_default_release,
                     _um_default_expand, _um_default_shrink);
    if (Heap == NULL)
    {
        _smutex_release(&lock);
        _um_abort("_linitheap: _ucreate2 failed!\n");
        LIBCLOG_RETURN_P(NULL);
    }
    if (_uopen(Heap) != 0)
    {
        _smutex_release(&lock);
        _um_abort("_linitheap: _uopen(%p) failed!\n", Heap);
        LIBCLOG_RETURN_P(NULL);
    }

    /*
     * Set the global low heap handle release mutex and be gone.
     */
    _um_low_heap = Heap;
    _smutex_release(&lock);

    LIBCLOG_RETURN_P(Heap);
}

