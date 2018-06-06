/* $Id: DosLoadModuleEx.c 3906 2014-10-23 23:50:44Z bird $ */
/** @file
 *
 * DosCreateEventSemEx.
 *
 * Copyright (c) 2004-2014 knut st. osmundsen <bird-srcspam@anduin.net>
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
#define INCL_DOSMODULEMGR
#define INCL_PRESERVE_REGISTER_MACROS
#define INCL_EXAPIS
#include <os2emx.h>
#define __LIBC_LOG_GROUP    __LIBC_LOG_GRP_DOSEX
#include <InnoTekLIBC/logstrict.h>
#include "DosEx.h"


/**
 * Extended DosLoadModule() which will make sure the loaded module is
 * loaded in a forked process.
 */
APIRET APIENTRY DosLoadModuleEx(PSZ pszObject, ULONG cbObject, PCSZ pszModule, PHMODULE phmod)
{
    LIBCLOG_ENTER("pszObject=%p cbObject=%ld pszModule=%p:{'%s'} phmod=%p\n", pszObject, cbObject, (void *)pszModule, pszModule, (void *)phmod);
    DOSEXTYPE   enmType;
    PDOSEX      pDosEx;
    HMODULE     hmte;
    int         rc;

    /*
     * Create the semaphore.
     */
    PRESERVE_REGS_SAVE_LOAD_SAFE();
    rc = DosLoadModule(pszObject, cbObject, pszModule, phmod);
    PRESERVE_REGS_RESTORE();
    if (rc)
        LIBCLOG_ERROR_RETURN_INT(rc);

    /*
     * Check if already loaded.
     */
    hmte = *phmod;
    pDosEx = __libc_dosexFind(DOSEX_TYPE_LOAD_MODULE, (unsigned)hmte);
    if (pDosEx)
    {
        pDosEx->u.LoadModule.cLoads++;
        __libc_dosexRelease(pDosEx);
    }
    else
    {
        /*
         * Allocate record.
         */
        enmType = DOSEX_TYPE_LOAD_MODULE;
        pDosEx = __libc_dosexAlloc(enmType);
        if (!pDosEx)
        {
            PRESERVE_REGS_SAVE_LOAD_SAFE_AGAIN();
            DosFreeModule(hmte);
            PRESERVE_REGS_RESTORE();
            LIBCLOG_ERROR_RETURN_INT(ERROR_NOT_ENOUGH_MEMORY);
        }

        /*
         * Initialize record.
         */
        pDosEx->u.LoadModule.hmte   = hmte;
        pDosEx->u.LoadModule.cLoads = 1;
    }

    LIBCLOG_RETURN_MSG(0, "ret 0 *phmod=%#lx\n", hmte);
}

