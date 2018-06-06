/* $Id: b_fsDirRemove.c 3353 2007-05-07 02:58:50Z bird $ */
/** @file
 *
 * kNIX - rmdir.
 *
 * Copyright (c) 2004 knut st. osmundsen <bird-src-spam@anduin.net>
 *
 *
 * This file is part of kLIBC.
 *
 * kLIBC is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * kLIBC is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with kLIBC; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */


/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#include "libc-alias.h"
#define INCL_FSMACROS
#define INCL_ERRORS
#include <os2emx.h>
#include "b_fs.h"
#include <errno.h>
#include <sys/stat.h>
#include <InnoTekLIBC/backend.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_BACK_FS
#include <InnoTekLIBC/logstrict.h>


/**
 * Removes a new directory.
 *
 * @returns 0 on success.
 * @returns -1 and errno on failure.
 * @param   pszPath     Path to the directory which is to be removed.
 */
int __libc_Back_fsDirRemove(const char *pszPath)
{
    LIBCLOG_ENTER("pszPath=%s\n", pszPath);
    FS_VAR();

    /*
     * Resolve the path.
     * (Symlinks should cause failure, so don't resolve last component.)
     */
    char szNativePath[PATH_MAX];
    int rc = __libc_back_fsResolve(pszPath, BACKFS_FLAGS_RESOLVE_PARENT | BACKFS_FLAGS_RESOLVE_DIR, &szNativePath[0], NULL);
    if (rc)
        LIBCLOG_RETURN_INT(rc);

    /*
     * Do It.
     */
    FS_SAVE_LOAD();
    rc = DosDeleteDir((PCSZ)&szNativePath[0]);
    FS_RESTORE();
    if (!rc)
        LIBCLOG_RETURN_INT(rc);


    /*
     * - OS/2 returns access denied when the directory contains files 
     *   or it is not a directory. 
     * - OS/2 returns path not found when the directory is actually a file.
     *
     * So, check for directory/other and return failure accordingly.
     */
    if (    rc == ERROR_ACCESS_DENIED
        ||  rc == ERROR_PATH_NOT_FOUND)
    {
        struct stat s;
        rc = __libc_back_fsNativeFileStat(&szNativePath[0], &s);
        if (!rc)
            rc = S_ISDIR(s.st_mode) ? -ENOTEMPTY : -ENOTDIR;
    }
    else
        rc = -__libc_native2errno(rc);
    LIBCLOG_ERROR_RETURN_INT(rc);
}

