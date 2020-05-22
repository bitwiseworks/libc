/* $Id: b_fsFileTimesSetFH.c 2313 2005-08-28 06:19:49Z bird $ */
/** @file
 *
 * LIBC SYS Backend - futimes.
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
#define INCL_FSMACROS
#include <os2emx.h>
#include "b_fs.h"
#include "b_time.h"
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <sys/timeb.h>
#include <emx/io.h>
#include <emx/time.h>
#include <emx/syscalls.h>
#include <limits.h>
#include "syscalls.h"
#include <InnoTekLIBC/libc.h>
#include <InnoTekLIBC/backend.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_BACK_FS
#include <InnoTekLIBC/logstrict.h>


/**
 * Sets the file the times of a file by filehandle.
 *
 * @returns 0 on success.
 * @returns Negative error code (errno.h) on failure.
 * @param   fh      Handle to file.
 * @param   paTimes Two timevalue structures. If NULL the current time is used.
 */
int __libc_Back_fsFileTimesSetFH(int fh, const struct timeval *paTimes)
{
    LIBCLOG_ENTER("fh=%d paTimes=%p:{{.tv_sec=%d, .tv_usec=%ld}, {.tv_sec=%d, .tv_usec=%ld}}\n",
                  fh, (void *)paTimes,
                  paTimes ? paTimes[0].tv_sec : ~0, paTimes ? paTimes[0].tv_usec : ~0,
                  paTimes ? paTimes[1].tv_sec : ~0, paTimes ? paTimes[1].tv_usec : ~0);

    /*
     * Get filehandle.
     */
    PLIBCFH pFH;

    int rc = __libc_FHEx(fh, &pFH);
    if (rc)
        LIBCLOG_ERROR_RETURN_INT(rc);

    /*
     * Check the type.
     */
    switch (pFH->fFlags & __LIBC_FH_TYPEMASK)
    {
        /* fail */
        case F_SOCKET:
        case F_PIPE: /* treat as socket for now */
            LIBCLOG_ERROR_RETURN_INT(-EINVAL);
        /* ignore */
        case F_DEV:
            LIBCLOG_RETURN_INT(0);

        /* use the path access. */
        case F_DIR:
            if (__predict_false(!pFH->pszNativePath))
                LIBCLOG_ERROR_RETURN_INT(-EINVAL);
            rc = __libc_back_fsNativeFileTimesSet(pFH->pszNativePath, paTimes);
            if (rc)
                LIBCLOG_ERROR_RETURN_INT(rc);
            LIBCLOG_RETURN_INT(rc);

        /* treat */
        default:
        case F_FILE:
            break;
    }

    if (!pFH->pOps)
    {
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
         * Standard OS/2 file handle.
         */
        FS_VAR();
        FS_SAVE_LOAD();
        FILESTATUS3     fsts3;

        /*
         * Get file info.
         */
        rc = DosQueryFileInfo(fh, FIL_STANDARD, &fsts3, sizeof(fsts3));
        if (rc)
        {
            FS_RESTORE();
            rc = -__libc_native2errno(rc);
            LIBCLOG_ERROR_RETURN_INT(rc);
        }

        /*
         * Update OS/2 bits.
         */
        __libc_back_timeUnix2FileTime(aTimes[0], &fsts3.ftimeLastAccess, &fsts3.fdateLastAccess);
        __libc_back_timeUnix2FileTime(aTimes[1], &fsts3.ftimeLastWrite,  &fsts3.fdateLastWrite);
        rc = DosSetFileInfo(fh, FIL_STANDARD, &fsts3, sizeof(fsts3));
        if (rc && pFH->pszNativePath)
        {
            LIBCLOG_ERROR("DosSetFileInfo(%d,,,) -> %d\n", fh, rc);
            rc = DosSetPathInfo((PCSZ)pFH->pszNativePath, FIL_STANDARD, &fsts3, sizeof(fsts3), 0);
            if (rc)
                LIBCLOG_ERROR("DosSetPathInfo('%s',,,) -> %d\n", pFH->pszNativePath, rc);
        }
        FS_RESTORE();
        if (rc)
        {
            rc = -__libc_native2errno(rc);
            LIBCLOG_ERROR_RETURN_INT(rc);
        }
    }
    else
    {
        /*
         * Non-standard handle - fail.
         */
        LIBCLOG_ERROR_RETURN_INT(-EOPNOTSUPP);
    }

    LIBCLOG_RETURN_INT(0);
}

