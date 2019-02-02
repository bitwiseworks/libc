/* $Id: sigsetmask.c 2254 2005-07-17 12:25:44Z bird $ */
/** @file
 *
 * LIBC - sigsetmask().
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
#include <InnoTekLIBC/backend.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_SIGNAL
#include <InnoTekLIBC/logstrict.h>



/**
 * Sets the  mask of blocked signals for the current thread.
 *
 * @returns old mask on success.
 * @returns -1 and errno set on failure.
 * @param   fBlockMask  New block mask.
 * @obsolete
 */
int _STD(sigsetmask)(int fBlockMask)
{
    LIBCLOG_ENTER("fBlockMask=%#x\n", fBlockMask);
    sigset_t    SigSet;
    sigset_t    SigSetOld;

    /*
     * Forward to sigprocmask().
     */
    __SIGSET_EMPTY(&SigSet);
    SigSet.__bitmap[0] = fBlockMask;
    if (sigprocmask(SIG_SETMASK, &SigSet, &SigSetOld))
        LIBCLOG_ERROR_RETURN_INT(-1);

    LIBCLOG_RETURN_INT((int)SigSetOld.__bitmap[0]);
}
