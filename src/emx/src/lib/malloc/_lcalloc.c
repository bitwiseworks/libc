/* $Id: _lcalloc.c 2254 2005-07-17 12:25:44Z bird $ */
/** @file
 *
 * Low Memory Heap - calloc().
 *
 * Copyright (c) 2003 knut st. osmundsen <bird-srcspam@anduin.net>
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with This program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */


/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#include "libc-alias.h"
#include <umalloc.h>
#include <emx/umalloc.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_HEAP
#include <InnoTekLIBC/logstrict.h>


/**
 * Allocate a memory block from the low memory heap.
 *
 * @returns Pointer to zero filled block of cItems*cbItem bytes.
 * @returns NULL on failure.
 * @param   cItems  Number of items to allocate.
 * @param   cbItem  Size of one item.
 */
void *_lcalloc(size_t cItems, size_t cbItem)
{
    LIBCLOG_ENTER("cItems=%d cbItem=%d\n", cItems, cbItem);
    if (!_um_low_heap)
        if (!_linitheap())
            LIBCLOG_ERROR_RETURN_P(NULL);
    void *pv = _ucalloc(_um_low_heap, cItems, cbItem);
    if (pv)
        LIBCLOG_RETURN_P(pv);
    LIBCLOG_ERROR_RETURN_P(pv);
}

