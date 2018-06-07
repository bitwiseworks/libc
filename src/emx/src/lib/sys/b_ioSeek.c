/* $Id: b_ioSeek.c 2254 2005-07-17 12:25:44Z bird $ */
/** @file
 *
 * LIBC SYS Backend - seek.
 *
 * Copyright (c) 2003-2004 knut st. osmundsen <bird@innotek.de>
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
#include <limits.h>
#include <errno.h>
#include <stdio.h>
#include <InnoTekLIBC/backend.h>
#define __LIBC_LOG_GROUP    __LIBC_LOG_GRP_BACK_IO
#include <InnoTekLIBC/logstrict.h>


/**
 * Change the current position of a file stream and get the new position.
 *
 * @returns new file offset on success.
 * @returns Negative error code (errno) on failure.
 * @param   hFile       File handle to preform seek operation on.
 * @param   off         Offset to seek to.
 * @param   iOrigin     The seek method. SEEK_CUR, SEEK_SET or SEEK_END.
 */
off_t __libc_Back_ioSeek(int hFile, off_t off, int iMethod)
{
    LIBCLOG_ENTER("hFile=%d off=%lld iMethod=%d\n", hFile, off, iMethod);

    /*
     * Get filehandle / validate input.
     */
    if (iMethod != SEEK_SET && iMethod != SEEK_CUR && iMethod != SEEK_END)
        LIBCLOG_RETURN_INT(-EINVAL);
    PLIBCFH pFH;
    int rc = __libc_FHEx(hFile, &pFH);
    if (rc)
        LIBCLOG_ERROR_RETURN_INT(rc);

    off_t   cbNew;
    if (!pFH->pOps)
    {
        /*
         * Standard OS/2 filehandle.
         */
        FS_VAR();
        FS_SAVE_LOAD();
#if OFF_MAX > LONG_MAX
        if (__libc_gpfnDosSetFilePtrL)
        {
            LONGLONG cbNewTmp;
            rc = __libc_gpfnDosSetFilePtrL(hFile, off, iMethod, &cbNewTmp);
            cbNew = cbNewTmp;
        }
        else
        {
            ULONG cbNewTmp;
            if (off > LONG_MAX || off < LONG_MIN)
            {
                FS_RESTORE();
                LIBCLOG_ERROR_RETURN_INT(-EOVERFLOW);
            }
            rc = DosSetFilePtr(hFile, off, iMethod, &cbNewTmp);
            cbNew = cbNewTmp;
        }
#else
        {
            ULONG cbNewTmp;
            rc = DosSetFilePtr(hFile, off, iMethod, &cbNewTmp);
            cbNew = cbNewTmp;
        }
#endif
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
         * Non-standard filehandle - fail for the present.
         */
        LIBCLOG_ERROR_RETURN_INT(-EOPNOTSUPP);
    }

    LIBCLOG_RETURN_MSG(cbNew, "ret %lld (%#llx)\n", cbNew, cbNew);
}


