/* $Id: $ */
/** @file
 *
 * LIBC getdirents().
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
#include <dirent.h>
#include <errno.h>
#include <InnoTekLIBC/backend.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_MISC
#include <InnoTekLIBC/logstrict.h>


/**
 * Reads directory entries from an open directory.
 *
 * @returns Number of bytes read.
 * @returns -1 on and errno set to EBADF, EINVAL or EIO.
 *
 * @param   fh      The file handle of an open directory.
 * @param   pvBuf   Where to store the directory entries.
 *                  The returned data is a series of dirent structs with
 *                  variable name size. d_reclen must be used the offset
 *                  to the next struct (from the start of the current one).
 * @param   cbBuf   Size of the buffer.
 *
 */
int _STD(getdirents)(int fh, char *pvBuf, int cbBuf)
{
    LIBCLOG_ENTER("fh=%d pvBuf=%p cbBuf=%d(%#x)\n", fh, pvBuf, cbBuf, cbBuf);

    ssize_t cb = __libc_Back_ioDirGetEntries(fh, pvBuf, cbBuf, NULL);
    if (cb >= 0)
        LIBCLOG_RETURN_INT((int)cb);

    errno = -cb;
    LIBCLOG_ERROR_RETURN_P(-1);
}

