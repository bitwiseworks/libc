/* $Id: _lrealloc.c 2254 2005-07-17 12:25:44Z bird $ */
/** @file
 *
 * Low Memory Heap - realloc().
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
 * Rellocate a memory block from the low memory heap.
 *
 * @returns Pointer to reallocated block on success.
 * @returns NULL if cbNew, pv is freed.
 * @returns NULL on failure (cbNew != 0), pv not touched.
 * @param   pv      Pointer to the memory block to allocate.
 *                  If NULL the function behaves like _lmalloc().
 * @param   cbNew   The new block size.
 *                  If Zero the function behaves like free().
 */
void *_lrealloc(void * pv, size_t cbNew)
{
    LIBCLOG_ENTER("pv=%p cbNew=%d\n", pv, cbNew);
    void *pvRet;
    if (!pv)
        pvRet = _lmalloc(cbNew);
    else
        pvRet = _um_realloc(pv, cbNew, cbNew < 32 ? 4 : 16, 0);
    if (pvRet || !cbNew)
        LIBCLOG_RETURN_P(pvRet);
    LIBCLOG_ERROR_RETURN_P(pvRet);
}

