/* $Id: b_signalMask.c 2308 2005-08-27 16:56:52Z bird $ */
/** @file
 *
 * LIBC SYS Backend - sigprocmask().
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
#include <InnoTekLIBC/thread.h>
#include <InnoTekLIBC/backend.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_BACK_SIGNAL
#include <InnoTekLIBC/logstrict.h>
#include "b_signal.h"


/**
 * Block or unblock signal deliveries of a thread.
 *
 * @returns 0 on success.
 * @returns Negative error code (errno) on failure.
 * @param   pThrd       Thread to apply this to.
 * @param   iHow        Describes the action taken if pSigSetNew not NULL. Recognized
 *                      values are SIG_BLOCK, SIG_UNBLOCK or SIG_SETMASK.
 *
 *                      SIG_BLOCK means to or the sigset pointed to by pSigSetNew with
 *                          the signal mask for the current thread.
 *                      SIG_UNBLOCK means to and the 0 complement of the sigset pointed
 *                          to by pSigSetNew with the signal mask of the current thread.
 *                      SIG_SETMASK means to set the signal mask of the current thread
 *                          to the sigset pointed to by pSigSetNew.
 *
 * @param   pSigSetNew  Pointer to signal set which will be applied to the current
 *                      threads signal mask according to iHow. If NULL no change
 *                      will be made the the current threads signal mask.
 * @param   pSigSetOld  Where to store the current threads signal mask prior to applying
 *                      pSigSetNew to it. This parameter can be NULL.
 */
int __libc_Back_signalMask(__LIBC_PTHREAD pThrd, int iHow, const sigset_t * __restrict pSigSetNew, sigset_t * __restrict pSigSetOld)
{
    LIBCLOG_ENTER("pThrd=%p iHow=%d pSigSetNew=%p {%08lx%08lx} pSigSetOld=%p\n",
                  (void *)pThrd, iHow,
                  (void *)pSigSetNew, pSigSetNew ? pSigSetNew->__bitmap[1] : 0, pSigSetNew ? pSigSetNew->__bitmap[0] : 0,
                  (void *)pSigSetOld);
    sigset_t        SigSetNew;
    sigset_t        SigSetOld;

    /*
     * Validate input.
     * ASSUMES SIG_BLOCK 1 and SIG_SETMASK 3.
     */
    if (iHow < SIG_BLOCK || iHow > SIG_SETMASK)
        LIBCLOG_ERROR_RETURN(-EINVAL, "ret -EINVAL - iHow=%d is incorrect!\n", iHow);


    /*
     * Prepare action.
     * We make copies on the stack so we will not crash accessing
     * bogus stuff inside the signal mutex.
     */
    __SIGSET_EMPTY(&SigSetOld);         /* touch it. (paranoia) */
    if (pSigSetNew)
        SigSetNew = *pSigSetNew;
    else
        __SIGSET_EMPTY(&SigSetNew);

    /*
     * Gain exclusive access to the signal stuff.
     */
    int fRelease = 1;
    int rc = __libc_back_signalSemRequest();
    if (rc)
        LIBCLOG_ERROR_RETURN_INT(rc);

    /*
     * Save old set of blocked signals and apply any changes.
     */
    SigSetOld = pThrd->SigSetBlocked;
    if (pSigSetNew)
    {
        switch (iHow)
        {
            case SIG_BLOCK:
                __SIGSET_OR(&pThrd->SigSetBlocked, &pThrd->SigSetBlocked, &SigSetNew);
                break;

            case SIG_UNBLOCK:
                __SIGSET_NOT(&SigSetNew);
                __SIGSET_AND(&pThrd->SigSetBlocked, &pThrd->SigSetBlocked, &SigSetNew);
                break;

            case SIG_SETMASK:
                pThrd->SigSetBlocked = SigSetNew;
                break;
        }
        __SIGSET_CLEAR(&pThrd->SigSetBlocked, SIGKILL);
        __SIGSET_CLEAR(&pThrd->SigSetBlocked, SIGSTOP);

        /*
         * Check if there are any pending signals with the new mask.
         */
        sigset_t    SigSetNotBlocked = pThrd->SigSetBlocked;
        __SIGSET_NOT(&SigSetNotBlocked);
        sigset_t    SigSetPending;
        __SIGSET_OR(&SigSetPending, &pThrd->SigSetPending, &__libc_gSignalPending);
        __SIGSET_AND(&SigSetPending, &SigSetPending, &SigSetNotBlocked);
        if (!__SIGSET_ISEMPTY(&SigSetPending))
        {
            __libc_back_signalReschedule(pThrd);
            fRelease = 0;
        }
    }

    /*
     * Release semaphore.
     */
    if (fRelease)
        __libc_back_signalSemRelease();

    /*
     * Copy old signal set if requested.
     */
    if (pSigSetOld)
        *pSigSetOld = SigSetOld;

    LIBCLOG_MSG("old={%08lx%08lx} new={%08lx%08lx}\n",
                SigSetOld.__bitmap[1], SigSetOld.__bitmap[0],
                SigSetNew.__bitmap[1], SigSetNew.__bitmap[0]);
    LIBCLOG_RETURN_INT(0);
}

