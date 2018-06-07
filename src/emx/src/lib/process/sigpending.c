/* $Id: sigpending.c 2254 2005-07-17 12:25:44Z bird $ */
/** @file
 *
 * LIBC - sigpending().
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

/**
 * Gets the set of signals which are blocked by the current thread and are
 * pending on the process or the calling thread.
 *
 * @returns 0 indicating success.
 * @returns -1 on failure. (failed to get semaphore)
 * @param   pSigSet     Pointer to signal set where the result is to be stored.
 */
int _STD(sigpending)(sigset_t *pSigSet)
{
    LIBCLOG_ENTER("pSigSet=%p\n", (void *)pSigSet);

    int rc = __libc_Back_signalPending(pSigSet);
    if (!rc)
        LIBCLOG_RETURN_MSG(0, "ret 0 (*pSigSet={%08lx %08lx})\n", pSigSet->__bitmap[1], pSigSet->__bitmap[0]);
    errno = -rc;
    LIBCLOG_ERROR_RETURN_INT(-1);
}

