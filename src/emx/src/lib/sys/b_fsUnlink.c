/* $Id: b_fsUnlink.c 2542 2006-02-07 02:24:44Z bird $ */
/** @file
 *
 * LIBC SYS Backend - unlink.
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
#define INCL_FSMACROS
#define INCL_ERRORS
#define INCL_DOSFILEMGR
#define INCL_DOSMISC
#include <os2emx.h>
#include "b_fs.h"
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <InnoTekLIBC/backend.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_BACK_FS
#include <InnoTekLIBC/logstrict.h>


/**
 * Unlinks a file, symlink, dev, pipe or socket, but not a directory.
 *
 * @returns 0 on success.
 * @returns Negative error code (errno.h) on failure.
 * @param   pszPath         Path to the filesystem file/dir/symlink/whatever to remove.
 */
int __libc_Back_fsUnlink(const char *pszPath)
{
    LIBCLOG_ENTER("pszPath=%p:{%s}\n", (void *)pszPath, pszPath);

    /*
     * Validate input.
     */
    if (!*pszPath)
        LIBCLOG_ERROR_RETURN_INT(-ENOENT);

    /*
     * Resolve the path.
     */
    char szNativePath[PATH_MAX];
    int rc = __libc_back_fsResolve(pszPath, BACKFS_FLAGS_RESOLVE_FULL_SYMLINK, &szNativePath[0], NULL);
    if (rc)
        LIBCLOG_ERROR_RETURN_INT(rc);

    FS_VAR_SAVE_LOAD();

    /*
     * DosDelete is kind of slow as it always scans the environment (in a R0
     * accessing R3 data safe manner to make matters worse even) for DELDIR.
     * Since only a limited number of programs actually changes this
     * environment block, we assume that it's ok to check the first time if
     * DELDIR processing is enabled or not. If it isn't there, we can use
     * DosForceDelete and save quite a bit of time if we're called some
     * times.
     */
    static int s_fUseForce = 0; /* state: 0 - uninit, 1 - DosForceDelete, -1 - DosDelete */
    if (s_fUseForce == 0)
    {
        PSZ psz = NULL;
        if (DosScanEnv((PCSZ)"DELDIR", &psz) || !psz)
            s_fUseForce = 1;
        else
            s_fUseForce = -1;
    }

    /*
     * We'll attempt delete it as a file first.
     */
    if (s_fUseForce == 1)
        rc = DosForceDelete((PCSZ)&szNativePath[0]);
    else
        rc = DosDelete((PCSZ)&szNativePath[0]);
    if (rc == ERROR_ACCESS_DENIED)
    {
        /*
         * There are three causes here:
         *      1) the file is marked read-only.
         *      2) it's a directory.
         *      3) we are denied access - network, hpfs386 or SES.
         *
         * If it's the first we are subject to race conditions, so we have to retry.
         * The third cause is distiguishable from the two othes by the failing DosSetPathInfo.
         */
        for (unsigned i = 0; (rc == ERROR_ACCESS_DENIED || rc == ERROR_PATH_NOT_FOUND) && i < 2; i++)
        {
            FILESTATUS3 fsts3;
            rc = DosQueryPathInfo((PCSZ)&szNativePath[0], FIL_STANDARD, &fsts3, sizeof(fsts3));
            if (!rc)
            {
                /* Directory? */
                if ((fsts3.attrFile & FILE_DIRECTORY) != 0)
                {
                    rc = ERROR_DIRECTORY;
                    break;
                }

                /* turn of the read-only attribute */
                if (fsts3.attrFile & FILE_READONLY)
                {
                    fsts3.attrFile &= ~FILE_READONLY;
                    rc = DosSetPathInfo((PCSZ)&szNativePath[0], FIL_STANDARD, &fsts3, sizeof(fsts3), 0);
                    LIBCLOG_MSG("attempt at disabling the R attribute -> %d\n", rc);
                    if (    rc == ERROR_ACCESS_DENIED
                        ||  rc == ERROR_SHARING_VIOLATION)
                    {
                        rc = ERROR_ACCESS_DENIED;
                        break;
                    }
                }

                /* retry */
                if (s_fUseForce == 1)
                    rc = DosForceDelete((PCSZ)&szNativePath[0]);
                else
                    rc = DosDelete((PCSZ)&szNativePath[0]);
            }
            else
            {
                rc = ERROR_ACCESS_DENIED;
                break;
            }
        }

        /*
         * OS/2 returns access denied when the directory
         * contains files or it is not a directory. Check for
         * directory/other and return failure accordingly.
         */
        if (rc)
        {
            if (rc == ERROR_ACCESS_DENIED)
                rc = -EACCES;
            else if (rc == ERROR_DIRECTORY)
                rc = -EISDIR;
            else
                rc = -__libc_native2errno(rc);
        }
    }
    else if (rc)
        rc = -__libc_native2errno(rc);

    FS_RESTORE();
    if (!rc)
        LIBCLOG_RETURN_INT(rc);
    LIBCLOG_ERROR_RETURN_INT(rc);
}

