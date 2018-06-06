/* $Id: sigwait.c 2254 2005-07-17 12:25:44Z bird $ */
/** @file
 *
 * LIBC - sigwait().
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
#include <InnoTekLIBC/thread.h>
#include <InnoTekLIBC/backend.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_SIGNAL
#include <InnoTekLIBC/logstrict.h>



/**
 * Wait for a signal to become pending.
 *
 * @returns 0 on success.
 * @returns -1 on failure, errno set.
 * @param   pSigSet     Signals to wait for.
 * @param   piSignalNo  Where to store the signal number which we accepted.
 */
int _STD(sigwait)(const sigset_t *pSigSet, int *piSignalNo)
{
    LIBCLOG_ENTER("pSigSet=%p {%#08lx%#08lx} pSignalNo=%p\n",
                  (void *)pSigSet, pSigSet ? pSigSet->__bitmap[1] : 0, pSigSet ? pSigSet->__bitmap[0] : 0,
                  (void *)piSignalNo);
    int         rc;
    siginfo_t   SigInfo = {0};

    /*
     * Forward this call to sigtimedwait().
     */
    rc = sigtimedwait(pSigSet, &SigInfo, NULL);
    if (piSignalNo)
        *piSignalNo = SigInfo.si_signo;
    if (rc >= 0)
        LIBCLOG_RETURN_MSG(0, "ret 0 - *piSignalNo=%d)\n", SigInfo.si_signo);
    LIBCLOG_ERROR_RETURN_MSG(-1, "ret -1 - *piSignalNo=%d)\n", SigInfo.si_signo);
}
