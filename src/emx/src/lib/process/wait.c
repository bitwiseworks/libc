/* $Id: waitid.c,v 1.1 2004/11/08 09:57:37 bird Exp $ */
/** @file
 *
 * LIBC - wait().
 *
 * Copyright (c) 2004 knut st. osmundsen <bird@innotek.de>
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
#include <sys/wait.h>
#include <errno.h>
#include <InnoTekLIBC/backend.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_PROCESS
#include <InnoTekLIBC/logstrict.h>


/**
 * Waits/polls for a child process to change it's running status.
 *
 * @returns Process id of the child.
 * @returns -1 and errno on failure.
 * @param   piStatus    Where to return the status.
 */
pid_t _STD(wait)(int *piStatus)
{
    LIBCLOG_ENTER("piStatus=%p\n", (void *)piStatus);
    /*
     * Just pass it along to waitpid.
     */
    pid_t pid = wait4(-1, piStatus, 0, NULL);
    if (pid >= 0)
        LIBCLOG_RETURN_MSG(pid, "ret %d (%#x) iStatus=%#x\n", pid, pid, piStatus ? *piStatus : -1);
    LIBCLOG_ERROR_RETURN_INT(pid);
}
