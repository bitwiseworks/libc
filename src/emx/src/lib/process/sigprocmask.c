/* $Id: sigprocmask.c 2254 2005-07-17 12:25:44Z bird $ */
/** @file
 *
 * LIBC - sigprocmask().
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
#include <sys/_sigset.h>
#include <errno.h>
#include <InnoTekLIBC/thread.h>
#include <InnoTekLIBC/backend.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_SIGNAL
#include <InnoTekLIBC/logstrict.h>


/**
 * Block or unblock signal deliveries.
 *
 * This API was by the POSIX specs only intended for use in a single threaded process,
 * however we define it as equivalent to pthread_sigmask().
 *
 * @returns 0 on success.
 * @returns -1 and errno set to EINVAL on failure.
 * @param   iHow        Describes the action taken if pSigSetNew not NULL. Recognized
 *                      values are SIG_BLOCK, SIG_UNBLOCK or SIG_SETMASK.
 *
 *                      SIG_BLOCK means to OR the sigset pointed to by pSigSetNew with
 *                          the signal mask for the current thread.
 *                      SIG_UNBLOCK means to AND the 0 complement of the sigset pointed
 *                          to by pSigSetNew with the signal mask of the current thread.
 *                      SIG_SETMASK means to REPLACE the signal mask of the current thread
 *                          to the sigset pointed to by pSigSetNew.
 *
 * @param   pSigSetNew  Pointer to signal set which will be applied to the current
 *                      threads signal mask according to iHow. If NULL no change
 *                      will be made the the current threads signal mask.
 * @param   pSigSetOld  Where to store the current threads signal mask prior to applying
 *                      pSigSetNew to it. This parameter can be NULL.
 */
int _STD(sigprocmask)(int iHow, const sigset_t * __restrict pSigSetNew, sigset_t * __restrict pSigSetOld)
{
    LIBCLOG_ENTER("iHow=%d pSigSetNew=%p {%08lx%08lx} pSigSetOld=%p\n",
                  iHow,
                  (void *)pSigSetNew,
                  pSigSetNew ? pSigSetNew->__bitmap[1] : 0, pSigSetNew ? pSigSetNew->__bitmap[0] : 0,
                  (void *)pSigSetOld);

    /*
     * Perform operation.
     */
    sigset_t        SigSetOld;
    int rc = __libc_Back_signalMask(__libc_threadCurrent(), iHow, pSigSetNew, &SigSetOld);
    if (!rc)
    {
        if (pSigSetOld)
            *pSigSetOld = SigSetOld;
        LIBCLOG_RETURN_MSG(0, "ret 0 old={%08lx%08lx}\n", SigSetOld.__bitmap[1], SigSetOld.__bitmap[0]);
    }
    errno = -rc;
    LIBCLOG_ERROR_RETURN_INT(-1);
}

