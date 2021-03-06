/* $Id: b_fsSymlinkRead.c 2254 2005-07-17 12:25:44Z bird $ */
/** @file
 *
 * LIBC SYS Backend - readlink.
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
 * Reads the content of a symbolic link.
 *
 * This is weird interface as it will return a truncated result if not
 * enough buffer space. It is also weird in that there is no string
 * terminator.
 *
 * @returns Number of bytes returned in pachBuf.
 * @returns Negative error code (errno.h) on failure.
 * @param   pszPath     The path to the symlink directory.
 * @param   pachBuf     Where to store the symlink value.
 * @param   cchBuf      Size of buffer.
 */
int __libc_Back_fsSymlinkRead(const char *pszPath, char *pachBuf, size_t cchBuf)
{
    LIBCLOG_ENTER("pszPath=%p:{%s} pachBuf=%p cchBuf=%d\n", (void *)pszPath, pszPath, (void *)pachBuf, cchBuf);

    /*
     * Resolve the path.
     */
    char szNativePath[PATH_MAX];
    int rc = __libc_back_fsResolve(pszPath, BACKFS_FLAGS_RESOLVE_FULL_SYMLINK, &szNativePath[0], NULL);
    if (!rc)
        rc = __libc_back_fsNativeSymlinkRead(szNativePath, pachBuf, cchBuf);

    if (rc >= 0)
        LIBCLOG_RETURN_MSG(rc, "ret %d pachBuf=:{%.*s}\n", rc, rc > 0 ? rc : 0, pachBuf);
    LIBCLOG_ERROR_RETURN_MSG(rc, "ret %d\n", rc);
}

