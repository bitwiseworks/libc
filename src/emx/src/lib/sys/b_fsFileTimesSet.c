/* $Id: b_fsFileTimesSet.c 2254 2005-07-17 12:25:44Z bird $ */
/** @file
 *
 * LIBC SYS Backend - utimes().
 *
 * Copyright (c) 2005 knut st. osmundsen <bird@anduin.net>
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
#include <sys/stat.h>
#include <InnoTekLIBC/backend.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_BACK_FS
#include <InnoTekLIBC/logstrict.h>


/**
 * Sets the file the times of a file.
 *
 * @returns 0 on success.
 * @returns Negative error code (errno.h) on failure.
 * @param   pszPath The path to the file to set the times of.
 * @param   paTimes Two timevalue structures. If NULL the current time is used.
 */
int __libc_Back_fsFileTimesSet(const char *pszPath, const struct timeval *paTimes)
{
    LIBCLOG_ENTER("pszPath=%p:{%s} paTimes=%p:{{.tv_sec=%d, .tv_usec=%ld}, {.tv_sec=%d, .tv_usec=%ld}}\n",
                  (void *)pszPath, pszPath, (void *)paTimes,
                  paTimes ? paTimes[0].tv_sec : ~0, paTimes ? paTimes[0].tv_usec : ~0,
                  paTimes ? paTimes[1].tv_sec : ~0, paTimes ? paTimes[1].tv_usec : ~0);

    /*
     * Resolve the path.
     */
    char szNativePath[PATH_MAX];
    int rc = __libc_back_fsResolve(pszPath, BACKFS_FLAGS_RESOLVE_FULL | BACKFS_FLAGS_RESOLVE_DIR_MAYBE, szNativePath, NULL);
    if (!rc)
        rc = __libc_back_fsNativeFileTimesSet(szNativePath, paTimes);

    if (!rc)
        LIBCLOG_RETURN_INT(rc);
    LIBCLOG_ERROR_RETURN_INT(rc);
}

