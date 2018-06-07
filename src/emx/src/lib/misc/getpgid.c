/* $Id: getpgid.c 2254 2005-07-17 12:25:44Z bird $ */
/** @file
 *
 * LIBC - getpgid().
 *
 * Copyright (c) 2005 knut st. osmundsen <bird@anduin.net>
 *
 *
 * This file is part of InnoTek LIBC.
 *
 * InnoTek LIBC is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * InnoTek LIBC is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with InnoTek LIBC; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */



/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#include "libc-alias.h"
#include <unistd.h>
#include <errno.h>
#include <InnoTekLIBC/backend.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_PROCESS
#include <InnoTekLIBC/logstrict.h>


/**
 * Gets the process group id of the current process.
 *
 * @returns the process group id on success.
 * @returns -1 and errno on failure.
 * @param   pid     The id of the process which process group id we're quering.
 *                  The value 0 means the current process.
 */
pid_t _STD(getpgid)(pid_t pid)
{
    LIBCLOG_ENTER("pid=%d\n", pid);
    pid_t pgid = __libc_Back_processGetPGrp(pid);
    if (pgid >= 0)
        LIBCLOG_RETURN_INT(pgid);
    errno = -pgid;
    LIBCLOG_ERROR_RETURN_INT(-1);
}

