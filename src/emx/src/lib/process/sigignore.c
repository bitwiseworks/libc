/* $Id: sigignore.c 2254 2005-07-17 12:25:44Z bird $ */
/** @file
 *
 * LIBC - sigignore().
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
 * Ignores the a signal (process wide).
 *
 * @returns 0 on success.
 * @returns -1 and errno set to EINVAL, if iSignalNo is invalid,
 * @param   iSignalNo   Signal to block.
 */
int _STD(sigignore)(int iSignalNo)
{
    LIBCLOG_ENTER("iSignalNo=%d\n", iSignalNo);
    struct sigaction    SigAct;
    int                 rc;

    /*
     * Validate.
     */
    if (!__SIGSET_SIG_VALID(iSignalNo))
    {
        errno = EINVAL;
        LIBCLOG_ERROR_RETURN(-1, "ret -1 - Invalid signal no %d\n", iSignalNo);
    }

    /*
     * Forward to sigaction().
     */
    SigAct.__sigaction_u.__sa_handler = SIG_IGN;
    SigAct.sa_flags                   = 0;
    __SIGSET_EMPTY(&SigAct.sa_mask);

    rc = sigaction(iSignalNo, &SigAct, NULL);
    if (!rc)
        LIBCLOG_RETURN_INT(rc);
    LIBCLOG_ERROR_RETURN_INT(rc);
}
