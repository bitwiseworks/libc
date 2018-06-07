/* $Id: DosCloseEventSemEx.c 2254 2005-07-17 12:25:44Z bird $ */
/** @file
 *
 * DosCloseEventSemEx.
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
#define INCL_ERRORS
#define INCL_DOSSEMAPHORES
#define INCL_FSMACROS
#define INCL_EXAPIS
#include <os2emx.h>
#define __LIBC_LOG_GROUP    __LIBC_LOG_GRP_DOSEX
#include <InnoTekLIBC/logstrict.h>
#include "DosEx.h"


/**
 * Close semaphore opened or created using the extended APIs.
 * @returns see DosCloseEventSem().
 * @param   hev     Handle to the event semaphore which is to be closed.
 */
APIRET APIENTRY DosCloseEventSemEx(HEV hev)
{
    LIBCLOG_ENTER("hev=%lx\n", hev);
    DOSEXTYPE   enmType;
    int         rc;
    FS_VAR();

    /*
     * Validate input.
     */
    if (!hev)
        LIBCLOG_ERROR_RETURN_INT(ERROR_INVALID_HANDLE);

    /*
     * Both shared and private handles comming this way, so
     * we by some testing it looks like shared handles have the upper
     * bit set. So, we'll use that bit to check which type to try first.
     *
     * If neither of the mutex types works, we'll let DosCloseMutexSem()
     * have a go.
     */
    FS_SAVE_LOAD();
    enmType = hev & 0x80000000 ? DOSEX_TYPE_EVENT_OPEN : DOSEX_TYPE_EVENT_CREATE;
    rc = __libc_dosexFree(enmType, hev);
    if (rc == -1)
    {
        enmType = enmType == DOSEX_TYPE_EVENT_CREATE ? DOSEX_TYPE_EVENT_OPEN : DOSEX_TYPE_EVENT_CREATE;
        rc = __libc_dosexFree(enmType, hev);
        if (rc == -1)
            rc = DosCloseEventSem(hev);
    }

    FS_RESTORE();
    if (!rc)
        LIBCLOG_RETURN_INT(rc);
    LIBCLOG_ERROR_RETURN_INT(rc);
}

