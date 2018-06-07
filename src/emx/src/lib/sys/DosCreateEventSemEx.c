/* $Id: DosCreateEventSemEx.c 2254 2005-07-17 12:25:44Z bird $ */
/** @file
 *
 * DosCreateEventSemEx.
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
 * Extended DosCreateEventSem() which will make sure the created semaphore is
 * opened in a forked process with the same handle and state.
 */
APIRET APIENTRY DosCreateEventSemEx(PSZ pszName, PHEV phev, ULONG flAttr, BOOL32 fState)
{
    LIBCLOG_ENTER("pszName=%p:{'%s'} phev=%p flAttr=%#lx fState=%ld\n", pszName, pszName, (void *)phev, flAttr, fState);
    DOSEXTYPE   enmType;
    PDOSEX      pDosEx;
    HEV         hev;
    int         rc;
    FS_VAR();

    /*
     * Create the semaphore.
     */
    FS_SAVE_LOAD();
    rc = DosCreateEventSem(pszName, phev, flAttr, fState);
    if (rc)
    {
        FS_RESTORE();
        LIBCLOG_ERROR_RETURN_INT(rc);
    }

    /*
     * Allocate record.
     */
    hev = *phev;
    enmType = pszName || (flAttr & DC_SEM_SHARED) ? DOSEX_TYPE_EVENT_OPEN : DOSEX_TYPE_EVENT_CREATE;
    pDosEx = __libc_dosexAlloc(enmType);
    if (!pDosEx)
    {
        DosCloseEventSem(hev);
        FS_RESTORE();
        LIBCLOG_ERROR_RETURN_INT(ERROR_NOT_ENOUGH_MEMORY);
    }

    /*
     * Initialize record.
     */
    if (enmType == DOSEX_TYPE_EVENT_CREATE)
    {
        pDosEx->u.EventCreate.hev           = hev;
        pDosEx->u.EventCreate.flFlags       = flAttr;
        if (fState)
            pDosEx->u.EventCreate.fInitialState = 1;
    }
    else
    {
        pDosEx->u.EventOpen.hev     = hev;
#ifdef PER_PROCESS_OPEN_COUNTS
        pDosEx->u.EventOpen.cOpens  = 1;
#endif
    }

    FS_RESTORE();
    LIBCLOG_RETURN_MSG(0, "ret 0 *phev=%#lx\n", hev);
}

