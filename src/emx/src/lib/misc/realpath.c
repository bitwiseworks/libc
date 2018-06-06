/* $Id: realpath.c 3771 2012-03-15 20:06:45Z bird $ */
/** @file
 *
 * realpath().
 *
 * Copyright (c) 2004 knut st. osmundsen <bird-srcspam@anduin.net>
 *
 *
 * This file is part of Innotek LIBC.
 *
 * Innotek LIBC is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Innotek LIBC is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with Innotek LIBC; if not, write to the Free Software
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
 * Gets the absolute path.
 * The returned path contains no symlinks, '.', '..' or extra slashes.
 *
 * @returns Pointer to the resolved path.
 * @returns NULL and errno on failure.
 * @param   path            The path to resolve
 * @param   resolved_path   Where to put the resolved path.
 *                          If NULL a fitting buffer is malloc'ed.
 */
char	*_STD(realpath)(const char *path, char resolved_path[])
{
    LIBCLOG_ENTER("path=%p:{%s} resolved_path=%p\n", (void *)path, path, resolved_path);

    if (!path)
    {
        errno = EINVAL;
        LIBCLOG_ERROR_RETURN(NULL, "ret NULL - path is NULL!\n");
    }

    int rc;
    if (!resolved_path)
    {
        char *psz = malloc(PATH_MAX);
        if (psz)
        {
            rc = __libc_Back_fsPathResolve(path, psz, PATH_MAX, __LIBC_BACKFS_FLAGS_RESOLVE_FULL_MAYBE);
            if (!rc)
            {
                int cch = strlen(psz) + 1;
                char *pszOld = psz;
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
        LIBCLOG_RETURN_P(NULL);
    }

    char *pszRet = resolved_path;
    rc = __libc_Back_fsPathResolve(path, resolved_path, PATH_MAX, __LIBC_BACKFS_FLAGS_RESOLVE_FULL_MAYBE);
    if (!rc)
        LIBCLOG_RETURN_MSG(pszRet, "ret %p - resolved_path=%p:{%s}\n", pszRet, resolved_path, resolved_path);
    errno = -rc;
    LIBCLOG_ERROR_RETURN_MSG(NULL, "ret NULL - resolved_path=%p:{%s}\n", resolved_path, resolved_path);
}

