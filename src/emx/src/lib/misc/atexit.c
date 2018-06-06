/* $Id: $ */
/** @file
 *
 * LIBC atexit().
 *
 * Copyright (c) 2005-2006 knut st. osmundsen <bird@anduin.net>
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
#ifdef __OS2__
# define INCL_DOSMODULEMGR
# define INCL_ERRORS
# define INCL_FSMACROS
# include <os2emx.h>
#endif
#include <stdlib.h>
#include <InnoTekLIBC/atexit.h>
#include <sys/builtin.h>
#include <emx/umalloc.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_INITTERM
#include <InnoTekLIBC/logstrict.h>


int _STD(atexit)(void (*pfnCallback)(void))
{
    LIBCLOG_ENTER("pfnCallback=%p\n", (void *)pfnCallback);
    __LIBC_PATEXIT pCur = __libc_atexit_new((void *)pfnCallback);
    if (pCur)
    {
        pCur->u.AtExit.pfnCallback = pfnCallback;
        pCur->enmType = __LIBC_ATEXITTYPE_ATEXIT;
        LIBCLOG_RETURN_INT(0);
    }
    LIBCLOG_ERROR_RETURN_INT(-1);
}


/**
 * Allocate a new atexit entry.
 *
 * @returns Pointer to new entry.
 * @returns NULL on failure.
 *
 * @param   pvCallback      The callback address.
 *                          This used to initialize the __LIBC_AT_EXIT::hmod field.
 */
__LIBC_PATEXIT __libc_atexit_new(void *pvCallback)
{
    /*
     * Try find the module this callback belongs to.
     */
    uintptr_t   hmod = 0;
    /** @todo generalize DosQueryModFromEIP and let the (loader) backend do that kind of job. */
#ifdef __OS2__
    HMODULE     hmodOS2;
    ULONG       iObj, offObj;
    FS_VAR_SAVE_LOAD();
    APIRET rc = DosQueryModFromEIP(&hmodOS2, &iObj, 0, NULL, &offObj, (uintptr_t)pvCallback);
    if (rc == NO_ERROR)
        hmod = hmodOS2;
    FS_RESTORE();
#else
    /** @todo port to NT and other target platforms. */
#endif

    /*
     * Search existing chunks.
     * (This is made simple by us not freeing anything.)
     */
    __LIBC_PATEXITCHUNK pChunk;
    for (pChunk = __libc_gAtExitHead; pChunk; pChunk = pChunk->pNext)
    {
        uint32_t i;
        while ((i = pChunk->c) < sizeof(pChunk->a) / sizeof(pChunk->a[0]))
        {
            if (__atomic_cmpxchg32(&pChunk->c, i + 1, i))
            {
                pChunk->a[i].hmod = hmod;
                pChunk->a[i].enmType = __LIBC_ATEXITTYPE_TRANS;
                return &pChunk->a[i];
            }
        }
    }

    /*
     * Add a new chunk.
     */
    pChunk = _hcalloc(1, sizeof(*pChunk));
    if (!pChunk)
        return NULL;
    /** @todo There is a chance that the exit list order could be screwed up here if two threads get here at the same time. (bird, 2006-08-21) */
    pChunk->c = 1;
    pChunk->a[0].hmod = hmod;
    pChunk->a[0].enmType = __LIBC_ATEXITTYPE_TRANS;
    do
    {
        pChunk->pNext = __libc_gAtExitHead;
    } while (__atomic_cmpxchg32((uint32_t volatile *)(void *)&__libc_gAtExitHead, (uint32_t)pChunk, (uint32_t)pChunk->pNext)); /** @todo atomic cmpxchg for ptrs! */

    return &pChunk->a[0];
}


/**
 * Invalidate all atexit and on_exit callback for a
 * module which is being unloaded.
 *
 * @param   hmod        The module handle.
 */
void __libc_atexit_unload(uintptr_t hmod)
{
    /*
     * Search existing chunks.
     * (This is made simple by us not freeing anything.)
     */
    __LIBC_PATEXITCHUNK pChunk;
    for (pChunk = __libc_gAtExitHead; pChunk; pChunk = pChunk->pNext)
    {
        uint32_t i = pChunk->c;
        while (i-- > 0)
        {
            if (    pChunk->a[i].hmod == hmod
                &&  pChunk->a[i].enmType > __LIBC_ATEXITTYPE_FREE
                &&  pChunk->a[i].enmType < __LIBC_ATEXITTYPE_TRANS)
                pChunk->a[i].enmType = __LIBC_ATEXITTYPE_UNLOADED;
        }
    }
}

