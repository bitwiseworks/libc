/* $Id: b_fsFileOwnerSetFH.c 3841 2014-03-16 19:46:11Z bird $ */
/** @file
 * kNIX - fchown().
 */

/*
 * Copyright (c) 2005-2014 knut st. osmundsen <bird-src-spam@anduin.net>
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
#include <os2emx.h>
#include "b_fs.h"
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <emx/io.h>
#include <emx/syscalls.h>
#include <limits.h>
#include "syscalls.h"
#include <InnoTekLIBC/libc.h>
#include <InnoTekLIBC/backend.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_BACK_FS
#include <InnoTekLIBC/logstrict.h>



/**
 * Sets the file ownership of a file by filehandle.
 *
 * @returns 0 on success.
 * @returns Negative error code (errno.h) on failure.
 * @param   fh          Handle to file.
 * @param   uid         The user id of the new owner, pass -1 to not change it.
 * @param   gid         The group id of the new group, pass -1 to not change it.
 */
int __libc_Back_fsFileOwnerSetFH(int fh, uid_t uid, gid_t gid)
{
    LIBCLOG_ENTER("fh=%d uid=%ld gid=%ld\n", fh, (long)uid, (long)gid);

    /*
     * Get filehandle.
     */
    PLIBCFH pFH;
    int rc = __libc_FHEx(fh, &pFH);
    if (rc)
        LIBCLOG_ERROR_RETURN_INT(rc);

    /*
     * Check the type.
     */
    switch (pFH->fFlags & __LIBC_FH_TYPEMASK)
    {
        /* fail */
        case F_SOCKET:
        case F_PIPE: /* treat as socket for now */
            LIBCLOG_ERROR_RETURN_INT(-EINVAL);
        /* ignore */
        case F_DEV:
            LIBCLOG_RETURN_INT(0);

        /* use the path access. */
        case F_DIR:
            if (__predict_false(!pFH->pszNativePath))
                LIBCLOG_ERROR_RETURN_INT(-EINVAL);
            rc = __libc_back_fsNativeFileOwnerSet(pFH->pszNativePath, uid, gid);
            if (rc)
                LIBCLOG_ERROR_RETURN_INT(rc);
            LIBCLOG_RETURN_INT(rc);

        /* treat */
        default:
        case F_FILE:
            break;
    }

    if (!pFH->pOps)
    {
        int fUnixEAs = !__libc_gfNoUnix
                    && pFH->pFsInfo
                    && pFH->pFsInfo->fUnixEAs;
        rc = __libc_back_fsNativeFileOwnerSetCommon(fh, pFH->pszNativePath, fUnixEAs, uid, gid);
        if (rc)
        {
            rc = -__libc_native2errno(rc);
            LIBCLOG_ERROR_RETURN_INT(rc);
        }
    }
    else
    {
        /*
         * Non-standard handle - fail.
         */
        LIBCLOG_ERROR_RETURN_INT(-EOPNOTSUPP);
    }

    LIBCLOG_RETURN_INT(0);
}

