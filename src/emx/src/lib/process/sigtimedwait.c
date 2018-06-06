/* $Id: sigtimedwait.c 2254 2005-07-17 12:25:44Z bird $ */
/** @file
 *
 * LIBC - sigtimedwait().
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
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_SIGNAL
#include <InnoTekLIBC/logstrict.h>



/**
 * Wait for one or more signals and remove and return the first of them
 * to occur.
 *
 * Will return immediately if one of the signals is already pending. If more than
 * one signal is pending the signal with highest priority will be returned.
 *
 * @returns Signal number on success.
 * @returns -1 on failure, errno set.
 * @param   pSigSet     Signals to wait for.
 * @param   pSigInfo    Where to store the signal info for the signal
 *                      that we accepted.
 * @param   pTimeout    Timeout specification. If NULL wait for ever.
 */
int _STD(sigtimedwait)(const sigset_t *pSigSet, siginfo_t *pSigInfo, const struct timespec *pTimeout)
{
    LIBCLOG_ENTER("pSigSet=%p {%#08lx%#08lx} pSigInfo=%p pTimeout=%p {%d, %ld}\n",
                  (void *)pSigSet, pSigSet ? pSigSet->__bitmap[1] : 0, pSigSet ? pSigSet->__bitmap[0] : 0,
                  (void *)pSigInfo,
                  (void *)pTimeout, pTimeout ? pTimeout->tv_sec : ~0, pTimeout ? pTimeout->tv_nsec : ~0);

    /*
     * Perform the operation.
     */
    siginfo_t SigInfo;
    int rc = __libc_Back_signalWait(pSigSet, &SigInfo, pTimeout);
    if (rc >= 0)
    {
        if (pSigInfo)
            *pSigInfo = SigInfo;
        LIBCLOG_RETURN_MSG(rc, "ret %d (*pSigInfo={si_signo=%d, si_errno=%d, si_code=%#x, si_timestamp=%#x, si_flags=%#x, si_pid=%d, si_tid=%d, si_uid=%d, si_status=%d, si_addr=%p, si_value=%#08x, si_band=%ld})\n",
                           rc, SigInfo.si_signo, SigInfo.si_errno, SigInfo.si_code, SigInfo.si_timestamp,
                           SigInfo.si_flags, SigInfo.si_pid, SigInfo.si_tid, SigInfo.si_uid, SigInfo.si_status,
                           SigInfo.si_addr, SigInfo.si_value.sigval_int, SigInfo.si_band);
    }

    errno = -rc;
    LIBCLOG_ERROR_RETURN_INT(-1);
}

