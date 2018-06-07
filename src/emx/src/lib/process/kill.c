/* $Id: $ */
/** @file
 *
 * LIBC - kill().
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
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_SIGNAL
#include <InnoTekLIBC/logstrict.h>



/**
 * Send a signal to a process.
 *
 * If the signal or any other signal is scheduled for the current thread, it will
 * be delivered before the function returns.
 *
 * @returns 0 on success.
 * @returns -1 with errno set to ESRCH if pid didn't specify any existing process.
 * @returns -1 with errno set to EINVAL if iSignalNo is not a valid signal.
 * @returns -1 with errno set to EPERM if the process doesn't have permission
 *          to send the signal to any of the processes specified by pid.
 *
 * @param   pid         Specification of which processes to send the signal to.
 *                      If 0 Then send to all processes in the same process group.
 *                      If < 0 Then send to all processes in the process group abs(pid).
 *                      If > 0 Then send to the process which process identifier is pid.
 * @param   iSignalNo   Signal to send.
 *                      If 0 Then only check if any processes specified by pid is around.
 */
int _STD(kill)(pid_t pid, int iSignalNo)
{
    LIBCLOG_ENTER("pid=%d iSignalNo=%d\n", pid, iSignalNo);
    int rc;

    /*
     * Validate input.
     */
    if (!__SIGSET_SIG_VALID(iSignalNo) && iSignalNo != 0)
    {
        errno = EINVAL;
        LIBCLOG_ERROR_RETURN(-1, "ret -1 - Invalid signal number %d\n", iSignalNo);
    }

    /*
     * Call backend to do the rest.
     */
    if (pid > 0)
        rc = __libc_Back_signalSendPid(pid, iSignalNo);
    else /* (pid <= 0) */
        rc = __libc_Back_signalSendPGrp(-pid, iSignalNo);
    if (!rc)
        LIBCLOG_RETURN_INT(0);
    errno = -rc;
    LIBCLOG_ERROR_RETURN_INT(-1);
}
