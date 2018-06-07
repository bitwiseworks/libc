/* $Id: getdelim.c 2594 2006-03-09 18:38:07Z bird $ */
/** @file
 *
 * LIBC - getline, GLIBC extension.
 *
 * Copyright (c) 2005 knut st. osmundsen <bird@anduin.net>
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
#define _GNU_SOURCE
#include "libc-alias.h"
#include <sys/fmutex.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <emx/io.h>
#include "getputc.h"
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_STREAM
#include <InnoTekLIBC/logstrict.h>


/**
 * Retrieves a string from the specified stream stopping when chDelim
 * have been read. The string is put into the buffer pointed to by *ppszString,
 * reallocating that if it's to small.
 *
 * @returns number of bytes read.
 * @returns -1 on failure - this includes EOF when nothing was retrieved.
 * @param   ppszString      Where to buffer pointer is stored. *ppszString can of course be NULL.
 * @param   pcchString      Size of the buffer pointed to by *ppszString.
 * @param   chDelim         The delimiter.
 * @param   pStream         The stream to read from.
 */
ssize_t _STD(getdelim)(char **ppszString, size_t *pcchString, int chDelim, FILE *pStream)
{
    LIBCLOG_ENTER("ppszString=%p:{%p}, pcchString=%p:{%u} chDelim=%c pStream=%p\n",
                  (void *)ppszString, (void *)*ppszString, (void *)pcchString, (int)*pcchString, chDelim, (void *)pStream);

    /*
     * Validate input.
     */
    if (!ppszString || !pcchString)
    {
        errno = EINVAL;
        LIBCLOG_ERROR_RETURN_MSG(-1, "ret -1 - Invalid paramter! ppszString=%p pcchString=%p\n", (void *)ppszString, (void *)pcchString);
    }

    /*
     * Allocate initial buffer.
     */
    if (!*ppszString || !*pcchString)
    {
        *ppszString = (char *)malloc(128);
        if (!*ppszString)
            LIBCLOG_ERROR_RETURN_INT(-1);
        *pcchString = 128;
    }

    /*
     * Lock the stream and start reading.
     */
    STREAM_LOCK(pStream);
    ssize_t rc = 0;
    char *psz = *ppszString;
    char *pszEnd = psz + *pcchString - 1;
    for (;;)
    {
        /** @todo this can be done faster by accessing what's in the buffer directly.
         * But for now simplicity will do the job. */
        int ch = _getc_inline(pStream);
        if (ch == EOF)
        {
            if (psz == *ppszString)
                rc = -1;
            break;
        }
        if (psz == pszEnd)
        {
            /*
             * Expand the buffer space!
             */
            size_t cch = *pcchString;
            cch += cch < 1024*1024 ? cch : 1024*1024; /* be reasonable */
            char *pszNew = (char *)realloc(*ppszString, cch);
            if (!pszNew)
            {
                rc = -1;
                break;
            }

            psz = pszNew + (psz - *ppszString);
            pszEnd = pszNew + cch - 1;
            *ppszString = pszNew;
            *pcchString = cch;
        }
        *psz++ = (char)ch;
        if (ch == chDelim)
            break;
    }
    STREAM_UNLOCK(pStream);

    if (!rc)
        rc = psz - *ppszString;

    *psz = '\0';
    LIBCLOG_RETURN_MSG(rc, "ret %d *ppszString=%p:{%s} *pcchString=%d\n",
                       (int)rc, (void *)*ppszString, *ppszString, (int)*pcchString);
}

