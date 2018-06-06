/* $Id: _realrealpath.c 3771 2012-03-15 20:06:45Z bird $ */
/** @file
 *
 * _realrealpath().
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
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/syslimits.h>
#include <InnoTekLIBC/backend.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_MISC
#include <InnoTekLIBC/logstrict.h>


/**
 * Gets the absolute native path.
 *
 * This is used when path and such is passed on to non-kLibC processes.
 *
 * @returns Pointer to the resolved path.
 * @returns NULL and errno on failure.
 * @param   pszPath     The path to resolve
 * @param   pszResolved Where to put the resolved path.  If NULL, a fitting
 *                      buffer is malloc'ed.
 * @param   cbResolved  Size of the passed in buffer. Minimum size of an
 *                      allocated buffer.
 */
char	*_realrealpath(const char *pszPath, char *pszResolved, size_t cbResolved)
{
    LIBCLOG_ENTER("pszPath=%p:{%s} pszResolved=%p cbResolved=%d\n", (void *)pszPath, pszPath, pszResolved, cbResolved);

    if (!pszPath)
    {
        errno = EINVAL;
        LIBCLOG_ERROR_RETURN(NULL, "ret NULL - pszPath is NULL!\n");
    }

    int rc;
    if (!pszResolved)
    {
        char *psz = malloc(PATH_MAX);
        if (psz)
        {
            rc = __libc_Back_fsPathResolve(pszPath, psz, PATH_MAX, __LIBC_BACKFS_FLAGS_RESOLVE_NATIVE | __LIBC_BACKFS_FLAGS_RESOLVE_FULL_MAYBE);
            if (!rc)
            {
                char *pszOld = psz;
                int cch = strlen(psz) + 1;
                if (cch < cbResolved)
                    cch = cbResolved;
                psz = realloc(psz, cch);
                if (!psz)
                    psz = pszOld;
                LIBCLOG_RETURN_MSG(psz, "ret %p:{%s}\n", psz, psz);
            }
            free(psz);
        }
        else
            rc = -ENOMEM;
        errno = -rc;
        LIBCLOG_ERROR_RETURN_P(NULL);
    }

    char *pszRet = pszResolved;
    rc = __libc_Back_fsPathResolve(pszPath, pszResolved, cbResolved, __LIBC_BACKFS_FLAGS_RESOLVE_NATIVE | __LIBC_BACKFS_FLAGS_RESOLVE_FULL_MAYBE);
    if (!rc)
        LIBCLOG_RETURN_MSG(pszRet, "ret %p - pszResolved=%p:{%s}\n", pszRet, pszResolved, pszResolved);
    errno = -rc;
    LIBCLOG_ERROR_RETURN_MSG(NULL, "ret NULL - pszResolved=%p:{%s}\n", pszResolved, pszResolved);
}

