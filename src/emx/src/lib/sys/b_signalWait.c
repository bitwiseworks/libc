/* $Id: b_signalWait.c 2254 2005-07-17 12:25:44Z bird $ */
/** @file
 *
 * LIBC SYS Backend - signal wait.
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
#define INCL_BASE
#define INCL_FSMACROS
#include <os2emx.h>

#include <signal.h>
#include <errno.h>
#include <386/builtin.h>
#include "b_signal.h"
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
 * @returns Negative error code (errno) on failure.
 * @param   pSigSet     Signals to wait for.
 * @param   pSigInfo    Where to store the signal info for the signal
 *                      that we accepted.
 * @param   pTimeout    Timeout specification. If NULL wait for ever.
 */
int         __libc_Back_signalWait(const sigset_t *pSigSet, siginfo_t *pSigInfo, const struct timespec *pTimeout)
{
    LIBCLOG_ENTER("pSigSet=%p {%#08lx%#08lx} pSigInfo=%p pTimeout=%p {%d, %ld}\n",
                  (void *)pSigSet, pSigSet ? pSigSet->__bitmap[1] : 0, pSigSet ? pSigSet->__bitmap[0] : 0,
                  (void *)pSigInfo,
                  (void *)pTimeout, pTimeout ? pTimeout->tv_sec : ~0, pTimeout ? pTimeout->tv_nsec : ~0);

    __LIBC_PTHREAD  pThrd = __libc_threadCurrent();
    sigset_t                        SigSet;
    __LIBC_THREAD_SIGWAIT           SigWait = {0};
    struct timespec                 Timeout;
    int                             rc;

    /*
     * Make copy of the input (we can safely crash here).
     */
    SigSet = *pSigSet;
    if (pTimeout)
        Timeout = *pTimeout;
    __SIGSET_CLEAR(&SigSet, SIGKILL);
    __SIGSET_CLEAR(&SigSet, SIGSTOP);
    if (__SIGSET_ISEMPTY(&SigSet))
        LIBCLOG_ERROR_RETURN(-EINVAL, "ret -EINVAL - Signal set is empty! (or you tried to wait for SIGKILL/SIGSTOP)\n");

    /*
     * Gain exclusive access to the signal stuff.
     */
    rc = __libc_back_signalSemRequest();
    if (rc)
        LIBCLOG_ERROR_RETURN_INT(rc);

    /*
     * Check if the requested signals are pending.
     */
    if (    !__SIGSET_ISEMPTY(&pThrd->SigSetPending)
        ||  !__SIGSET_ISEMPTY(&__libc_gSignalPending))
    {
        sigset_t    SigSetPending;
        __SIGSET_OR(&SigSetPending, &pThrd->SigSetPending, &__libc_gSignalPending);
        __SIGSET_AND(&SigSetPending, &SigSetPending, &SigSet);
        if (!__SIGSET_ISEMPTY(&SigSetPending))
        {
            rc = __libc_back_signalAccept(pThrd, 0, &SigSetPending, (siginfo_t *)&SigWait.SigInfo);
            if (rc > 0)
            {
                if (pSigInfo)
                    *pSigInfo = SigWait.SigInfo;
                __libc_back_signalSemRelease();
                LIBCLOG_RETURN_INT(rc);
            }
            LIBC_ASSERT_FAILED();
        }
    }

    /*
     * Mark thread as blocked in sigtimedwait().
     */
    if (pThrd->enmStatus != enmLIBCThreadStatus_unknown)
    {
        LIBC_ASSERTM_FAILED("Must push thread state!\n");
        __libc_back_signalSemRelease();
        LIBCLOG_ERROR_RETURN_INT(-EDEADLK);
    }
    SigWait.fDone           = 0;
    SigWait.SigSetWait      = SigSet;
    SigWait.SigInfo.si_signo= 0;
    pThrd->u.pSigWait   = &SigWait;
    pThrd->enmStatus    = enmLIBCThreadStatus_sigwait;

    /*
     * Wait till state changes back and then return according to the wait result.
     *
     * EAGAIN means time out. While EINTR means that we've been interrupted by
     * the delivery of a signal, which might mean that we got what we waited
     * for or that some other signal was delivered to this thread. Very simple. :-)
     */
    rc = __libc_back_signalWait(pThrd, &SigWait.fDone, pTimeout ? &Timeout : NULL);
    pThrd->enmStatus = enmLIBCThreadStatus_unknown;
    __libc_back_signalSemRelease();
    if (rc == -EINTR)
    {
        if (SigWait.fDone && SigWait.SigInfo.si_signo > 0)
        {
            rc = SigWait.SigInfo.si_signo;
            if (pSigInfo)
                *pSigInfo = SigWait.SigInfo;
        }
    }


    LIBCLOG_RETURN_MSG(rc, "ret %d SigWait.SigInfo={si_signo=%d, si_errno=%d, si_code=%#x, si_timestamp=%#x, si_flags=%#x, si_pid=%d, si_tid=%d, si_uid=%d, si_status=%d, si_addr=%p, si_value=%#08x, si_band=%ld}\n",
                       rc, SigWait.SigInfo.si_signo, SigWait.SigInfo.si_errno, SigWait.SigInfo.si_code, SigWait.SigInfo.si_timestamp,
                       SigWait.SigInfo.si_flags, SigWait.SigInfo.si_pid, SigWait.SigInfo.si_tid, SigWait.SigInfo.si_uid, SigWait.SigInfo.si_status,
                       SigWait.SigInfo.si_addr, SigWait.SigInfo.si_value.sigval_int, SigWait.SigInfo.si_band);
}

