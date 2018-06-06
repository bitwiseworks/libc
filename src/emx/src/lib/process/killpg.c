/* $Id: killpg.c 2254 2005-07-17 12:25:44Z bird $ */
/** @file
 *
 * LIBC - killpg().
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
 * Send a signal to a process group.
 *
 * Same as kill(-pgrp, iSignalNo);
 *
 * @returns 0 on success.
 * @returns -1 with errno set to EINVAL if iSignalNo is not a valid signal.
 * @returns -1 with errno set to EINVAL if pgrp is less or equal to 1.
 *
 * @param   pgrp        Process group id to send signals to. This value must be
 *                      greater or equal to 0.
 *                      Special value 0 means current process group.
 *                      Special value 1 means all processes.
 * @param   iSignalNo   Signal to send.
 *                      If 0 Then only do error checking.
 */
int _STD(killpg)(pid_t pgrp, int iSignalNo)
{
    LIBCLOG_ENTER("pgrp=%d iSignalNo=%d\n", pgrp, iSignalNo);
    int rc;

    /*
     * Validate input.
     */
    if (!__SIGSET_SIG_VALID(iSignalNo) && iSignalNo != 0)
    {
        errno = EINVAL;
        LIBCLOG_ERROR_RETURN(-1, "ret -1 - Invalid signal number %d\n", iSignalNo);
    }
    if (pgrp < 0)
    {
        errno = EINVAL;
        LIBCLOG_ERROR_RETURN(-1, "ret -1 - Invalid process group id %d\n", pgrp);
    }

    /*
     * forward to kill
     */
    rc = kill(-pgrp, iSignalNo);
    if (!rc)
        LIBCLOG_RETURN_INT(rc);
    LIBCLOG_ERROR_RETURN_INT(rc);
}
