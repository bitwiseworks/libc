/* $Id: strerror_r.c 1287 2004-03-11 03:05:55Z bird $ */
/** @file
 *
 * strerror_r()
 *
 * Copyright (c) 2003 knut st. osmundsen <bird-srcspam@anduin.net>
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with This program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include "libc-alias.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <InnoTekLIBC/thread.h>
#include <sys/errno.h>

int _STD(strerror_r)(int iErrno, char *pszBuffer, size_t cchBuffer)
{
    if (iErrno >= 0 && iErrno < sys_nerr)
    {
        const char *psz = sys_errlist[iErrno];
        int cch = strlen(psz) + 1;
        if (cch <= cchBuffer)
        {
            memcpy(pszBuffer, psz, cch);
            return 0;
        }
        memcpy(pszBuffer, psz, cchBuffer);
        pszBuffer[cchBuffer - 1] = '\0';
        errno = ERANGE;
        return -1;
    }
    errno = EINVAL;
    return -1;
}

