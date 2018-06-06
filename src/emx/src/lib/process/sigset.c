/* $Id: sigset.c 2254 2005-07-17 12:25:44Z bird $ */
/** @file
 *
 * LIBC - BSD Style signal().
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
#include <errno.h>
#include <signal.h>
#include <InnoTekLIBC/libc.h>
#include <InnoTekLIBC/backend.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_SIGNAL
#include <InnoTekLIBC/logstrict.h>


/**
 * Modify the signal dispositions.
 *

   The disp argument specifies the signal's disposition, which may be SIG_DFL,
   SIG_IGN, or the address of a signal handler.

   If sigset() is used, and disp is the address of a signal handler, the system
   shall add sig to the calling process' signal mask before executing the signal
    handler; when the signal handler returns, the system shall restore the calling
    process' signal mask to its state prior to the delivery of the signal.

   In addition, if sigset() is used, and disp is equal to SIG_HOLD, sig shall be
   added to the calling process' signal mask and sig's disposition shall remain
   unchanged.

   If sigset() is used, and disp is not equal to SIG_HOLD, sig shall be removed
   from the calling process' signal mask.
 *
 * @returns pointer to previous signal handler.
 * @param   iSignalNo   Signal number to set the handler for.
 * @param   pfnHandler  Pointer to the new signal handler.
 */
void (*_STD(sigset)(int iSignalNo, void (*pfnHandler)(int)))(int)
{
    LIBCLOG_ENTER("iSignalNo=%d pfnHandler=%p\n", iSignalNo, (void *)pfnHandler);

    /*
     * Validate input.
     */
    if (   !__SIGSET_SIG_VALID(iSignalNo)
        ||  iSignalNo == SIGKILL
        ||  iSignalNo == SIGSTOP)
    {
        errno = EINVAL;
        LIBCLOG_ERROR_RETURN(SIG_ERR, "ret SIG_ERR - Invalid signal %d!\n", iSignalNo);
    }
    if (    pfnHandler == SIG_ERR
        ||  pfnHandler == SIG_ACK)
    {
        errno = EINVAL;
        LIBCLOG_ERROR_RETURN(SIG_ERR, "ret SIG_ERR - Invalid handler %p!\n", (void *)pfnHandler);
    }

    /*
     * Handle the special signal hold operation.
     */
    struct sigaction  SigActOld;
    if (pfnHandler == SIG_HOLD)
    {
        sigset_t    SigSet;
        __SIGSET_EMPTY(&SigSet);
        __SIGSET_SET(&SigSet, iSignalNo);
        int rc = sigprocmask(SIG_BLOCK, &SigSet, NULL);
        if (!rc)
        {
            LIBCLOG_RETURN_P(SIG_HOLD);
            //rc = sigaction(iSignalNo, NULL, &SigActOld);
            //if (!rc)
            //    LIBCLOG_RETURN_P(SigActOld.__sigaction_u.__sa_handler);
            //LIBC_ASSERTM_FAILED("sigaction failed with rc=%d errno=%d for querying iSignalNo=%d\n", rc, errno, iSignalNo);
            //LIBCLOG_RETURN_P(SIG_DFL);
        }
    }
    else
    {
        /*
         * Initialize action.
         */
        struct sigaction SigAct;
        SigAct.__sigaction_u.__sa_handler = pfnHandler;
        SigAct.sa_flags                   = 0;
        __SIGSET_EMPTY(&SigAct.sa_mask);
        if (__SIGSET_ISSET(&__libc_gSignalRestartMask, iSignalNo))
            SigAct.sa_flags |= SA_RESTART;

        /*
         * Change signal action.
         */
        int rc = sigaction(iSignalNo, &SigAct, &SigActOld);
        if (!rc)
            LIBCLOG_RETURN_P(SigActOld.__sigaction_u.__sa_handler);
    }

    LIBCLOG_ERROR_RETURN_P(SIG_ERR);
}

