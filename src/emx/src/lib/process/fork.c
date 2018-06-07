/* $Id: $ */
/** @file
 *
 * LIBC fork().
 *
 * Copyright (c) 2004-2005 knut st. osmundsen <bird@anduin.net>
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
#include <process.h>
#include <unistd.h>
#include <errno.h>
#include <InnoTekLIBC/backend.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_FORK
#include <InnoTekLIBC/logstrict.h>


/**
 * Fork of a child process.
 *
 * @returns 0 in the child process.
 * @returns process identifier of the new child in the parent process. (positive, non-zero)
 * @returns -1 and errno on failure.
 */
pid_t _STD(fork)(void)
{
    LIBCLOG_ENTER("\n");
    pid_t pid = __libc_Back_processFork();
    if (pid >= 0)
        LIBCLOG_RETURN_INT(pid);
    errno = -pid;
    LIBCLOG_ERROR_RETURN_INT(-1);
}

