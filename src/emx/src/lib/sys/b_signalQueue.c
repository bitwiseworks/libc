/* $Id: b_signalQueue.c 2254 2005-07-17 12:25:44Z bird $ */
/** @file
 *
 * LIBC SYS Backend - sigqueue().
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
#include "syscalls.h"



/**
 * Queue a signal.
 *
 * @returns 0 on success.
 * @returns -1 on failure, errno set.
 * @param   pid             The target process id.
 * @param   iSignalNo       Signal to queue.
 * @param   SigVal          The value to associate with the signal.
 */
int __libc_Back_signalQueue(pid_t pid, int iSignalNo, const union sigval SigVal)
{
    LIBCLOG_ENTER("pid=%d iSignalNo=%d SigVal=%p\n", pid, iSignalNo, SigVal.sigval_ptr);

    /*
     * Validate.
     */
    if (!__SIGSET_SIG_VALID(iSignalNo) && iSignalNo != 0)
        LIBCLOG_ERROR_RETURN(-EINVAL, "ret -EINVAL - Invalid signal no %d\n", iSignalNo);

    /*
     * Verify pid.
     */
    siginfo_t   SigInfo = {0};
    SigInfo.si_signo    = iSignalNo;
    SigInfo.si_code     = SI_QUEUE;
    SigInfo.si_flags   |= __LIBC_SI_QUEUED;
    SigInfo.si_value    = SigVal;
    int rc;
    if (pid == _sys_pid)
    {
        if (iSignalNo)
            rc = __libc_back_signalQueueSelf(iSignalNo, &SigInfo);
        else /* permission only */
            rc = 0;
    }
    else
        rc = __libc_back_signalSendPidOther(pid, iSignalNo, &SigInfo);

    /*
     * Done.
     */
    if (!rc)
        LIBCLOG_RETURN_INT(rc);
    LIBCLOG_ERROR_RETURN_INT(rc);
}

