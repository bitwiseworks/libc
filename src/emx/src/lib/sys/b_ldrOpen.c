/* $Id: b_ldrOpen.c 3811 2014-02-16 22:56:50Z bird $ */
/** @file
 *
 * LIBC SYS Backend - dlopen.
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
#define INCL_EXAPIS
#include <os2emx.h>
#include <string.h>
#include <InnoTekLIBC/backend.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_BACK_LDR
#include <InnoTekLIBC/logstrict.h>
#include "b_fs.h"
#include "syscalls.h"


/**
 * Opens a shared library.
 * @returns 0 on success.
 * @returns Native error number.
 * @param   pszLibrary      Name of library to load.
 * @param   fFlags          Flags - ignored.
 * @param   ppvModule       Where to store the handle.
 * @param   pszError        Where to store error information.
 * @param   cchError        Size of error buffer.
 */
int  __libc_Back_ldrOpen(const char *pszLibrary, int fFlags, void **ppvModule, char *pszError, size_t cchError)
{
    LIBCLOG_ENTER("pszLibrary=%p:{%s} fFlags=%#x ppvModule=%p pszError=%p cchError=%d\n",
                  (void *)pszLibrary, pszLibrary, fFlags, (void *)ppvModule, (void *)pszError, cchError);
    const char *pszNativePath;
    char        szNativePath[PATH_MAX];
    int         rc;

    /*
     * Sepecial case where we're requested to open the global namespace object
     * for the process.
     */
    if (!pszLibrary)
    {
        *ppvModule = __LIBC_BACK_LDR_GLOBAL;
        LIBCLOG_RETURN_INT(0);
    }

    /*
     * Resolve the path if one is given.  Ignore failures to resolve the file
     * name is it may lack the extension (I think) - DosLoadModule will fail,
     * so no problem.
     */
    if (!strpbrk(pszLibrary, ":/\\"))
    {
        pszNativePath = pszLibrary; /* no path, don't try resolve anything. */
        rc = 0;
    }
    else
    {
        pszNativePath = szNativePath;
        rc = __libc_back_fsResolve(pszLibrary, BACKFS_FLAGS_RESOLVE_FULL_MAYBE, &szNativePath[0], NULL);
    }
    if (rc == 0)
    {
        HMODULE hmod;
        FS_VAR_SAVE_LOAD();
        rc = DosLoadModuleEx((PSZ)pszError, cchError, (PCSZ)pszNativePath, &hmod);
        FS_RESTORE();
        if (!rc)
        {
            *ppvModule = (void *)hmod;
            LIBCLOG_RETURN_INT(0);
        }
    }
    else
        rc = __libc_back_errno2native(-rc);
    LIBCLOG_ERROR_RETURN_INT(rc);
}



