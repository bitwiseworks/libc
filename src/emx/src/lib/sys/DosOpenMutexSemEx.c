/* $Id: DosOpenMutexSemEx.c 2254 2005-07-17 12:25:44Z bird $ */
/** @file
 *
 * DosOpenMutexSemEx.
 *
 * Copyright (c) 2004 knut st. osmundsen <bird-srcspam@anduin.net>
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
#define INCL_ERRORS
#define INCL_DOSSEMAPHORES
#define INCL_EXAPIS
#define INCL_FSMACROS
#include <os2emx.h>
#define __LIBC_LOG_GROUP    __LIBC_LOG_GRP_DOSEX
#include <InnoTekLIBC/logstrict.h>
#include "DosEx.h"


/**
 * Extended DosOpenMutexSem() which will make sure the opened semaphore is
 * opened in a forked process with the same handle.
 */
APIRET APIENTRY DosOpenMutexSemEx(PSZ pszName, PHMTX phmtx)
{
    LIBCLOG_ENTER("pszName=%p:{'%s'} phmtx=%p:{%#lx}\n", pszName, pszName, (void *)phmtx, *phmtx);
    PDOSEX      pDosEx;
    HMTX        hmtx;
    int         rc;
    FS_VAR();

    /*
     * Open the semaphore.
     */
    FS_SAVE_LOAD();
    rc = DosOpenMutexSem(pszName, phmtx);
    if (rc)
    {
        FS_RESTORE();
        LIBCLOG_ERROR_RETURN_INT(rc);
    }

    /*
     * See if we have a record for this semaphore already.
     */
    hmtx = *phmtx;
    pDosEx = __libc_dosexFind(DOSEX_TYPE_MUTEX_OPEN, hmtx);
    if (pDosEx)
    {
#ifdef PER_PROCESS_OPEN_COUNTS
        pDosEx->u.MutexOpen.cOpens++;
#endif
        __libc_dosexRelease(pDosEx);
    }
    else
    {
        /*
         * Allocate a new record.
         */
        pDosEx = __libc_dosexAlloc(DOSEX_TYPE_MUTEX_OPEN);
        if (!pDosEx)
        {
            DosCloseMutexSem(hmtx);
            FS_RESTORE();
            LIBCLOG_ERROR_RETURN_INT(ERROR_NOT_ENOUGH_MEMORY);
        }

        /*
         * Initialize record.
         */
        pDosEx->u.MutexOpen.hmtx    = hmtx;
#ifdef PER_PROCESS_OPEN_COUNTS
        pDosEx->u.MutexOpen.cOpens  = 1;
#endif
    }

    FS_RESTORE();
    LIBCLOG_RETURN_MSG(0, "ret 0 *phmtx=%#lx\n", hmtx);
}

