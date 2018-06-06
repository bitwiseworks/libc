/* $Id: sigpause_bsd.c 2254 2005-07-17 12:25:44Z bird $ */
/** @file
 *
 * LIBC - sigpause() BSD variant.
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
#include <InnoTekLIBC/backend.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_SIGNAL
#include <InnoTekLIBC/logstrict.h>

#undef __sigpause_bsd
int __sigpause_bsd(int fBlockMask);



/**
 * Wait for a signal to occur applying the specified signal mask to the
 * current thread.
 *
 * This is the BSD 4.3 API. It have an incomplete signal mask.
 *
 * @returns -1 and errno set to EINTR on success.
 * @returns -1 and errno set to EINVAL, if fBlockMask is invalid,
 * @param   fBlockMask  Blocked signals during the waiting.
 */
int __sigpause_bsd(int fBlockMask)
{
    LIBCLOG_ENTER("fBlockMask=%#x\n", fBlockMask);
    sigset_t    SigSet;
    int         rc;

    /*
     * Forward to sigsuspend().
     */
    __SIGSET_EMPTY(&SigSet);
    *(int*)&SigSet.__bitmap[0] = fBlockMask;
    rc = sigsuspend(&SigSet);
    if (!rc)
        LIBCLOG_ERROR_RETURN_INT(rc);
    if (errno == EINTR)
        LIBCLOG_RETURN_INT(rc);
    LIBCLOG_ERROR_RETURN_INT(rc);
}

