/* $Id: sigvec.c 2254 2005-07-17 12:25:44Z bird $ */
/** @file
 *
 * LIBC - sigvec().
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
 * Sets and/or query the signal action for a signal.
 *
 * This API is an early version of the sigaction() interface and
 * provided for compatability with old sources.
 *
 * @returns 0 on success
 * @returns -1 and errno set on failure.
 * @param   iSignalNo   Signal which action is to be changed or/and queried.
 * @param   pSigVec     New signal action. NULL allowed.
 * @param   pSigVecOld  Where to store the previous signal action. NULL allowed.
 * @obsolete
 */
int _STD(sigvec)(int iSignalNo, struct sigvec *pSigVec, struct sigvec *pSigVecOld)
{
    LIBCLOG_ENTER("iSignalNo=%d pSigVec=%p {sv_handle=%p, sv_mask=%#x, sv_flags=%#x} pSigVecOld=%p\n",
                  iSignalNo, (void *)pSigVec,
                  pSigVec ? (void *)pSigVec->sv_handler : NULL,
                  pSigVec ? pSigVec->sv_mask : 0,
                  pSigVec ? pSigVec->sv_flags : 0,
                  (void *)pSigVecOld);
    struct sigaction    SigAct;
    struct sigaction    SigActOld;
    int                 rc;

    /*
     * sigvec doesn't allow any operation on non-catachble signals.
     */
    if (iSignalNo == SIGKILL || iSignalNo == SIGSTOP)
    {
        errno = EINVAL;
        LIBCLOG_ERROR_RETURN(-1, "ret -1 - Invalid signal %d\n", iSignalNo);
    }

    /*
     * Forward to sigaction() converting the input and output as we go along.
     */
    if (pSigVec)
    {
        SigAct.__sigaction_u.__sa_handler   = pSigVec->sv_handler;
        SigAct.sa_flags                     = pSigVec->sv_flags ^ SV_INTERRUPT;
        __SIGSET_EMPTY(&SigAct.sa_mask);
        SigAct.sa_mask.__bitmap[0] = pSigVec->sv_mask;
    }

    rc = sigaction(iSignalNo, pSigVec ? &SigAct : NULL, &SigActOld);
    if (rc)
        LIBCLOG_ERROR_RETURN_INT(rc);

    if (pSigVecOld)
    {
        pSigVecOld->sv_handler  = SigActOld.__sigaction_u.__sa_handler;
        pSigVecOld->sv_flags    = SigActOld.sa_flags ^ SV_INTERRUPT;
        pSigVecOld->sv_mask     = *(int *)SigActOld.sa_mask.__bitmap[0];
    }

    LIBCLOG_RETURN_MSG(0, "ret 0 (*pSigVecOld {sv_handle=%p, sv_mask=%#lx, sv_flags=%#x})\n",
                       (void *)SigActOld.__sigaction_u.__sa_sigaction,
                       SigActOld.sa_mask.__bitmap[0],
                       SigActOld.sa_flags ^ SV_INTERRUPT);
}

