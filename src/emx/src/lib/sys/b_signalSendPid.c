/* $Id: b_signalSendPid.c 2254 2005-07-17 12:25:44Z bird $ */
/** @file
 *
 * LIBC SYS Backend - send signal to process.
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
#define INCL_BASE
#define INCL_FSMACROS
#define INCL_DOSSIGNALS
#include <os2emx.h>

#include <signal.h>
#include <errno.h>
#include <InnoTekLIBC/backend.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_BACK_SIGNAL
#include <InnoTekLIBC/logstrict.h>
#include "b_signal.h"
#include "syscalls.h"


/**
 * Send a signal to a process.
 *
 * Special case for iSignalNo equal to 0, where no signal is sent but permissions to
 * do so is checked.
 *
 * @returns 0 on if signal sent.
 * @returns -errno on failure.
 *
 * @param   pid         Process Id of the process which the signal is to be sent to.
 * @param   iSignalNo   The signal to send.
 *                      If 0 no signal is sent, but error handling is done as if.
 */
int     __libc_Back_signalSendPid(pid_t pid, int iSignalNo)
{
    LIBCLOG_ENTER("pid=%d iSignalNo=%d\n", pid, iSignalNo);
    int     rc;

    /*
     * Validate input.
     */
    if (!__SIGSET_SIG_VALID(iSignalNo) && iSignalNo != 0)
        LIBCLOG_ERROR_RETURN(-EINVAL, "ret -EINVAL - Invalid signal no. %d\n", iSignalNo);
    if (pid <= 0)
        LIBCLOG_ERROR_RETURN(-EINVAL, "ret -EINVAL - Invalid pid %d\n", pid);

    /*
     * Differ between others and our self.
     */
    if (_sys_pid != pid)
        rc = __libc_back_signalSendPidOther(pid, iSignalNo, NULL);
    else
    {
        if (iSignalNo != 0)
        {
            rc = __libc_Back_signalRaise(iSignalNo, NULL, NULL, 0);
            if (rc > 0)
                rc = 0;
        }
        else /* permission check, succeeded. */
            rc = 0;
    }
    if (!rc)
        LIBCLOG_RETURN_INT(rc);
    LIBCLOG_ERROR_RETURN_INT(rc);
}

