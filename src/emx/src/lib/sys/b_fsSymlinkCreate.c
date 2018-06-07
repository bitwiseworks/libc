/* $Id: b_fsSymlinkCreate.c 2254 2005-07-17 12:25:44Z bird $ */
/** @file
 *
 * LIBC SYS Backend - symlink.
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
#include "b_fs.h"
#include <string.h>
#include <errno.h>
#include <InnoTekLIBC/backend.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_BACK_FS
#include <InnoTekLIBC/logstrict.h>


/**
 * Creates a symbolic link.
 *
 * @returns 0 on success.
 * @returns Negative error code (errno.h) on failure.
 * @param   pszTarget   The target of the symlink link.
 * @param   pszSymlink  The path to the symbolic link to create.
 */
int __libc_Back_fsSymlinkCreate(const char *pszTarget, const char *pszSymlink)
{
    LIBCLOG_ENTER("pszTarget=%p:{%s} pszSymlink=%p:{%s}\n", (void *)pszTarget, pszTarget, (void *)pszSymlink, pszSymlink);

    /*
     * Resolve the path and call native worker.
     */
    char szNativePath[PATH_MAX];
    int rc = __libc_back_fsResolve(pszSymlink, BACKFS_FLAGS_RESOLVE_PARENT | BACKFS_FLAGS_RESOLVE_DIR_MAYBE, &szNativePath[0], NULL);
    if (!rc)
        rc = __libc_back_fsNativeSymlinkCreate(pszTarget, &szNativePath[0]);

    if (!rc)
        LIBCLOG_RETURN_INT(rc);
    LIBCLOG_ERROR_RETURN_INT(rc);
}


