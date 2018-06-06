/* $Id: b_fsPathResolve.c 3914 2014-10-24 14:01:38Z bird $ */
/** @file
 *
 * LIBC SYS Backend - realpath.
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
#define INCL_BASE
#include <os2emx.h>
#include "b_fs.h"
#include <errno.h>
#include <string.h>
#include <InnoTekLIBC/backend.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_BACK_FS
#include <InnoTekLIBC/logstrict.h>



/**
 * Resolves the path into an canonicalized absolute path.
 *
 * @returns 0 on success.
 * @returns Negative error code (errno.h) on failure.
 * @param   pszPath     The path to resolve.
 * @param   pszBuf      Where to store the resolved path.
 * @param   cchBuf      Size of the buffer.
 * @param   fFlags      Combination of __LIBC_BACKFS_FLAGS_RESOLVE_* defines.
 */
int __libc_Back_fsPathResolve(const char *pszPath, char *pszBuf, size_t cchBuf, unsigned fFlags)
{
    LIBCLOG_ENTER("pszPath=%p:{%s} pszBuf=%p cchBuf=%d fFlags=%#x\n", (void *)pszPath, pszPath, (void *)pszBuf, cchBuf, fFlags);

    /* lock fs stuff so unixroot don't change. */
    int rc = __libc_back_fsMutexRequest();
    if (rc)
        LIBCLOG_ERROR_RETURN_INT(rc);

    /*
     * Resolve to native path.
     */
    int             fInUnixTree = 0;
    unsigned int    fBackFsFlags = fFlags & __LIBC_BACKFS_FLAGS_RESOLVE_FULL_MAYBE
                                 ? BACKFS_FLAGS_RESOLVE_DIR_MAYBE | BACKFS_FLAGS_RESOLVE_FULL_MAYBE
                                 : BACKFS_FLAGS_RESOLVE_DIR_MAYBE | BACKFS_FLAGS_RESOLVE_FULL;
    if (!(fFlags & __LIBC_BACKFS_FLAGS_RESOLVE_DIRECT_BUF))
    {
        char        szNativePath[PATH_MAX];
        szNativePath[0] = szNativePath[1] = szNativePath[2] = szNativePath[3] = '\0';
        rc = __libc_back_fsResolve(pszPath, fBackFsFlags, szNativePath, &fInUnixTree);

        /*
         * Copy the (half) result back to the caller.
         */
        char *pszSrc = &szNativePath[0];
        if (   !(fFlags & __LIBC_BACKFS_FLAGS_RESOLVE_NATIVE)
            && fInUnixTree
            && *pszSrc)
        {
            pszSrc += __libc_gcchUnixRoot;
            LIBC_ASSERTM(*pszSrc == '/', "bogus fInUnixTree flag! pszSrc='%s' whole thing is '%s'\n", pszSrc, szNativePath);
        }
        __libc_back_fsMutexRelease();

        int cch = strlen(pszSrc) + 1;
        if (cch < cchBuf)
            memcpy(pszBuf, pszSrc, cchBuf);
        else if (!rc)
            rc = -ERANGE;
    }
    else
    {
        /*
         * Special case for testing purposes only.
         */
        if (cchBuf >= PATH_MAX)
        {
            rc = __libc_back_fsResolve(pszPath, fBackFsFlags, pszBuf, &fInUnixTree);
            if (   !(fFlags & __LIBC_BACKFS_FLAGS_RESOLVE_NATIVE)
                && fInUnixTree
                && pszBuf)
            {
                memmove(pszBuf, pszBuf + __libc_gcchUnixRoot, strlen(pszBuf) - __libc_gcchUnixRoot + 1);
                LIBC_ASSERTM(*pszBuf== '/', "bogus fInUnixTree flag! pszBuf='%s'\n", pszBuf);
            }
        }
        else
            rc = EINVAL;

        __libc_back_fsMutexRelease();
    }

    if (!rc)
        LIBCLOG_RETURN_MSG(rc, "ret %d pszPath=%p:{%s}\n", rc, (void *)pszPath, pszPath);
    LIBCLOG_ERROR_RETURN_MSG(rc, "ret %d pszPath=%p:{%s}\n", rc, (void *)pszPath, pszPath);
}


