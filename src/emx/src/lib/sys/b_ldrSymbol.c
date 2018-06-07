/* $Id: b_ldrSymbol.c 3811 2014-02-16 22:56:50Z bird $ */
/** @file
 *
 * LIBC SYS Backend - dlsym.
 *
 * Copyright (c) 2004-2014 knut st. osmundsen <bird@innotek.de>
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
#define INCL_BASE
#define INCL_FSMACROS
#include <os2emx.h>
#include <InnoTekLIBC/backend.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_BACK_LDR
#include <InnoTekLIBC/logstrict.h>


/**
 * Finds a symbol in an open shared library.
 *
 * @returns 0 on success.
 * @returns Native error number.
 * @param   pvModule        Module handle returned by __libc_Back_ldrOpen();
 * @param   pszSymbol       Name of the symbol we're to find in pvModule.
 * @param   ppfn            Where to store the symbol address.
 */
int __libc_Back_ldrSymbol(void *pvHandle,  const char *pszSymbol, void **ppfn)
{
    LIBCLOG_ENTER("pvHandle=%p pszSymbol=%p:{%s} ppfn=%p\n", pvHandle,
                  (void *)pszSymbol, (uintptr_t)pszSymbol >= 10000 ? pszSymbol : "<ordinal>", (void *)ppfn);
    int rc;
    if (pvHandle == __LIBC_BACK_LDR_GLOBAL)
    {
        /* Pretending there aren't anything in the global name space for now.
           Later we could probably do a QS_MTE assisted scan of loaded modules
           and what not.  */
        rc = ERROR_PROC_NOT_FOUND;
    }
    else
    {
        PFN     pfn;
        FS_VAR();
        FS_SAVE_LOAD();
        if ((uintptr_t)pszSymbol < 10000)
            rc = DosQueryProcAddr((HMODULE)pvHandle, (uintptr_t)pszSymbol, NULL, &pfn);
        else
            rc = DosQueryProcAddr((HMODULE)pvHandle, 0, (PCSZ)pszSymbol, &pfn);
        FS_RESTORE();
        if (!rc)
        {
            *ppfn = (void *)pfn;
            LIBCLOG_RETURN_MSG(0, "ret 0 *ppfn=%p\n", (void *)pfn);
        }
    }
    LIBCLOG_ERROR_RETURN_INT(rc);
}



