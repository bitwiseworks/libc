/* $Id: b_ioFileSizeSet.c 2254 2005-07-17 12:25:44Z bird $ */
/** @file
 *
 * LIBC SYS Backend - chsize.
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
#define INCL_ERRORS
#include <os2emx.h>
#include "b_fs.h"
#include <limits.h>
#include <errno.h>
#include <InnoTekLIBC/backend.h>
#define __LIBC_LOG_GROUP    __LIBC_LOG_GRP_BACK_IO
#include <InnoTekLIBC/logstrict.h>

/*******************************************************************************
*   Global Variables                                                           *
*******************************************************************************/
/** A page of zeros.
 * @todo Make this a separate segment for optimal thunking effiency! */
static const char __libc_gachZeros[65536 - 4096];


/*******************************************************************************
*   Internal Functions                                                         *
*******************************************************************************/
static inline int getFileSize(HFILE hFile, off_t *pcb);
static inline int setFileSize(HFILE hFile, off_t cbFile);
static inline int seekFile(HFILE hFile, off_t cbFile, int iMethod, off_t *poffFile);

/**
 * Calls the correct OS/2 api for this operation.
 * FS is saved and loaded by caller.
 * @returns OS/2 error code or negative errno.
 */
static inline int getFileSize(HFILE hFile, off_t *pcb)
{
    union
    {
        FILESTATUS3     fsts3;
        FILESTATUS3L    fsts3L;
    } info;
    int     rc;
#if OFF_MAX > LONG_MAX
    if (__libc_gpfnDosOpenL)
    {
        rc = DosQueryFileInfo(hFile, FIL_STANDARDL, &info, sizeof(info.fsts3L));
        *pcb = info.fsts3L.cbFile;
    }
    else
#endif
    {
        rc = DosQueryFileInfo(hFile, FIL_STANDARD, &info, sizeof(info.fsts3));
        *pcb = info.fsts3.cbFile;
    }
    return rc;
}

/**
 * Calls the correct OS/2 api for this operation.
 * FS is saved and loaded by caller.
 * @returns OS/2 error code or negative errno.
 */
static inline int setFileSize(HFILE hFile, off_t cbFile)
{
    int rc;
#if OFF_MAX > LONG_MAX
    if (__libc_gpfnDosSetFileSizeL)
        rc = __libc_gpfnDosSetFileSizeL(hFile, cbFile);
    else
    {
        if (cbFile > __LONG_MAX)
            return -EOVERFLOW;
        rc = DosSetFileSize(hFile, cbFile);
    }
#else
    rc = DosSetFileSize(hFile, cbFile);
#endif
    return rc;
}

/**
 * Calls the correct OS/2 api for this operation.
 * FS is saved and loaded by caller.
 * @returns OS/2 error code or negative errno.
 */
static inline int seekFile(HFILE hFile, off_t off, int iMethod, off_t *poffFile)
{
    int rc;
#if OFF_MAX > LONG_MAX
    if (__libc_gpfnDosSetFilePtrL)
    {
        LONGLONG cbNewTmp;
        rc = __libc_gpfnDosSetFilePtrL(hFile, off, iMethod, &cbNewTmp);
        off = cbNewTmp;
    }
    else
    {
        ULONG cbNewTmp;
        if (off > LONG_MAX || off < LONG_MIN)
            return -EOVERFLOW;
        rc = DosSetFilePtr(hFile, off, iMethod, &cbNewTmp);
        off = cbNewTmp;
    }
#else
    {
        ULONG cbNewTmp;
        rc = DosSetFilePtr(hFile, off, iMethod, &cbNewTmp);
        off = cbNewTmp;
    }
#endif
    if (poffFile)
        *poffFile = off;
    return rc;
}


/**
 * Sets the size of an open file.
 *
 * When expanding a file the contents of the allocated
 * space is undefined.
 *
 * @returns 0 on success.
 * @returns Negative error code (errno.h) on failure.
 * @param   fh      Handle to the file which size should be changed.
 * @param   cbFile  The new filesize.
 * @param   fZero   If set any new allocated file space will be
 *                  initialized to zero.
 */
int __libc_Back_ioFileSizeSet(int fh, __off_t cbFile, int fZero)
{
    LIBCLOG_ENTER("fh=%d cbFile=%lld fZero=%d\n", fh, cbFile, fZero);

    /*
     * Get filehandle.
     */
    PLIBCFH     pFH;
    int rc = __libc_FHEx(fh, &pFH);
    if (rc)
        LIBCLOG_ERROR_RETURN_INT(rc);

    if (!pFH->pOps)
    {
        /*
         * Standard OS/2 file handle.
         *
         * Get the current file size so we can perform
         * any necessary zero padding.
         */
        FS_VAR();
        FS_SAVE_LOAD();
        off_t   cbCur;
        rc = getFileSize(fh, &cbCur);
        if (!rc)
        {
            if (cbCur != cbFile)
            {
                /*
                 * File size change needed.
                 */
                rc = setFileSize(fh, cbFile);
                if (!rc)
                {
                    /*
                     * We're done if it was a truncation.
                     */
                    off_t   cbLeft = cbFile - cbCur;
                    if (    cbLeft <= 0
                        ||  !fZero
                        || (pFH->pFsInfo && pFH->pFsInfo->fZeroNewBytes))
                    {
                        FS_RESTORE();
                        LIBCLOG_RETURN_INT(0);
                    }

                    /*
                     * Must fill the allocated file space with zeros.
                     */
                    off_t offSaved;
                    rc = seekFile(fh, 0, FILE_CURRENT, &offSaved);
                    if (!rc)
                        rc = seekFile(fh, cbCur, FILE_BEGIN, NULL);
                    while (!rc && cbLeft > 0)
                    {
                        ULONG cb = cbLeft >= sizeof(__libc_gachZeros) ? sizeof(__libc_gachZeros) : cbLeft;
                        rc = DosWrite(fh, &__libc_gachZeros[0], cb, &cb);
                        cbLeft -= cb;
                    }
                    int rc2 = seekFile(fh, offSaved, FILE_BEGIN, NULL);
                    if (!rc && !rc2)
                    {
                        FS_RESTORE();
                        LIBCLOG_RETURN_INT(0);
                    }

                    /*
                     * Shit, we failed writing zeros.
                     * Try undo the expand operation.
                     */
                    setFileSize(fh, cbCur);
                    if (rc2)
                    {
                        seekFile(fh, offSaved, FILE_BEGIN, NULL);
                        if (!rc)
                            rc = rc2;
                    }
                }
            }
        }

        FS_RESTORE();
        if (rc > 0)
            rc = -__libc_native2errno(rc);
        LIBCLOG_ERROR_RETURN_INT(rc);
    }
    else
    {
        /*
         * Non-standard file handle - fail.
         */
        LIBCLOG_ERROR_RETURN_INT(-EOPNOTSUPP);
    }
}

