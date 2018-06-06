/* $Id: DosFreeMemEx.c 2254 2005-07-17 12:25:44Z bird $ */
/** @file
 *
 * DosFreeMemEx.
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
#define INCL_DOSMEMMGR
#define INCL_FSMACROS
#define INCL_EXAPIS
#include <os2emx.h>
#define __LIBC_LOG_GROUP    __LIBC_LOG_GRP_DOSEX
#include <InnoTekLIBC/logstrict.h>
#include "DosEx.h"


/**
 * Free memory allocated by DosAllocMemEx.
 *
 * @returns See DosFreeMem().
 * @param   pv  Address of the memory to free.
 */
APIRET APIENTRY DosFreeMemEx(PVOID pv)
{
    LIBCLOG_ENTER("pv=%p\n", pv);
    DOSEXTYPE   enmType;
    int         rc;
    FS_VAR();

    /*
     * Validate input.
     */
    if (!pv)
        LIBCLOG_ERROR_RETURN_INT(ERROR_INVALID_ADDRESS);

    /*
     * We'll make guesses at which type to try first based on the address.
     * It's difficult to tell exactly but we say that 3/4 of the address space
     * below 512MB is shared. As for the high memory it's harder to say,
     * especially since we don't know the limit of that memory, so there
     * we'll just say everything about 900MB is more likely to be shared
     * than private.
     *
     * If we don't find the address when both types have been tried
     * we'll give it to DosFreeMem().
     */
    FS_SAVE_LOAD();
    enmType = (uintptr_t)pv < 128*1024*1024 || ((uintptr_t)pv >= 512*1024*1024 && (uintptr_t)pv < 900*1024*1024)
        ? DOSEX_TYPE_MEM_ALLOC : DOSEX_TYPE_MEM_OPEN;
    rc = __libc_dosexFree(enmType, (unsigned)pv);
    if (rc == -1)
    {
        enmType = enmType == DOSEX_TYPE_MEM_OPEN ? DOSEX_TYPE_MEM_ALLOC : DOSEX_TYPE_MEM_OPEN;
        rc = __libc_dosexFree(enmType, (unsigned)pv);
        if (rc == -1)
            rc = DosFreeMem(pv);
    }

    FS_RESTORE();
    if (!rc)
        LIBCLOG_RETURN_INT(rc);
    LIBCLOG_ERROR_RETURN_INT(rc);
}

