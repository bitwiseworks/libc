/* $Id: b_signalInterrupt.c 2254 2005-07-17 12:25:44Z bird $ */
/** @file
 *
 * LIBC SYS Backend - siginterrupt().
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
#include <InnoTekLIBC/backend.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_BACK_SIGNAL
#include <InnoTekLIBC/logstrict.h>
#include "b_signal.h"


/**
 * Change interrupt/restart system call properties for a signal.
 *
 * @returns 0 on success.
 * @returns Negative error code (errno) on failure.
 * @param   iSignalNo   Signal number to change interrupt/restart
 *                      properties for.
 * @param   fFlag       If set Then clear the SA_RESTART from the handler action.
 *                      If clear Then set the SA_RESTART from the handler action.
 * @remark  The SA_RESTART flag is inherited when using signal().
 */
int __libc_Back_signalInterrupt(int iSignalNo, int fFlag)
{
    LIBCLOG_ENTER("iSignalNo=%d fFlag=%d\n", iSignalNo, fFlag);
    struct sigaction    SigAct = {{0}};
    int                 rc;
    int                 rc2 = 0;

    /*
     * Validate.
     */
    if (!__SIGSET_SIG_VALID(iSignalNo))
        LIBCLOG_ERROR_RETURN(-EINVAL, "ret -EINVAL - Invalid signal no %d\n", iSignalNo);

    /*
     * Gain exclusive access to the signal stuff.
     */
    rc = __libc_back_signalSemRequest();
    if (rc)
        LIBCLOG_ERROR_RETURN_INT(rc);

    /*
     * Get the previous signal action.
     */
    rc = __libc_back_signalAction(iSignalNo, NULL, &SigAct);
    if (!rc)
    {
        if (fFlag)
        {   /* interrupt */
            SigAct.sa_flags &= ~SA_RESTART;
            __SIGSET_CLEAR(&__libc_gSignalRestartMask, iSignalNo);
        }
        else
        {   /* restart */
            SigAct.sa_flags |= SA_RESTART;
            __SIGSET_SET(&__libc_gSignalRestartMask, iSignalNo);
        }
        rc2 = __libc_back_signalAction(iSignalNo, &SigAct, NULL);
    }

    /*
     * Release semaphore.
     */
    __libc_back_signalSemRelease();

    /*
     * Check for failure and return.
     */
    if (!rc && !rc2)
        LIBCLOG_RETURN_INT(0);
    LIBC_ASSERTM(!rc2, "Impossible!\n");
    if (!rc)
        rc = rc2;
    LIBCLOG_ERROR_RETURN_INT(rc);
}

