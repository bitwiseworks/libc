/* $Id: b_fsDirCurrentSetFH.c 2323 2005-09-25 11:01:36Z bird $ */
/** @file
 *
 * LIBC SYS Backend - fchdir.
 *
 * Copyright (c) 2005 knut st. osmundsen <bird@innotek.de>
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
#include "b_dir.h"
#include <errno.h>
#include <emx/io.h>
#include <InnoTekLIBC/backend.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_BACK_FS
#include <InnoTekLIBC/logstrict.h>


/**
 * Changes the current directory of the process.
 *
 * @returns 0 on success.
 * @returns Negative error code (errno.h) on failure.
 * @param   fh          The handle of an open directory.
 * @param   fDrive      Force a change of the current drive too.
 */
int __libc_Back_fsDirCurrentSetFH(int fh, int fDrive)
{
    LIBCLOG_ENTER("fh=%d fDrive=%d\n", fh, fDrive);
    FS_VAR();

    /*
     * Validate the file handle.
     */
    __LIBC_PFH pFH;
    int rc = __libc_FHEx(fh, &pFH);
    if (rc)
        LIBCLOG_ERROR_RETURN_INT(rc);
    if ((pFH->fFlags & __LIBC_FH_TYPEMASK) != F_DIR)
        LIBCLOG_ERROR_RETURN_INT(-ENOTDIR);
    __LIBC_PFHDIR pFHDir = (__LIBC_PFHDIR)pFH;

    /*
     * Lock the fs global state.
     */
    rc = __libc_back_fsMutexRequest();
    if (rc)
        LIBCLOG_ERROR_RETURN_INT(rc);

    /*
     * Change the current directory.
     *
     * If we change into or out of the unix root, we must make sure the
     * default disk is changed too. We'll only do unix chdir behaviour
     * when in the unix compartement or when fDrive is set.
     */
    FS_SAVE_LOAD();
    rc = DosSetCurrentDir((PCSZ)pFHDir->Core.pszNativePath);
    if (!rc)
    {
        if (fDrive || __libc_gcchUnixRoot)
        {
            if (fDrive || pFHDir->fInUnixTree != __libc_gfInUnixTree)
            {
                const char *psz = pFHDir->Core.pszNativePath;
                ULONG ulDrive = psz[0] - (psz[0] >= 'A' && psz[0] <= 'Z' ? 'A' - 1 : 'a' -1);
                rc = DosSetDefaultDisk(ulDrive);
                LIBC_ASSERTM(!rc, "DosSetDefaultDisk(%ld) -> %d. drive is %c\n", ulDrive, rc, psz[0]);
                rc = 0;             /* ignore this kind of errors. */
                __libc_gfInUnixTree = pFHDir->fInUnixTree;
            }
        }
    }
    else
        rc = -__libc_native2errno(rc);
    FS_RESTORE();

    __libc_back_fsMutexRelease();
    if (!rc)
        LIBCLOG_RETURN_INT(rc);
    LIBCLOG_ERROR_RETURN_INT(rc);
}

