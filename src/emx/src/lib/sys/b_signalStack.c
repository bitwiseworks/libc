/* $Id: b_signalStack.c 2254 2005-07-17 12:25:44Z bird $ */
/** @file
 *
 * LIBC SYS Backend - sigaltstack.
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
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_BACK_SIGNAL
#include <InnoTekLIBC/logstrict.h>
#include "b_signal.h"

/**
 * Changes and/or queries the alternative signal stack settings of a thread.
 *
 * @returns 0 on success.
 * @returns Negative error number (errno.h) on failure.
 * @param   pThrd       Thread which signal stack to change and/or query.
 * @param   pStack      New stack settings. (Optional)
 * @param   pOldStack   Old stack settings. (Optional)
 */
int __libc_Back_signalStack(__LIBC_PTHREAD pThrd, const stack_t *pStack, stack_t *pOldStack)
{
    LIBCLOG_ENTER("pStack=%p {ss_flags=%#x, ss_sp=%p, ss_size=%d} pOldStack=%p\n",
                  (void *)pStack,
                  pStack ? pStack->ss_flags : 0,
                  pStack ? pStack->ss_sp    : NULL,
                  pStack ? pStack->ss_size  : 0,
                  (void *)pOldStack);

    /*
     * Validate input.
     */
    if (pStack)
    {
        if (pStack->ss_flags & ~SS_DISABLE)
            LIBCLOG_ERROR_RETURN(-EINVAL, "ret -EINVAL - ss_flags=%#x is invalid\n", pStack->ss_flags);
        if (pStack->ss_flags != SS_DISABLE)
        {
            if (!pStack->ss_sp)
                LIBCLOG_ERROR_RETURN(-EINVAL, "ret -EINVAL - ss_sp=%p is NULL when SS_DISABLE is not set\n", pStack->ss_sp);
            if (pStack->ss_size < MINSIGSTKSZ)
                LIBCLOG_ERROR_RETURN(-EINVAL, "ret -EINVAL - ss_size=%d is too small. minimum size is %d\n", pStack->ss_size, MINSIGSTKSZ);

            /* Touch it. */
            *(volatile long *)pStack->ss_sp = *(volatile long *)pStack->ss_sp;
            ((volatile char *)pStack->ss_sp)[pStack->ss_size - 1] = ((volatile char *)pStack->ss_sp)[pStack->ss_size - 1];
        }
    }

    /*
     * Copy input.
     */
    stack_t         OldStack = {0};
    stack_t         Stack = {0};
    if (pStack)
        Stack = *pStack;

    /*
     * Gain exclusive access to the signal stuff.
     */
    int rc = __libc_back_signalSemRequest();
    if (rc)
        LIBCLOG_ERROR_RETURN_INT(rc);

    /*
     * Get the old stack.
     */
    OldStack.ss_flags = pThrd->pvSigStack ? 0 : SS_DISABLE;
    if (pThrd->fSigStackActive)
        OldStack.ss_flags |= SS_ONSTACK;
    OldStack.ss_sp    = pThrd->pvSigStack;
    OldStack.ss_size  = pThrd->cbSigStack;

    /*
     * Change the stack.
     */
    if (pStack)
    {
        if (!pThrd->fSigStackActive)
        {
            if (Stack.ss_flags != SS_DISABLE)
            {
                pThrd->pvSigStack = Stack.ss_sp;
                pThrd->cbSigStack = Stack.ss_size;
            }
            else
            {
                pThrd->pvSigStack = NULL;
                pThrd->cbSigStack = 0;
            }
        }
        else
            rc = -EPERM;
    }

    /*
     * Release semaphore and return.
     */
    __libc_back_signalSemRelease();
    if (!rc)
    {
        if (pOldStack)
            *pOldStack = OldStack;
        LIBCLOG_RETURN_MSG(0, "ret 0 (*pOldStack {ss_flags=%#x, ss_sp=%p, ss_size=%d}\n",
                           OldStack.ss_flags, OldStack.ss_sp, OldStack.ss_size);
    }
    LIBCLOG_ERROR_RETURN(rc, "ret %d - Stack is in use\n", rc);
}

