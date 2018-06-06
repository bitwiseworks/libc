/* $Id: b_fsFileOwnerSet.c 3817 2014-02-19 01:40:34Z bird $ */
/** @file
 * kNIX - chown.
 */

/*
 * Copyright (c) 2004-2014 knut st. osmundsen <bird-src-spam@anduin.net>
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
#include "b_fs.h"
#include <InnoTekLIBC/backend.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_BACK_FS
#include <InnoTekLIBC/logstrict.h>


/**
 * Changes the ownership and/or group of a file or directory.
 *
 * @returns 0 on success.
 * @returns Negative error code (errno.h) on failure.
 * @param   pszPath     Path to the file to modify ownership of, following all
 *                      symbolic links.
 * @param   uid         The user id of the new owner, pass -1 to not change it.
 * @param   gid         The group id of the new group, pass -1 to not change it.
 */
int __libc_Back_fsFileOwnerSet(const char *pszPath, uid_t uid, gid_t gid)
{
    LIBCLOG_ENTER("pszPath=%p:{%s} %ld %ld\n", (void *)pszPath, pszPath, (long)uid, (long)gid);

    /*
     * Resolve the path.
     */
    char szNativePath[PATH_MAX];
    int rc = __libc_back_fsResolve(pszPath, BACKFS_FLAGS_RESOLVE_FULL | BACKFS_FLAGS_RESOLVE_DIR_MAYBE,
                                   szNativePath, NULL);
    if (!rc)
        rc = __libc_back_fsNativeFileOwnerSet(szNativePath, uid, gid);

    if (!rc)
        LIBCLOG_RETURN_INT(rc);
    LIBCLOG_ERROR_RETURN_INT(rc);
}

