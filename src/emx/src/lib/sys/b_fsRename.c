/* $Id: b_fsRename.c 3905 2014-10-23 22:35:09Z bird $ */
/** @file
 *
 * LIBC SYS Backend - rename.
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
#define INCL_FSMACROS
#define INCL_ERRORS
#include <os2emx.h>
#include "b_fs.h"
#include <errno.h>
#include <string.h>
#include <InnoTekLIBC/backend.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_BACK_FS
#include <InnoTekLIBC/logstrict.h>


/**
 * Renames a file or directory.
 *
 * @returns 0 on success.
 * @returns Negative error code (errno.h) on failure.
 * @param   pszPathOld      Old file path.
 * @param   pszPathNew      New file path.
 *
 * @remark OS/2 doesn't preform the deletion of the pszPathNew atomically.
 */
int __libc_Back_fsRename(const char *pszPathOld, const char *pszPathNew)
{
    LIBCLOG_ENTER("pszPathOld=%p:{%s} pszPathNew=%p:{%s}\n", (void *)pszPathOld, pszPathOld, (void *)pszPathNew, pszPathNew);
    FS_VAR();

    /*
     * Validate input.
     */
    if (!*pszPathOld || !*pszPathNew)
        LIBCLOG_ERROR_RETURN_INT(-ENOENT);

    /*
     * Resolve the paths.
     */
    char szNativePathOld[PATH_MAX];
    int rc = __libc_back_fsResolve(pszPathOld, BACKFS_FLAGS_RESOLVE_FULL_SYMLINK | BACKFS_FLAGS_RESOLVE_DIR_MAYBE, &szNativePathOld[0], NULL);
    if (rc)
        LIBCLOG_ERROR_RETURN_INT(rc);

    char szNativePathNew[PATH_MAX];
    rc = __libc_back_fsResolve(pszPathNew, BACKFS_FLAGS_RESOLVE_PARENT | BACKFS_FLAGS_RESOLVE_DIR_MAYBE, &szNativePathNew[0], NULL);
    if (rc)
        LIBCLOG_ERROR_RETURN_INT(rc);

    /*
     * Loop for 10 times before we give up and decides to loose the race.
     */
    FS_SAVE_LOAD();
    int cTries = 10;
    for (;;)
    {
        /*
         * Attempt the operation.
         */
        rc = DosMove((PCSZ)&szNativePathOld[0], (PCSZ)&szNativePathNew[0]);
        if (!rc)
            break;
        else
        {
            /*
             * Error codes which *may* be caused by an existing file or directory.
             */
            if (   (    rc == ERROR_ACCESS_DENIED
                    ||  rc == ERROR_SHARING_VIOLATION)
                && --cTries > 0)
            {
                /*
                 * If the paths are identical, we'll simply ignore the error.
                 * (Typically ERROR_SHARING_VIOLATION problem.)
                 */
                if (!strcmp(&szNativePathOld[0], &szNativePathNew[0]))
                    rc = 0;
                else
                {
                    /*
                     * Probe the source and target to see what we should
                     * attempt next.
                     */
                    FILESTATUS3     fsts3Old;
                    int rc2 = DosQueryPathInfo((PCSZ)&szNativePathOld[0], FIL_STANDARD, &fsts3Old, sizeof(fsts3Old));
                    if (rc2)
                    {
                        /*
                         * Hmm, not allowed access to source. Or did it go away?
                         * Anyway, we're done.
                         */
                        rc = -__libc_native2errno(rc2);
                    }
                    else
                    {
                        /*
                         * This is where we stop with ERROR_SHARING_VIOLATION.
                         */
                        if (rc == ERROR_SHARING_VIOLATION)
                            rc = fsts3Old.attrFile & FILE_DIRECTORY ? -EBUSY : -ETXTBSY;
                        else
                        {
                            FILESTATUS3     fsts3New;
                            rc2 = DosQueryPathInfo((PCSZ)&szNativePathNew[0], FIL_STANDARD, &fsts3New, sizeof(fsts3New));
                            if (rc2)
                            {
                                /*
                                 * No target, source must be busy doing something.
                                 */
                                rc = fsts3Old.attrFile & FILE_DIRECTORY ? -EBUSY : -ETXTBSY;
                            }
                            else
                            {
                                if (    (fsts3New.attrFile & FILE_DIRECTORY)
                                    && !(fsts3Old.attrFile & FILE_DIRECTORY))
                                {
                                    /*
                                     * The target is a directory while the source is not.
                                     */
                                    rc = -EISDIR;
                                }
                                else
                                {
                                    /*
                                     * Ok, now we start cooking.
                                     *
                                     * This should've been done atomically, but non-atomically is
                                     * better than not doing anything (usually). We use the loop
                                     * to retry the move operation and handle races with other
                                     * processes somewhat ok...
                                     *
                                     * Try clear the read-only attribute on files before deleting them.
                                     * Directories cannot have read-only attributes on OS/2 it seems.
                                     */
                                    if (fsts3New.attrFile & FILE_DIRECTORY)
                                    {
                                        rc2 = DosDeleteDir((PCSZ)&szNativePathNew[0]);
                                        LIBCLOG_MSG("DosDeleteDir('%s') -> %d\n", szNativePathNew, rc2);
                                        if (!rc2)
                                            continue;
                                        if (rc2 == ERROR_ACCESS_DENIED)
                                            rc = -ENOTEMPTY;
                                        else
                                            rc = -__libc_native2errno(rc2);
                                    }
                                    else
                                    {
                                        if (fsts3New.attrFile & FILE_READONLY)
                                        {
                                            fsts3New.attrFile &= ~FILE_READONLY;
                                            rc2 = DosSetPathInfo((PCSZ)&szNativePathNew[0], FIL_STANDARD, &fsts3New,
                                                                 sizeof(fsts3New), 0 /*fOptions*/);
                                            LIBCLOG_MSG("DosSetPathInfo('%s',read-write) -> %d\n", szNativePathNew, rc2);
                                        }

                                        rc2 = DosDelete((PCSZ)&szNativePathNew[0]);
                                        LIBCLOG_MSG("DosDelete('%s') -> %d\n", szNativePathNew, rc2);
                                        if (!rc2)
                                            continue;
                                        rc = -__libc_native2errno(rc2);
                                    }
                                }
                            }
                        } /* !ERROR_SHARING_VIOLATION */
                    }
                }
            }
            else if (rc == ERROR_DIRECTORY_IN_CDS || rc == ERROR_CIRCULARITY_REQUESTED)
                rc = -EINVAL;
            else
                rc = -__libc_native2errno(rc);
            break;
        }

    } /* retry loop */
    FS_RESTORE();
    if (!rc)
        LIBCLOG_RETURN_INT(rc);
    LIBCLOG_ERROR_RETURN_INT(rc);
}

