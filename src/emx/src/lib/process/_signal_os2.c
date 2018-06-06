/* $Id: _signal_os2.c 2254 2005-07-17 12:25:44Z bird $ */
/** @file
 *
 * LIBC - OS/2 style signal().
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
#include <InnoTekLIBC/libc.h>
#include <InnoTekLIBC/backend.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_SIGNAL
#include <InnoTekLIBC/logstrict.h>


/**
 * OS/2 style signal().
 *
 * @returns pointer to previous signal handler.
 * @param   iSignalNo   Signal number to set the handler for.
 * @param   pfnHandler  Pointer to the new signal handler.
 */
void (*_signal_os2(int iSignalNo, void (*pfnHandler)(int)))(int)
{
    LIBCLOG_ENTER("iSignalNo=%d pfnHandler=%p\n", iSignalNo, (void*)pfnHandler);
    struct sigaction    SigActOld;

    /*
     * Validate.
     */
    if (!__SIGSET_SIG_VALID(iSignalNo))
    {
        errno = EINVAL;
        LIBCLOG_ERROR_RETURN_MSG(SIG_ERR, "ret SIG_ERR - invalid signal number for SIG_ACK %d\n", iSignalNo);
    }
    if (pfnHandler == SIG_HOLD)
    {
        errno = EINVAL;
        LIBCLOG_ERROR_RETURN_MSG(SIG_ERR, "ret SIG_ERR - SIG_HOLD is not supported by OS/2 style signal handling!\n");
    }

    /*
     * Do requested action.
     */
    if (pfnHandler == SIG_ACK)
    {
        /*
         * Unblock the signal.
         */
        if (iSignalNo != SIGKILL)
        {
            sigset_t sigset;
            __SIGSET_EMPTY(&sigset);
            __SIGSET_SET(&sigset, iSignalNo);
            if (!sigprocmask(SIG_UNBLOCK, &sigset, NULL))
            {
                int rc = sigaction(iSignalNo, NULL, &SigActOld);
                LIBC_ASSERTM(!rc, "sigaction failed when querying current signal action!\n");
                rc = rc;
                LIBCLOG_RETURN_P(SigActOld.__sigaction_u.__sa_handler);
            }
        }
        else
        {
            LIBCLOG_ERROR("invalid signal number for SIG_ACK %d\n", iSignalNo);
            errno = EINVAL;
        }
    }
    else
    {
        /*
         * Set signal action.
         */
        int                 rc;
        struct sigaction    SigAct;
        SigAct.__sigaction_u.__sa_handler = pfnHandler;
        SigAct.sa_flags                   = SA_ACK;
        __SIGSET_EMPTY(&SigAct.sa_mask);

        /*
         * Do work.
         */
        if (__SIGSET_ISSET(&__libc_gSignalRestartMask, iSignalNo))
            SigAct.sa_flags |= SA_RESTART;
        rc = __libc_Back_signalAction(iSignalNo, &SigAct, &SigActOld);

        /*
         * Check for error and return.
         */
        if (!rc)
            LIBCLOG_RETURN_P(SigActOld.__sigaction_u.__sa_handler);
        errno = -rc;
    }
    LIBCLOG_ERROR_RETURN_P(SIG_ERR);
}


