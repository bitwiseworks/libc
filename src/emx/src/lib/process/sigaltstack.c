/* $Id: sigaltstack.c 2254 2005-07-17 12:25:44Z bird $ */
/** @file
 *
 * LIBC - sigaltstack().
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
 * Changes and/or queries the alternative signal stack
 * settings of the current thread.
 *
 * @returns 0 on success.
 * @returns Negative error number (errno.h) on failure.
 * @param   pThrd       Thread which signal stack to change and/or query.
 * @param   pStack      New stack settings. (Optional)
 * @param   pOldStack   Old stack settings. (Optional)
 */
int _STD(sigaltstack)(const stack_t *pStack, stack_t *pOldStack)
{
    LIBCLOG_ENTER("pStack=%p {ss_flags=%#x, ss_sp=%p, ss_size=%d} pOldStack=%p\n",
                  (void *)pStack,
                  pStack ? pStack->ss_flags : 0,
                  pStack ? pStack->ss_sp    : NULL,
                  pStack ? pStack->ss_size  : 0,
                  (void *)pOldStack);

    /*
     * Perform operation and return.
     */
    stack_t OldStack;
    int rc = __libc_Back_signalStack(__libc_threadCurrent(), pStack, &OldStack);
    if (!rc)
    {
        if (pOldStack)
            *pOldStack = OldStack;
        LIBCLOG_RETURN_MSG(0, "ret 0 (*pOldStack {ss_flags=%#x, ss_sp=%p, ss_size=%d}\n",
                           OldStack.ss_flags, OldStack.ss_sp, OldStack.ss_size);
    }

    errno = -rc;
    LIBCLOG_ERROR_RETURN_INT(-1);
}

