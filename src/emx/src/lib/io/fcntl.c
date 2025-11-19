/* $Id: $ */
/** @file
 *
 * LIBC - fcntl().
 *
 * Copyright (c) 1992-1998 by Eberhard Mattes
 * Copyright (c) 2003-2005 knut st. osmundsen <bird@innotek.de>
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
#include <stdarg.h>
#include <unistd.h>
#include <io.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <emx/io.h>
#include <InnoTekLIBC/backend.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_IO
#include <InnoTekLIBC/logstrict.h>


/*******************************************************************************
*   Internal Functions                                                         *
*******************************************************************************/
static int dupfd(int fh, int fhMin);

int _STD(fcntl)(int fh, int iRequest, ...)
{
    LIBCLOG_ENTER("fh=%d iRequest=%d\n", fh, iRequest);
    int rc;
    int rcSuccess = 0;
    switch (iRequest)
    {
        /*
         * No arg.
         */
        case F_GETFD:
        case F_GETFL:
            rc = __libc_Back_ioFileControl(fh, iRequest, 0, &rcSuccess);
            break;

        /*
         * int arg.
         */
        case F_SETFL:
        case F_SETFD:
        {
            va_list Args;
            va_start(Args, iRequest);
            int fFlags = va_arg(Args, int);
            va_end(Args);
            rc = __libc_Back_ioFileControl(fh, iRequest, fFlags, &rcSuccess);
            break;
        }

        /*
         * struct flock * argument.
         */
        case F_GETLK:   /* get record locking information */
        case F_SETLK:   /* set record locking information */
        case F_SETLKW:  /* F_SETLK; wait if blocked */
        {
            va_list Args;
            va_start(Args, iRequest);
            struct flock *pFlock = va_arg(Args, struct flock *);
            va_end(Args);
            rc = __libc_Back_ioFileControl(fh, iRequest, (intptr_t)pFlock, &rcSuccess);
            break;
        }

        /*
         * Handled here.
         */
        case F_DUPFD:
        {
            va_list Args;
            va_start(Args, iRequest);
            int fhMin = va_arg(Args, int);
            va_end(Args);
            rc = dupfd(fh, fhMin);
            if (rc >= 0)
                LIBCLOG_RETURN_INT(rc);
            break;
        }
        case F_CLOSEM:
        case F_MAXFD:
        {
            /* NOTE: Arg is ignored */
            rc = __libc_fhFcntl(fh, iRequest, 0, &rcSuccess);
            break;
        }

        default:
            errno = -EINVAL;
            LIBCLOG_ERROR_RETURN_MSG(-1, "ret -1 - iRequest=%d\n", iRequest);
    }

    if (!rc)
        LIBCLOG_RETURN_INT(rcSuccess);
    errno = -rc;
    LIBCLOG_ERROR_RETURN_INT(-1);
}


/**
 * Create a new file handle for 'fh' that is the lowest numbered
 * available file handle greater than or equal to 'fhMin'.
 *
 * @returns New file handle on success.
 * @returns -1 and errno on failure.
 * @param   fh      The file handle to duplicate.
 * @param   fhMin   The minimum file handle number.
 */
static int dupfd(int fh, int fhMin)
{
    LIBCLOG_ENTER("fh=%d fhMin=%d\n", fh, fhMin);

    /*
     * Check filehandle range.
     */
    /** @todo Define a max handles or something, get it dynamically is probably right idea. */
    if (fhMin < 0 || fhMin >= /*_POSIX_OPEN_MAX */ 10000)
    {
        errno = EINVAL;
        LIBCLOG_ERROR_RETURN_INT(-1);
    }

    /*
     * Brute force, duplicate till we get what we want.
     */
    int    *paFHs = alloca(fhMin * sizeof(int));
    int     e = errno;
    int     i = 0;
    int     fhNew;
    for (;;)
    {
        fhNew = dup(fh);
        if (fhNew < 0 || fhNew >= fhMin)
            break;
        if (i >= fhMin)
        {
            /* Avoid writing beyond the end of paFHs[] if dup()
               happens not to work as advertised. */

            close(fhNew);
            fhNew = -1;
            errno = EMFILE;
            break;
        }
        paFHs[i++] = fhNew;
    }

    /*
     * Clean up, save errno again on failure.
     */
    if (fhNew < 0)
        e = errno;
    while (i > 0)
        close(paFHs[--i]);
    errno = e;

    LIBCLOG_MIX_RETURN_INT(fhNew);
}



