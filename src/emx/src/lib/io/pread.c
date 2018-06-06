/* $Id: pread.c 2254 2005-07-17 12:25:44Z bird $ */
/** @file
 *
 * pread.
 *
 * Copyright (c) 2004 knut st. osmundsen <bird@anduin.net>
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
#include <errno.h>
#include <unistd.h>
#include <InnoTekLIBC/backend.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_IO
#include <InnoTekLIBC/logstrict.h>



/**
 * Read from a file without changing the file pointer.
 *
 * This API will not work if the file is non-blocking or another
 * thread tries operating on the file while it's executing.
 *
 * @returns Number of bytes written.
 * @param   fh      Filehandle.
 * @param   pvBuf   Where to store the read bytes.
 * @param   cbBuf   Number of bytes to read.
 * @param   off     Where in the file to read it from.
 */
ssize_t	 _STD(pread)(int fh, void *pvBuf, size_t cbBuf, off_t off)
{
    LIBCLOG_ENTER("fh=%d pvBuf=%p cbBuf=%d off=%lld (%#llx)\n", fh, pvBuf, cbBuf, off, off);

    /*
     * Save current offset.
     */
    off_t offSaved = lseek(fh, 0, SEEK_CUR);
    if (offSaved >= 0)
    {
        /*
         * Reposition ourselves to off.
         * Do we have to support extending the file???
         */
        if (lseek(fh, off, SEEK_SET) >= 0)
        {
            /*
             * Attemp the write.
             */
            ssize_t cbRead = read(fh, pvBuf, cbBuf);
            if (cbRead >= 0)
            {
                /*
                 * Restore fileposition and return.
                 */
                if (lseek(fh, offSaved, SEEK_SET) >= 0)
                    LIBCLOG_RETURN_INT(cbRead);
            }
            /* quitely try restore the original position without messing up errno */
            int saved_errno = errno;
            lseek(fh, offSaved, SEEK_SET);
            errno = saved_errno;
        }
    }

    LIBCLOG_ERROR_RETURN_INT(-1);
}

