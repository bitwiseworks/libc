/* $Id: nanosleep.c 2254 2005-07-17 12:25:44Z bird $ */
/** @file
 *
 * LIBC - nanosleep().
 *
 * Copyright (c) 2005 knut st. osmundsen <bird@anduin.net>
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
#include <time.h>
#include <errno.h>
#include <sys/limits.h>
#include <InnoTekLIBC/backend.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_THREAD
#include <InnoTekLIBC/logstrict.h>


/**
 * Suspends the execution of the calling thread for a period of time.
 *
 * @returns 0 on success
 * @returns -1 and EINVAL if the request timespec is invalid.
 * @returns -1 and EINTR if the suspention was interrupted by the arrival and processing of a signal.
 *
 * @param   pReqTS      The request period of time to suspend the thread.
 * @param   pRemTS      Where to, on interruption, store the remaining time.
 */
int _STD(nanosleep)(const struct timespec *pReqTS, struct timespec *pRemTS)
{
    LIBCLOG_ENTER("pReqTS=%p:{.tv_sec=%u, .tv_nsec=%ld} pRemTS=%p\n", (void *)pReqTS, pReqTS->tv_sec, pReqTS->tv_nsec, (void *)pRemTS);

    /*
     * Validate.
     */
    if (    pReqTS->tv_nsec < 0
        ||  pReqTS->tv_nsec >= 1000000000)
    {
        errno = EINVAL;
        LIBCLOG_ERROR_RETURN(-1, "ret -1 - Invalid timespec tv_nsec=%ld\n", pReqTS->tv_nsec);
    }

    /*
     * Convert it all to nanosecond and call the backend.
     */
    unsigned long long ullNanoReq;
    if (    sizeof(pReqTS->tv_sec) < sizeof(ullNanoReq)
        ||  pReqTS->tv_sec < (ULLONG_MAX / 1000000000))
        ullNanoReq = pReqTS->tv_sec * 1000000000ULL
                   + pReqTS->tv_nsec;
    else
        ullNanoReq = ~0ULL;

    unsigned long long ullNanoRem;
    int rc = __libc_Back_threadSleep(ullNanoReq, pRemTS ? &ullNanoRem : NULL);
    if (!rc)
        LIBCLOG_RETURN_INT(0);
    /* error */
    if (    rc == -EINTR
        &&  pRemTS)
    {
        pRemTS->tv_sec  = ullNanoRem / 1000000000;
        pRemTS->tv_nsec = ullNanoRem % 1000000000;
    }
    errno = -rc;
    LIBCLOG_ERROR_RETURN_INT(-1);
}

