/* $Id: sighold.c 2254 2005-07-17 12:25:44Z bird $ */
/** @file
 *
 * LIBC - sighold().
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
 * Blocks a signal for the current thread.
 *
 * @returns 0 on success.
 * @returns -1 and errno set to EINVAL, if iSignalNo is invalid,
 * @param   iSignalNo   Signal to block.
 */
int _STD(sighold)(int iSignalNo)
{
    LIBCLOG_ENTER("iSignalNo=%d\n", iSignalNo);
    sigset_t    SigSet;
    int         rc;

    /*
     * Validate.
     */
    if (!__SIGSET_SIG_VALID(iSignalNo))
    {
        errno = EINVAL;
        LIBCLOG_ERROR_RETURN(-1, "ret -1 - Invalid signal no %d\n", iSignalNo);
    }

    /*
     * Forward to sigprocmask().
     */
    __SIGSET_EMPTY(&SigSet);
    __SIGSET_SET(&SigSet, iSignalNo);
    rc = sigprocmask(SIG_BLOCK, &SigSet, NULL);

    if (!rc)
        LIBCLOG_RETURN_INT(rc);
    LIBCLOG_ERROR_RETURN_INT(rc);
}
