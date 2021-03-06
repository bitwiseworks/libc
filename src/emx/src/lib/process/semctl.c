/* $Id: semctl.c 2254 2005-07-17 12:25:44Z bird $ */
/** @file
 *
 * LIBC - semctl.
 *
 * Copyright (c) 2005 knut st. osmundsen <bird@anduin.net>
 *
 *
 * This file is part of kBuild.
 *
 * kBuild is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * kBuild is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with kBuild; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */


/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#include "libc-alias.h"
#include <sys/types.h>
#include <sys/sem.h>
#include <stdarg.h>
#include <errno.h>
#include <InnoTekLIBC/backend.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_PROCESS
#include <InnoTekLIBC/logstrict.h>


/**
 * semctl.
 */
int _STD(semctl)(int semid, int semnum, int cmd, ...)
{
    LIBCLOG_ENTER("semid=%d semnum=%d cmd=%d\n", semid, semnum, cmd);

    /*
     * Get optional argument.
     */
    union semun arg = {0};
    va_list args;
    va_start(args, cmd);
    switch (cmd)
    {
        case IPC_SET:
        case IPC_STAT:
        case GETALL:
        case SETVAL:
        case SETALL:
            arg = va_arg(args, union semun);
            break;
    }
    va_end(args);

    /* do syscall */
    int rc = __libc_Back_sysvSemCtl(semid, semnum, cmd, arg);
    if (rc >= 0)
        LIBCLOG_RETURN_INT(rc);
    errno = -rc;
    LIBCLOG_ERROR_RETURN_INT(-1);
}

