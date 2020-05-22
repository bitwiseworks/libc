/* $Id: b_fsNativeFileTimesSet.c 2313 2005-08-28 06:19:49Z bird $ */
/** @file
 *
 * LIBC SYS Backend - internal [l]utimes.
 *
 * Copyright (c) 2005 knut st. osmundsen <bird@innotek.de>
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
#define INCL_FSMACROS
#include <os2emx.h>
#include "b_fs.h"
#include "b_time.h"
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/timeb.h>
#include "syscalls.h"
#include <emx/syscalls.h>
#include <emx/time.h>
#include <InnoTekLIBC/libc.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_BACK_FS
#include <InnoTekLIBC/logstrict.h>


/**
 * Sets the file times of a native file.
 *
 * @returns 0 on success.
 * @returns Negative error code (errno.h) on failure.
 * @param   pszNativePath   Path to the file to set the times of.
 * @param   paTimes         Two timevalue structures. If NULL the current time is used.
 */
int __libc_back_fsNativeFileTimesSet(const char *pszNativePath, const struct timeval *paTimes)
{
    LIBCLOG_ENTER("pszNativePath=%p:{%s} paTimes=%p:{{.tv_sec=%d, .tv_usec=%ld}, {.tv_sec=%d, .tv_usec=%ld}}\n",
                  (void *)pszNativePath, pszNativePath, (void *)paTimes,
                  paTimes ? paTimes[0].tv_sec : ~0, paTimes ? paTimes[0].tv_usec : ~0,
                  paTimes ? paTimes[1].tv_sec : ~0, paTimes ? paTimes[1].tv_usec : ~0);
    FILESTATUS3 fsts3;
    FS_VAR();

    /*
     * Validate input, refusing named pipes.
     */
    if (    (pszNativePath[0] == '/' || pszNativePath[0] == '\\')
        &&  (pszNativePath[1] == 'p' || pszNativePath[1] == 'P')
        &&  (pszNativePath[2] == 'i' || pszNativePath[2] == 'I')
        &&  (pszNativePath[3] == 'p' || pszNativePath[3] == 'P')
        &&  (pszNativePath[4] == 'e' || pszNativePath[4] == 'E')
        &&  (pszNativePath[5] == '/' || pszNativePath[5] == '\\'))
        LIBCLOG_ERROR_RETURN_INT(-ENOENT);

    /*
     * If potential device, then perform real check.
     * (Devices are subject to mode in POSIX.)
     */
    /** @todo copy device check from the path resolver. */

    /*
     * Ok, we can normalize the time.
     */
    time_t aTimes[2]; /* seconds since 1970 */
    if (!paTimes)
    {
        struct timeb TimeBuf;
        __ftime(&TimeBuf);
        aTimes[0] = TimeBuf.time;
        aTimes[1] = TimeBuf.time;
    }
    else
    {
        if (!_tzset_flag)
            tzset();
        aTimes[0] = paTimes[0].tv_sec;
        _gmt2loc(&aTimes[0]);
        aTimes[1] = paTimes[1].tv_sec;
        _gmt2loc(&aTimes[1]);
    }

    /*
     * Get path info.
     */
    FS_SAVE_LOAD();
    int rc = DosQueryPathInfo((PCSZ)pszNativePath, FIL_STANDARD, &fsts3, sizeof(fsts3));
    if (rc)
    {
        rc = -__libc_native2errno(rc);
        FS_RESTORE();
        LIBCLOG_ERROR_RETURN_INT(rc);
    }

    /*
     * Update OS/2 attributes.
     */
    __libc_back_timeUnix2FileTime(aTimes[0], &fsts3.ftimeLastAccess, &fsts3.fdateLastAccess);
    __libc_back_timeUnix2FileTime(aTimes[1], &fsts3.ftimeLastWrite,  &fsts3.fdateLastWrite);
    rc = DosSetPathInfo((PCSZ)pszNativePath, FIL_STANDARD, &fsts3, sizeof(fsts3), 0);
    FS_RESTORE();
    if (!rc)
        LIBCLOG_RETURN_INT(rc);
    rc = -__libc_native2errno(rc);
    LIBCLOG_ERROR_RETURN_INT(rc);
}

