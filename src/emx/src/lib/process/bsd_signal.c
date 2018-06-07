/* $Id: bsd_signal.c 2254 2005-07-17 12:25:44Z bird $ */
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
#include <signal.h>
#include <errno.h>
#include <InnoTekLIBC/libc.h>
#include <InnoTekLIBC/backend.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_SIGNAL
#include <InnoTekLIBC/logstrict.h>


/**
 * BSD style signal().
 *
 * @returns pointer to previous signal handler.
 * @param   iSignalNo   Signal number to set the handler for.
 * @param   pfnHandler  Pointer to the new signal handler.
 */
void (*_STD(bsd_signal)(int iSignalNo, void (*pfnHandler)(int)))(int)
{
    LIBCLOG_ENTER("iSignalNo=%d pfnHandler=%p\n", iSignalNo, (void*)pfnHandler);
    struct sigaction    SigAct;
    struct sigaction    SigActOld;
    int                 rc;

    /*
     * Validate input.
     */
    if (pfnHandler == SIG_ACK)
    {
        errno = EINVAL;
        LIBCLOG_ERROR_RETURN(SIG_ERR, "ret SIG_ERR - SIG_ACK is not supported by BSD style signal handling!\n");

    }
    if (pfnHandler == SIG_HOLD)
    {
        errno = EINVAL;
        LIBCLOG_ERROR_RETURN(SIG_ERR, "ret SIG_ERR - SIG_HOLD is not supported by BSD style signal handling!\n");
    }
    if (!__SIGSET_SIG_VALID(iSignalNo))
    {
        errno = EINVAL;
        LIBCLOG_ERROR_RETURN(SIG_ERR, "ret SIG_ERR - Invalid signal number %d\n", iSignalNo);
    }

    /*
     * Initialize action.
     */
    SigAct.__sigaction_u.__sa_handler = pfnHandler;
    SigAct.sa_flags                   = 0;
    __SIGSET_EMPTY(&SigAct.sa_mask);
    if (__SIGSET_ISSET(&__libc_gSignalRestartMask, iSignalNo))
        SigAct.sa_flags |= SA_RESTART;

    /*
     * Change signal action.
     */
    rc = __libc_Back_signalAction(iSignalNo, &SigAct, &SigActOld);
    if (!rc)
        LIBCLOG_RETURN_P(SigActOld.__sigaction_u.__sa_handler);
    errno = -rc;
    LIBCLOG_ERROR_RETURN_P(SIG_ERR);
}

