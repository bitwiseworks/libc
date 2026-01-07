/* $Id: b_fsPathConf.c 3695 2011-03-15 23:30:51Z bird $ */
/** @file
 *
 * kNIX - fpathconf.
 *
 * Copyright (c) 2011 knut st. osmundsen <bird-src-spam@anduin.net>
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

#include "syscalls.h"
#include "b_fs.h"
#include <InnoTekLIBC/backend.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_BACK_FS
#include <InnoTekLIBC/logstrict.h>

/**
 * Query filesystem configuration information by path.
 *
 * @returns 0 on success.
 * @returns Negative error code (errno.h) on failure.
 * @param   pszPath     Path to query info about.
 * @param   iName       Which path config variable to query.
 * @param   plValue     Where to return the value.
 */
int __libc_Back_fsPathConf(const char *pszPath, int iName, long *plValue)
{
    LIBCLOG_ENTER("pszPath=%p:{%s} iName=%d plValue=%p\n", (void *)pszPath, pszPath, iName, plValue);

    *plValue = 0;

    /*
     * Resolve the path into an fsinfo structure and hand it on to the common
     * function.
     */
    char szNativePath[PATH_MAX];
    int rc = __libc_back_fsResolve(pszPath, BACKFS_FLAGS_RESOLVE_FULL | BACKFS_FLAGS_RESOLVE_DIR_MAYBE, &szNativePath[0], NULL);
    if (!rc)
    {
        __LIBC_PFSINFO pFsInfo = __libc_back_fsInfoObjByPath(szNativePath);
        rc = __libc_back_fsInfoPathConf(pFsInfo, iName, plValue);
        __libc_back_fsInfoObjRelease(pFsInfo);
    }

    LIBCLOG_MIX_RETURN_INT(rc);
}

