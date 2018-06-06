/* $Id: _getdcwd.c 3711 2011-03-17 16:26:03Z bird $ */
/** @file
 *
 * VAC/MSC interface.
 *
 * Copyright (c) 2004 knut st. osmundsen <bird-srcspam@anduin.net>
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
#include <direct.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/param.h>
#include <emx/syscalls.h>
#include <InnoTekLIBC/backend.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_MISC
#include <InnoTekLIBC/logstrict.h>


/**
 * Gets the full path of the current directory of a specific drive.
 *
 * @returns pszBuffer or pointer to an malloc'ed buffer containing the requested path.
 * @returns NULL and errno on failure.
 * @param   iDrive      Which drive to get the current directory of. This is 1-based,
 *                      meaning that A is 1, B is 2, and so on. 0 will return the
 *                      current directory of the current drive (which can be the unix root!).
 * @param   pszBuffer   Where to store the current directory of the drive.
 *                      If NULL a buffer will be malloc'ed. The size of the malloc'ed
 *                      buffer will be at least @a cbBuffer bytes.
 * @param   cbBuffer    The size of the buffer @a pszBuffer points to.  If 
                        @a pszbuffer is NULL, it specifies the minimum buffer 
                        size of the allocated buffer.
 */
char *_getdcwd(int iDrive, char *pszBuffer, int cbBuffer)
{
    LIBCLOG_ENTER("iDrive=%d pszBuffer=%p cbBuffer=%d\n", iDrive, (void *)pszBuffer, cbBuffer);
    char chDrive = iDrive ? iDrive + 'A' - 1 : 0;

    int rc;
    if (!pszBuffer)
    {
        size_t cbAlloced = cbBuffer > PATH_MAX ? cbBuffer : PATH_MAX + 1;
        LIBCLOG_MSG("Allocating buffer, %zd bytes.\n", cbAlloced);
        pszBuffer = malloc(cbAlloced);
        if (pszBuffer)
        {
            rc = __libc_Back_fsDirCurrentGet(pszBuffer, cbAlloced, chDrive, 0);
            if (!rc)
            {
                /*
                 * Reallocate the buffer before we return?
                 */
                size_t cbReturned = strlen(pszBuffer) + 1;
                if (   cbReturned + 64 <= cbAlloced 
                    && cbBuffer        <  (ssize_t)cbAlloced)
                {
                    char *pvOld = pszBuffer;
                    if (cbReturned <= cbBuffer)
                        cbReturned = cbBuffer + 1;
                    pszBuffer = realloc(pszBuffer, cbReturned);
                    if (pszBuffer)
                        cbAlloced = cbReturned;
                    else
                        pszBuffer = pvOld;
                }
                LIBCLOG_RETURN_MSG(pszBuffer, "ret %p:{%s} (%zu bytes)\n", (void *)pszBuffer, pszBuffer, cbAlloced);
            }

            free(pszBuffer);
        }
        else
            rc = -ENOMEM;
    }
    else
    {
        rc = __libc_Back_fsDirCurrentGet(pszBuffer, cbBuffer, chDrive, 0);
        if (!rc)
            LIBCLOG_RETURN_MSG(pszBuffer, "ret %p:{%s}\n", (void *)pszBuffer, pszBuffer);
    }

    errno = -rc;
    LIBCLOG_ERROR_RETURN_P(NULL);
}

