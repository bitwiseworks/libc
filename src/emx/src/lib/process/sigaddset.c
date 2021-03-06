/* $Id: sigaddset.c 2254 2005-07-17 12:25:44Z bird $ */
/** @file
 *
 * LIBC - sigaddset().
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
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_SIGNAL
#include <InnoTekLIBC/logstrict.h>


/**
 * Adds a signal to a signal set.
 *
 * @returns 0 on success.
 * @Returns -1 on failure and errno set to EINVAL.
 * @param   pSigSet     Signal set to update.
 * @param   iSignalNo   Signal Number to add to the set.
 */
int _STD(sigaddset)(sigset_t *pSigSet, int iSignalNo)
{
    LIBCLOG_ENTER("pSigSet=%p iSignalNo=%d\n", (void*)pSigSet, iSignalNo);

    /*
     * Validate input.
     */
    if (!__SIGSET_SIG_VALID(iSignalNo))
    {
        errno = EINVAL;
        LIBCLOG_ERROR_RETURN(-1, "ret -1 - invalid signal number %d\n", iSignalNo);
    }

    /*
     * Set the bit.
     */
    __SIGSET_SET(pSigSet, iSignalNo);
    LIBCLOG_RETURN_INT(0);
}
