/* $Id: b_fsDirCurrentSet.c 3014 2007-04-07 04:53:35Z bird $ */
/** @file
 *
 * LIBC SYS Backend - chdir.
 *
 * Copyright (c) 2004 knut st. osmundsen <bird@innotek.de>
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
#define INCL_FSMACROS
#include <os2emx.h>
#include "b_fs.h"
#include <InnoTekLIBC/backend.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_BACK_FS
#include <InnoTekLIBC/logstrict.h>


/**
 * Changes the current directory of the process.
 *
 * @returns 0 on success.
 * @returns Negative error code (errno.h) on failure.
 * @param   pszPath     Path to the new current directory of the process.
 * @param   fDrive      Force a change of the current drive too.
 */
int __libc_Back_fsDirCurrentSet(const char *pszPath, int fDrive)
{
    LIBCLOG_ENTER("pszPath=%p:{%s} fDrive=%d\n", (void *)pszPath, pszPath, fDrive);

    /*
     * Lock the fs global state.
     */
    int rc = __libc_back_fsMutexRequest();
    if (rc)
        LIBCLOG_ERROR_RETURN_INT(rc);

    /*
     * Resolve the specified file path.
     */
    int     fInUnixTree;
    char    szNativePath[PATH_MAX];
    rc = __libc_back_fsResolve(pszPath, BACKFS_FLAGS_RESOLVE_FULL | BACKFS_FLAGS_RESOLVE_DIR, &szNativePath[0], &fInUnixTree);
    if (!rc)
    {
        /*
         * Change the current directory.
         *
         * If we change into or out of the unix root, we must make sure the
         * default disk is changed too. We'll only do unix chdir behaviour
         * when in the unix compartement or when fDrive is set.
         */
        FS_VAR_SAVE_LOAD();
        rc = DosSetCurrentDir((PCSZ)&szNativePath[0]);
        if (!rc)
        {
            if (fDrive || __libc_gcchUnixRoot)
            {
                if (fDrive || fInUnixTree != __libc_gfInUnixTree)
                {
                    ULONG ulDrive = szNativePath[0] - (szNativePath[0] >= 'A' && szNativePath[0] <= 'Z' ? 'A' - 1 : 'a' -1);
                    rc = DosSetDefaultDisk(ulDrive);
                    LIBC_ASSERTM(!rc, "DosSetDefaultDisk(%ld) -> %d. drive is %c\n", ulDrive, rc, szNativePath[0]);
                    rc = 0;             /* ignore this kind of errors. */
                    __libc_gfInUnixTree = fInUnixTree;
                }
            }
        }
        else
            rc = -__libc_native2errno(rc);
        FS_RESTORE();
    }

    __libc_back_fsMutexRelease();
    if (!rc)
        LIBCLOG_RETURN_INT(rc);
    LIBCLOG_ERROR_RETURN_INT(rc);
}

