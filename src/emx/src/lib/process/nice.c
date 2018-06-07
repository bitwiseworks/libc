/* $Id: nice.c 2254 2005-07-17 12:25:44Z bird $ */
/** @file
 *
 * LIBC - nice().
 *
 * Copyright (c) 2005 knut st. osmundsen <bird@anduin.net>
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
#include <unistd.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <errno.h>
#include <InnoTekLIBC/backend.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_PROCESS
#include <InnoTekLIBC/logstrict.h>


/**
 * Sets the priority of a process, group of processes or all processes owned by a user.
 *
 * @returns Priority in the range PRIO_MIN to PRIO_MAX.
 * @returns -1 and errno on failure.
 * @param   iIncr       The priority incerement.
 */
int _STD(nice)(int iIncr)
{
    LIBCLOG_ENTER("iIncr=%d\n", iIncr);
    int iPrio;
    int rc = __libc_Back_processGetPriority(PRIO_PROCESS, 0, &iPrio);
    if (!rc)
    {
        if (iIncr)
        {
            iPrio += iIncr;
            if (iPrio > PRIO_MAX)
                iPrio = PRIO_MAX;
            if (iPrio < PRIO_MIN)
                iPrio = PRIO_MIN;
            rc = __libc_Back_processSetPriority(PRIO_PROCESS, 0, iPrio + iIncr);
        }
        if (!rc)
            LIBCLOG_RETURN_INT(iPrio);
    }
    errno = -rc;
    LIBCLOG_ERROR_RETURN_INT(-1);
}
