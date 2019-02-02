/* $Id: waitid.c 2254 2005-07-17 12:25:44Z bird $ */
/** @file
 *
 * LIBC - waitid().
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
 * @returns 0 on success, pSigInfo containing status info.
 * @returns -1 and errno on failure.
 *          ECHILD is returned as specfied for waitpid() in SuS. The waitid() errno specs
 *          are very short and doesn't explain what they mean by "idtype and id specify an
 *          invalid set of processes".
 * @param   enmIdType   What kind of process specification Id contains.
 * @param   Id          Process specification of the enmIdType sort.
 * @param   pSigInfo    Where to store the result.
 * @param   fOptions    The WEXITED, WUNTRACED, WSTOPPED and WCONTINUED flags are used to
 *                      select the events to report. WNOHANG is used for preventing the api
 *                      from blocking. And WNOWAIT is used for peeking.
 */
int _STD(waitid)(idtype_t enmIdType, id_t Id, siginfo_t *pSigInfo, int fOptions)
{
    LIBCLOG_ENTER("enmIdType=%d Id=%#llx (%lld) pSigInfo=%p fOptions=%#x\n", enmIdType, Id, Id, (void *)pSigInfo, fOptions);

    /*
     * Validate options.
     */
    if (!(fOptions & (WEXITED | WUNTRACED | WSTOPPED | WCONTINUED)))
    {
        errno = EINVAL;
        LIBCLOG_ERROR_RETURN_MSG(-1, "ret -1 - No event was selected! fOptions=%#x\n", fOptions);
    }
    if (fOptions & ~(WEXITED | WUNTRACED | WSTOPPED | WCONTINUED | WNOHANG | WNOWAIT))
    {
        errno = EINVAL;
        LIBCLOG_ERROR_RETURN_MSG(-1, "ret -1 - Unknown options %#x. (fOptions=%#x)\n",
                                 fOptions & ~(WEXITED | WUNTRACED | WSTOPPED | WCONTINUED | WNOHANG | WNOWAIT), fOptions);
    }


    /*
     * Do the requested job.
     */
    int rc;
    siginfo_t   SigInfo = {0};
    switch (enmIdType)
    {
        case P_PID:
        case P_PGID:
        case P_ALL:
            rc = __libc_Back_processWait(enmIdType, (pid_t)Id, &SigInfo, fOptions, NULL);
            break;
        default:
            rc = -EINVAL;
            LIBCLOG_ERROR_RETURN_MSG(-1, "ret -1 - Invalid enmIdType=%d!\n", enmIdType);
            break;
    }

    /*
     * Handle return code.
     */
    if (!rc)
    {
        if (pSigInfo)
            *pSigInfo = SigInfo;
        LIBCLOG_RETURN_MSG(0, "ret 0 SigInfo.si_pid=%#x SigInfo.si_code=%#x SigInfo.si_status=%#x\n",
                           SigInfo.si_pid, SigInfo.si_code, SigInfo.si_status);
    }
    errno = -rc;
    LIBCLOG_ERROR_RETURN_INT(-1);
}
