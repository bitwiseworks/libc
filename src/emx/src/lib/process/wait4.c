/* $Id: wait4.c 2254 2005-07-17 12:25:44Z bird $ */
/** @file
 *
 * LIBC - wait4().
 *
 * Copyright (c) 2004 knut st. osmundsen <bird@innotek.de>
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
#include <sys/wait.h>
#include <errno.h>
#include <InnoTekLIBC/backend.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_PROCESS
#include <InnoTekLIBC/logstrict.h>


/**
 * Waits/polls for on one or more processes to change it's running status.
 *
 * @returns Process id of the child.
 * @returns -1 and errno on failure.
 * @param   pid         Pid to wait for.
 *                      If -1, then any child process.
 *                      If > 0, then wait for process with that id.
 *                      If 0, then wait for any child process who is in the same process group id.
 *                      If < -1, then wait any child process whose process group is -pid.
 * @param   piStatus    Where to return the status.
 * @param   fOptions    Same as waitid() in our implementation.
 * @param   pRUsage     Where to return the resource usage of the child. (Optional)
 *                      This is not implemented yet - the call fails if specified.
 */
pid_t _STD(wait4)(pid_t pid, int *piStatus, int fOptions, struct rusage *pRUsage)
{
    LIBCLOG_ENTER("pid=%#x (%d) piStatus=%p fOptions=%#x pRUsage=%p\n", pid, pid, (void *)piStatus, fOptions, (void *)pRUsage);

    /*
     * Check if unsupported feature.
     */
    if (pRUsage)
    {
        LIBCLOG_ERROR("pRUsage=%p - not implemented\n", (void *)pRUsage);
        /* errno = ENOSYS;
        LIBCLOG_RETURN_INT(-1); */
    }
    if (fOptions & ~(WEXITED | WUNTRACED | WSTOPPED | WCONTINUED | WNOHANG | WNOWAIT))
    {
        errno = EINVAL;
        LIBCLOG_ERROR_RETURN_MSG(-1, "ret -1 - Unknown options %#x. (fOptions=%#x)\n",
                                 fOptions & ~(WEXITED | WUNTRACED | WSTOPPED | WCONTINUED | WNOHANG | WNOWAIT), fOptions);
    }

    /*
     * Call waitid to do the actual waiting.
     */
    /* convert pid to enmIdType and Id. */
    id_t     Id;
    idtype_t enmIdType;
    if (pid > 0)
    {
        enmIdType = P_PID;
        Id = pid;
    }
    else if (pid == 0)
    {
        enmIdType = P_PGID;
        Id = 0;
    }
    else if (pid == -1)
    {
        enmIdType = P_ALL;
        Id = 0;
    }
    else
    {
        Id = -pid;
        enmIdType = P_PGID;
    }

    /* do the call */
    siginfo_t SigInfo = {0};
    int rc = __libc_Back_processWait(enmIdType, Id, &SigInfo, fOptions | WEXITED, pRUsage);
    if (!rc)
    {
        /*
         * Convert signal info to status code.
         */
        int iStatus;
        switch (SigInfo.si_code)
        {
            default:
                LIBC_ASSERTM((fOptions & WNOHANG) && !SigInfo.si_code && !SigInfo.si_status,
                             "Invalid si_code=%#x si_status=%#x fOptions=%#x\n", SigInfo.si_code, SigInfo.si_status, fOptions);
            case CLD_EXITED:    iStatus = W_EXITCODE(SigInfo.si_status, 0); break;
            case CLD_KILLED:    iStatus = W_EXITCODE(0, SigInfo.si_status); break;
            case CLD_DUMPED:    iStatus = W_EXITCODE(0, SigInfo.si_status) | WCOREFLAG; break;
            case CLD_TRAPPED:   iStatus = W_STOPCODE(SigInfo.si_status); break;
            case CLD_STOPPED:   iStatus = W_STOPCODE(SigInfo.si_status); break;
            case CLD_CONTINUED: iStatus = W_EXITCODE(0, SigInfo.si_status); break;

        }
        if (piStatus)
            *piStatus = iStatus;
        LIBCLOG_RETURN_MSG(SigInfo.si_pid, "ret %d (%#x) iStatus=%#x\n",
                           SigInfo.si_pid, SigInfo.si_pid, iStatus);
    }

    /*
     * wait4() doesn't return EINVAL for anything but invalid fOptions
     * and since we've already validated those, they cannot be the cause
     * of EINVAL.
     */
    if (rc == -EINVAL)
        rc = -ECHILD;
    errno = -rc;
    LIBCLOG_ERROR_RETURN_INT((pid_t)-1);
}

