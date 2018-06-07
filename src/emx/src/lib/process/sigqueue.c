/* $Id: sigqueue.c 2254 2005-07-17 12:25:44Z bird $ */
/** @file
 *
 * LIBC - sigqueue().
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
#include <string.h>
#include <InnoTekLIBC/backend.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_SIGNAL
#include <InnoTekLIBC/logstrict.h>



/**
 * Queue a signal.
 *
 * @returns 0 on success.
 * @returns -1 on failure, errno set.
 * @param   pid             The target process id.
 * @param   iSignalNo       Signal to queue.
 * @param   SigVal          The value to associate with the signal.
 */
int _STD(sigqueue)(pid_t pid, int iSignalNo, const union sigval SigVal)
{
    LIBCLOG_ENTER("pid=%d iSignalNo=%d SigVal=%p\n", pid, iSignalNo, SigVal.sigval_ptr);

    /*
     * Call backend worker.
     */
    int rc = __libc_Back_signalQueue(pid, iSignalNo, SigVal);
    if (!rc)
        LIBCLOG_RETURN_INT(0);
    errno = -rc;
    LIBCLOG_ERROR_RETURN_INT(-1);
}


