/* $Id: DosCreateMutexSemEx.c 2254 2005-07-17 12:25:44Z bird $ */
/** @file
 *
 * DosCreateMutexSemEx.
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
#define INCL_FSMACROS
#define INCL_EXAPIS
#include <os2emx.h>
#define __LIBC_LOG_GROUP    __LIBC_LOG_GRP_DOSEX
#include <InnoTekLIBC/logstrict.h>
#include "DosEx.h"


/**
 * Extended DosCreateMutexSem() which will make sure the created semaphore is
 * opened in a forked process with the same handle.
 */
APIRET APIENTRY DosCreateMutexSemEx(PSZ pszName, PHMTX phmtx, ULONG flAttr, BOOL32 fState)
{
    LIBCLOG_ENTER("pszName=%p:{'%s'} phmtx=%p flAttr=%#lx fState=%ld\n", pszName, pszName, (void *)phmtx, flAttr, fState);
    DOSEXTYPE   enmType;
    PDOSEX      pDosEx;
    HMTX        hmtx;
    int         rc;
    FS_VAR();

    /*
     * Create the semaphore.
     */
    FS_SAVE_LOAD();
    rc = DosCreateMutexSem(pszName, phmtx, flAttr, fState);
    if (rc)
    {
        FS_RESTORE();
        LIBCLOG_ERROR_RETURN_INT(rc);
    }

    /*
     * Allocate record.
     */
    hmtx = *phmtx;
    enmType = pszName || (flAttr & DC_SEM_SHARED) ? DOSEX_TYPE_MUTEX_OPEN : DOSEX_TYPE_MUTEX_CREATE;
    pDosEx = __libc_dosexAlloc(enmType);
    if (!pDosEx)
    {
        DosCloseMutexSem(hmtx);
        FS_RESTORE();
        LIBCLOG_ERROR_RETURN_INT(ERROR_NOT_ENOUGH_MEMORY);
    }

    /*
     * Initialize record.
     */
    if (enmType == DOSEX_TYPE_MUTEX_CREATE)
    {
        pDosEx->u.MutexCreate.hmtx          = hmtx;
        pDosEx->u.MutexCreate.flFlags       = flAttr;
        if (fState)
            pDosEx->u.MutexCreate.fInitialState = 1;
    }
    else
    {
        pDosEx->u.MutexOpen.hmtx    = hmtx;
#ifdef PER_PROCESS_OPEN_COUNTS
        pDosEx->u.MutexOpen.cOpens  = 1;
#endif
    }

    FS_RESTORE();
    LIBCLOG_RETURN_MSG(0, "ret 0 *phmtx=%#lx\n", hmtx);
}

