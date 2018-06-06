/* $Id: b_fsDirChangeRoot.c 2254 2005-07-17 12:25:44Z bird $ */
/** @file
 *
 * LIBC Backend - chroot().
 *
 * Copyright (c) 2004 knut st. osmundsen <bird-srcspam@anduin.net>
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
#include "b_fs.h"
#include <string.h>
#include <InnoTekLIBC/backend.h>
#include <InnoTekLIBC/libc.h>
#define __LIBC_LOG_GROUP    __LIBC_LOG_GRP_BACK_FS
#include <InnoTekLIBC/logstrict.h>



/**
 * Sets or change the unixroot of the current process.
 *
 * @returns 0 on success.
 * @returns -1 and errno on failure.
 * @param   pszNewRoot  The new root.
 */
int __libc_Back_fsDirChangeRoot(const char *pszNewRoot)
{
    LIBCLOG_ENTER("pszNewRoot=%p:{%s}\n", (void *)pszNewRoot, pszNewRoot);

    /*
     * Lock fs data.
     */
    int rc = __libc_back_fsMutexRequest();
    if (rc)
        LIBCLOG_ERROR_RETURN_INT(rc);

    /*
     * Resolve the path to a native path and verifying it in the process.
     */
    char szNativePath[PATH_MAX];
    rc = __libc_back_fsResolve(pszNewRoot, BACKFS_FLAGS_RESOLVE_FULL | BACKFS_FLAGS_RESOLVE_DIR, &szNativePath[0], NULL);
    if (!rc)
    {
        /*
         * Replace the current unix root.
         */
        int cch = strlen(&szNativePath[0]);
        memcpy(__libc_gszUnixRoot, &szNativePath[0], cch + 1);
        __libc_gcchUnixRoot = cch;
        __libc_gfNoUnix     = 0;
        __libc_gfInUnixTree = 0; /** @todo logic for correct __libc_gfInUnixTree update in chroot() operation. */
    }

    __libc_back_fsMutexRelease();
    if (!rc)
        LIBCLOG_RETURN_INT(rc);
    LIBCLOG_ERROR_RETURN_INT(rc);
}

