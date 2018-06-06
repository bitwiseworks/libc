/* $Id: posix_memalign.c 3722 2011-03-17 21:33:44Z bird $ */
/** @file
 *
 * posix_memalign() implementation.
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
#include <stddef.h>
#include <errno.h>
#include <umalloc.h>
#include <emx/umalloc.h>
#include <InnoTekLIBC/thread.h>
#include <assert.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_HEAP
#include <InnoTekLIBC/logstrict.h>

/**
 * Aligned allocation.
 * @returns 0 on success.
 * @returns -1 on failure. errno set.
 * @param   memptr      Where to store the pointer to the allocated memory.
 * @param   alignment   How to align the memory. Must be power of two.
 * @param   size        Number of bytes to allocate.
 * @todo    Split this into a _umalloc_aligned() worker perhaps.
 */
int _STD(posix_memalign)(void **memptr, size_t alignment, size_t size)
{
    LIBCLOG_ENTER("memptr=%p alignment=%d size=%d\n", (void *)memptr, alignment, size);
    _UM_MT_DECL
    void *      pv;
    unsigned    flags;
    Heap_t      Heap = _UM_DEFAULT_REGULAR_HEAP;

    /*
     * Input values.
     */
    *memptr = NULL;                     /* touch it so we crash here. */
    if (   (alignment & (alignment - 1)) != 0
        || alignment < sizeof(void *))
    {
        errno = EINVAL;
        LIBCLOG_ERROR_RETURN_INT(-1);
    }

    /*
     * Get, optionally initialize, and assert the validity of the heap.
     */
    if (Heap == NULL)
        Heap = _um_init_default_regular_heap();

    assert(Heap->magic == _UM_MAGIC_HEAP);
    if (Heap->magic != _UM_MAGIC_HEAP)
    {
        errno = EDOOFUS;
        LIBCLOG_ERROR_RETURN_INT(-1);
    }

    /*
     * Do the allocation.
     */
    _um_heap_lock(Heap);
    flags = 0;
    if (Heap->type & _HEAP_TILED)
        flags |= _UMFI_TILED;
    pv = _um_alloc_no_lock(Heap, size, alignment, flags);
    _um_heap_unlock(Heap);

    /*
     * Store the result and return.
     */
    *memptr = pv;
    if (pv)
        LIBCLOG_RETURN_INT(0);
    errno = ENOMEM;
    LIBCLOG_ERROR_RETURN_INT(-1);
}

