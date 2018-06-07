/* $Id: psignal.c 2254 2005-07-17 12:25:44Z bird $ */
/** @file
 *
 * LIBC - psignal().
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
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <emx/syscalls.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_SIGNAL
#include <InnoTekLIBC/logstrict.h>


/**
 * Writes a message to stderr on the form:
 *
 *      <message>: <signal description>
 *
 * or is pszMsg is NULL or empty:
 *
 *      <signal description>
 *
 * The signal description is taken from sys_siglist and the message is supplied
 * by the caller.
 *
 * @param   uSignalNo   Signal number.
 * @param   pszMsg      Message to display.
 */
void	_STD(psignal)(unsigned int uSignalNo, const char *pszMsg)
{
    LIBCLOG_ENTER("uSignalNo=%d pszMsg=%p {'%s'}\n", uSignalNo, pszMsg, pszMsg);

    /*
     * Determin the signal description.
     */
    const char *pszSignal;
    if (uSignalNo < NSIG)
        pszSignal = sys_siglist[uSignalNo];
    else
    {
        static char sz[64] = "Unknown signal no.";
        itoa(uSignalNo, &sz[sizeof("Unknown signal no.") - 1], 10);
        pszSignal = &sz[0];
        LIBCLOG_ERROR("Invalid signal %d\n", uSignalNo);
    }

    /*
     * Write message.
     */
    if (pszMsg && *pszMsg)
    {
        __write(STDERR_FILENO, pszMsg, strlen(pszMsg));
        __write(STDERR_FILENO, ": ", sizeof(": ") - 1);
    }
    __write(STDERR_FILENO, pszSignal, strlen(pszSignal));
    __write(STDERR_FILENO, "\n", sizeof("\n") - 1);

    LIBCLOG_RETURN_VOID();
}

