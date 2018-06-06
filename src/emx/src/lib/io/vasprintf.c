/* $Id: vasprintf.c 2254 2005-07-17 12:25:44Z bird $ */
/** @file
 *
 * LIBC - asprintf.
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
#include "libc-alias.h"
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_STREAM
#include <InnoTekLIBC/logstrict.h>


/**
 * Allocating sprintf.
 *
 * @returns number of formatted bytes excluding the terminating zero.
 * @returns -1 on failure.
 * @param   ppsz        Where to store the allocated string pointer.
 *                      On failure this will be NULL.
 * @param   pszFormat   Format string.
 * @param   ...         Format arguments.
 */
int _STD(vasprintf)(char **ppsz, const char *pszFormat, va_list args)
{
    LIBCLOG_ENTER("ppsz=%p pszFormat=%p:{%s} args=%p\n", (void *)ppsz, (void *)pszFormat, pszFormat, args);
    *ppsz = NULL;

    /*
     * Just a quick hack, not optimal, but it'll work for now.
     */
    size_t cb = 4096;
    char *psz = malloc(cb);
    if (psz)
    {
        for (int cTries = 0; cTries < 5; cTries++)
        {
            va_list argsCopy = args;
            int cch = vsnprintf(psz, cb, pszFormat, args);
            if (cch < 0) /* shouldn't happen */
                break;
            if (cch < cb)
            {
                /* don't waste memory */
                if (cb - cch > 128)
                {
                    char *psz2 = (char *)realloc(psz, cch + 1);
                    if (psz2)
                        psz = psz2;
                }

                *ppsz = psz;
                LIBCLOG_RETURN_MSG(cch, "ret %d *ppsz=%p:{%s}\n", cch, (void *)psz, psz);
            }

            /*
             * Ok, we've gotta retry with a bigger buffer.
             */
            args = argsCopy;
            cb = (cch + 3 + 15 + cTries * cb) & ~15; /* 3 extra and 16 bytes alignment, special hack for retries. */
            char *psz2 = (char *)realloc(psz, cb);
            if (!psz2)
                break;
            psz = psz2;
        }
        LIBC_ASSERT_FAILED();
        free(psz);
    }

    LIBCLOG_ERROR_RETURN_INT(-1);
}

