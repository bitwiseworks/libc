/* $Id: $ */
/** @file
 *
 * LIBC exit().
 *
 * Copyright (c) 2005 knut st. osmundsen <bird@anduin.net>
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
#include <emx/startup.h>
#include <InnotekLIBC/atexit.h>
#define __LIBC_LOG_GROUP  __LIBC_LOG_GRP_INITTERM
#include <InnotekLIBC/logstrict.h>


/*******************************************************************************
*   Global Variables                                                           *
*******************************************************************************/
/** Static exit list entry. */
static __LIBC_ATEXITCHUNK gAtExitChunk;
/** Pointer to the head of the exit list chain - LIFO. */
__LIBC_PATEXITCHUNK __libc_gAtExitHead = &gAtExitChunk;


void _STD(exit)(int ret)
{
    LIBCLOG_ENTER("ret=%d\n", ret);

    /*
     * Call registered at exit callbacks.
     *
     * These are called in reverse registration order and we're
     * removing them from the list as we move along to prevent endless recursions.
     * (It is allowed to call exit() from a handler, but we must not call that handler again!)
     */
    for (__LIBC_PATEXITCHUNK pChunk = __libc_gAtExitHead; pChunk; pChunk = pChunk->pNext)
    {
        while (pChunk->c > 0)
        {
            __LIBC_PATEXIT pCur = &pChunk->a[--pChunk->c];
            switch (pCur->enmType)
            {
                case __LIBC_ATEXITTYPE_ATEXIT:
                    LIBCLOG_MSG("calling atexit: %p()\n", (void *)pCur->u.AtExit.pfnCallback);
                    pCur->u.AtExit.pfnCallback();
                    break;

                case __LIBC_ATEXITTYPE_ONEXIT:
                    LIBCLOG_MSG("calling onexit: %p(%d,%p)\n", (void *)pCur->u.OnExit.pfnCallback, ret, pCur->u.OnExit.pvUser);
                    pCur->u.OnExit.pfnCallback(ret, pCur->u.OnExit.pvUser);
                    break;

                default:
                    break;
            }
        }
    }

    /*
     * Terminate the CRT and do the real exit.
     */
    _CRT_term();
    LIBCLOG_MSG("calling _exit(%d)\n", ret);
    _exit(ret);
}

