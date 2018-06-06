/* $Id: _getcwdux.c 2254 2005-07-17 12:25:44Z bird $ */
/** @file
 *
 * _getcwdux().
 *
 * Copyright (c) 2004 knut st. osmundsen <bird@innotek.de>
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
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <InnoTekLIBC/backend.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_MISC
#include <InnoTekLIBC/logstrict.h>


/**
 * Gets the current directory of the process - without drive letter.
 *
 * This function is intended for porting Unix software which doesn't handle the
 * driveletter from getcwd(). For example you can use -Dgetcwd=_getcwdux on the
 * commandline, or define a wrapper getcwd() function which calls this.
 *
 * The slashes are always the unix way, of course.
 *
 * @returns buf on success.
 * @returns NULL and errno on failure.
 * @param   buf     Where to store the current directory.
 *                  If NULL a buffer is malloc'ed. The size of a malloc'ed buffer is
 *                  bufsize unless that is 0 when it'll be allocated as big as necessary.
 * @param   bufsize Size of the buffer.
 * @remark  The EMX implementation of this interface did not return a driveletter.
 */
char *_getcwdux(char *buf, size_t bufsize)
{
    LIBCLOG_ENTER("buf=%p bufsize=%d\n", (void *)buf, bufsize);

    /*
     * For compatability with the GNU C library we must
     * handle buf == NULL.
     */
    int rc;
    if (!buf)
    {
        LIBCLOG_MSG("Allocating buffer, %d bytes.\n", bufsize ? bufsize : PATH_MAX);
        size_t cch = bufsize ? bufsize : PATH_MAX;
        buf = malloc(cch);
        if (buf)
        {
            rc = __libc_Back_fsDirCurrentGet(buf, cch, 0, __LIBC_BACK_FSCWD_NO_DRIVE);
            if (!rc)
            {
                /*
                 * Reallocate a PATH_MAX buffer before we return.
                 */
                if (!bufsize)
                {
                    char *pvOld = buf;
                    cch = strlen(buf) + 1;
                    buf = realloc(buf, cch > bufsize ? cch : bufsize);
                    if (!buf)
                        buf = pvOld;
                }

                /*
                 * Slashify.
                 */
                char *psz = strchr(buf, '\\');
                while (psz)
                {
                    *psz++ = '/';
                    psz = strchr(psz, '\\');
                }

                LIBCLOG_RETURN_MSG(buf, "ret %p:{%s}\n", (void *)buf, buf);
            }
            free(buf);
        }
        else
            rc = -ENOMEM;
    }
    else
    {
        rc = __libc_Back_fsDirCurrentGet(buf, bufsize, 0, 0);
        if (!rc)
        {
            /*
             * Slashify.
             */
            char *psz = strchr(buf, '\\');
            while (psz)
            {
                *psz++ = '/';
                psz = strchr(psz, '\\');
            }

            LIBCLOG_RETURN_MSG(buf, "ret %p:{%s}\n", (void *)buf, buf);
        }
    }

    errno = -rc;
    LIBCLOG_ERROR_RETURN_P(NULL);
}

