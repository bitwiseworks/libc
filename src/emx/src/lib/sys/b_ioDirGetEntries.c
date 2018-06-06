/* $Id: $ */
/** @file
 *
 * LIBC SYS Backend - getdirentries().
 *
 * Copyright (c) 2005 knut st. osmundsen <bird@innotek.de>
 *
 *
 * This file is part of InnoTek LIBC.
 *
 * InnoTek LIBC is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * InnoTek LIBC is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
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
#include "b_dir.h"
#include <errno.h>
#include <sys/dirent.h>
#include <InnoTekLIBC/backend.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_BACK_IO
#include <InnoTekLIBC/logstrict.h>



/**
 * Reads directory entries from an open directory.
 *
 * @returns Number of bytes read.
 * @returns Negative error code (errno.h) on failure.
 *
 * @param   fh      The file handle of an open directory.
 * @param   pvBuf   Where to store the directory entries.
 *                  The returned data is a series of dirent structs with
 *                  variable name size. d_reclen must be used the offset
 *                  to the next struct (from the start of the current one).
 * @param   cbBuf   Size of the buffer.
 * @param   poff    Where to store the lseek offset of the first entry.
 *
 */
ssize_t __libc_Back_ioDirGetEntries(int fh, void *pvBuf, size_t cbBuf, __off_t *poff)
{
    LIBCLOG_ENTER("fh=%d pvBuf=%p cbBuf=%d(%#x) poff=%p\n", fh, pvBuf, cbBuf, cbBuf, (void *)poff);

    /*
     * Validate input and resolve handle.
     */
    if (!pvBuf)
        LIBCLOG_ERROR_RETURN_INT(-EFAULT);
    if (cbBuf < sizeof(struct dirent))
        LIBCLOG_ERROR_RETURN_INT(-EINVAL);
    __LIBC_PFH pFH;
    int rc = __libc_FHEx(fh, &pFH);
    if (rc)
        LIBCLOG_ERROR_RETURN_INT(rc);
    if (!pFH->pOps || pFH->pOps->enmType != enmFH_Directory)
        LIBCLOG_ERROR_RETURN_INT(-ENOTDIR); /* BSD returns EINVAL */

    /*
     * Use the read call to do the work...
     */
    __LIBC_PFHDIR pFHDir = (__LIBC_PFHDIR)pFH;
    off_t off = pFHDir->uCurEntry;
    ssize_t cb = __libc_back_dirGetEntries(pFHDir, pvBuf, cbBuf);
    if (cb >= 0)
    {
        if (poff)
            *poff = off;
        LIBCLOG_RETURN_MSG(cb, "ret %#x *poff=%#x\n", cb, off);
    }
    LIBCLOG_ERROR_RETURN_INT(cb);
}

