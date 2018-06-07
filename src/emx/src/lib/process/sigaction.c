/* $Id: sigaction.c 2254 2005-07-17 12:25:44Z bird $ */
/** @file
 *
 * LIBC - sigaction().
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
#include <string.h>
#include <InnoTekLIBC/backend.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_SIGNAL
#include <InnoTekLIBC/logstrict.h>



/**
 * Set and/or query the signal action for a signal.
 *
 * Signal actions are per process as per the POSIX specs.
 *
 * @returns 0 on success.
 * @returns -1 on failure, errno set.
 * @param   iSignalNo   Signal number which action is to be changed and/or queried.
 * @param   pSigAct     The new signal action. NULL allowed.
 * @param   pSigActOld  Where to store the old signal action. NULL allowed.
 */
int _STD(sigaction)(int iSignalNo, const struct sigaction *pSigAct, struct sigaction *pSigActOld)
{
    LIBCLOG_ENTER("iSignalNo=%d pSigAct=%p {sa_handler=%p, sa_mask={%#08lx%#08lx}, sa_flags=%#x} pSigActOld=%p\n",
                  iSignalNo, (void *)pSigAct,
                  pSigAct ? (void*)pSigAct->__sigaction_u.__sa_sigaction : NULL,
                  pSigAct ? pSigAct->sa_mask.__bitmap[0] : 0,
                  pSigAct ? pSigAct->sa_mask.__bitmap[1] : 0,
                  pSigAct ? pSigAct->sa_flags : 0,
                  (void *)pSigActOld);
    struct sigaction    SigAct;
    struct sigaction    SigActOld = {{0}};

    /*
     * Make copy of the input (we can safely crash here) and validate it.
     */
    if (    !__SIGSET_SIG_VALID(iSignalNo)
        ||  (   pSigAct
             && (iSignalNo == SIGSTOP || iSignalNo == SIGKILL)))
    {
        errno = EINVAL;
        LIBCLOG_ERROR_RETURN(-1, "ret -1 - Invalid signal no %d\n", iSignalNo);
    }
    if (pSigAct)
    {
        SigAct = *pSigAct;
        if (SigAct.sa_flags & ~(  SA_ONSTACK | SA_RESTART | SA_RESETHAND | SA_NODEFER | SA_NOCLDWAIT
                                | SA_SIGINFO | SA_NOCLDSTOP | SA_ACK | SA_SYSV))
        {
            errno = EINVAL;
            LIBCLOG_ERROR_RETURN(-1, "ret -1 - Invalid sa_flags=%#x\n", SigAct.sa_flags);
        }
        if (    SigAct.__sigaction_u.__sa_handler == SIG_ERR
            ||  SigAct.__sigaction_u.__sa_handler == SIG_ACK
            ||  SigAct.__sigaction_u.__sa_handler == SIG_HOLD)
        {
            errno = EINVAL;
            LIBCLOG_ERROR_RETURN(-1, "ret -1 - Invalid sa_handler=%p\n", (void*)SigAct.__sigaction_u.__sa_handler);
        }

        /*
         * Check if the address is around.
         */
        if (    SigAct.__sigaction_u.__sa_handler != SIG_IGN
            &&  SigAct.__sigaction_u.__sa_handler != SIG_DFL)
        {
            char ach[1];
            memcpy(&ach, (void *)SigAct.__sigaction_u.__sa_handler, sizeof(ach));  /* check if address is around. */
        }
    }

    /*
     * Call backend worker.
     */
    int rc = __libc_Back_signalAction(iSignalNo, pSigAct ? &SigAct : NULL, &SigActOld);
    if (!rc)
    {
        if (pSigActOld)
            *pSigActOld = SigActOld;

        LIBCLOG_RETURN_MSG(0, "ret 0 (*pSigActOld=%p {sa_handler=%p, sa_mask={%#08lx%#08lx}, sa_flags=%#x}\n",
                           (void *)pSigActOld,
                           (void *)SigActOld.__sigaction_u.__sa_sigaction,
                           SigActOld.sa_mask.__bitmap[0],
                           SigActOld.sa_mask.__bitmap[1],
                           SigActOld.sa_flags);
    }
    errno = -rc;
    LIBCLOG_ERROR_RETURN_INT(-1);
}

