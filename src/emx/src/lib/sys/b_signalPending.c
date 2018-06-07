/* $Id: b_signalPending.c 2254 2005-07-17 12:25:44Z bird $ */
/** @file
 *
 * LIBC SYS Backend - sigpending().
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
#include <signal.h>
#include <errno.h>
#include <InnoTekLIBC/thread.h>
#include <InnoTekLIBC/backend.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_BACK_SIGNAL
#include <InnoTekLIBC/logstrict.h>
#include "b_signal.h"


/**
 * Gets the set of signals which are blocked by the current thread and are
 * pending on the process or the calling thread.
 *
 * @returns 0 indicating success.
 * @returns Negative error code (errno) on failure.
 * @param   pSigSet     Pointer to signal set where the result is to be stored.
 */
int __libc_Back_signalPending(sigset_t *pSigSet)
{
    LIBCLOG_ENTER("pSigSet=%p\n", (void *)pSigSet);
    __LIBC_PTHREAD  pThrd = __libc_threadCurrent();
    sigset_t        SigSet;

    __SIGSET_EMPTY(&SigSet);            /* touch it! (paranoia) */

    /*
     * Gain exclusive access to the signal stuff.
     */
    int rc =__libc_back_signalSemRequest();
    if (rc)
        LIBCLOG_ERROR_RETURN_INT(rc);

    /*
     * Or the blocked and pending members for this thread.
     */
    __SIGSET_OR(&SigSet, &__libc_gSignalPending, &pThrd->SigSetPending);
    __SIGSET_AND(&SigSet, &pThrd->SigSetBlocked, &SigSet);

    /*
     * Release semaphore.
     */
    __libc_back_signalSemRelease();

    /*
     * Copy the set.
     */
    *pSigSet = SigSet;

    LIBCLOG_RETURN_MSG(0, "ret 0 (*pSigSet={%08lx %08lx})\n", SigSet.__bitmap[1], SigSet.__bitmap[0]);
}


