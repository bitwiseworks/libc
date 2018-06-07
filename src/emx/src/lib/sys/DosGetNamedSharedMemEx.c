/* $Id: DosGetNamedSharedMemEx.c 2254 2005-07-17 12:25:44Z bird $ */
/** @file
 *
 * DosGetNamedSharedMemEx.
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
 * Extended DosGetNamedSharedMem().
 *
 * @returns See DosGetNamedSharedMem().
 * @param   pv      The address of the shared memory.
 * @param   flFlags Allocation flags. This API supports the same flags as DosGetNamedSharedMem()
 *                  but adds OBJ_FORK. If OBJ_FORK is specified the object will
 *                  be automatically be opened in a forked() process.
 * @remark  The memory must be freed with DosFreeMemEx()!
 */
APIRET APIENTRY DosGetNamedSharedMemEx(PPVOID ppv, PCSZ pszName, ULONG flFlags)
{
    LIBCLOG_ENTER("ppv=%p pszName=%p:{'%s'} flFlags=%#lx\n", (void *)ppv, pszName, pszName, flFlags);
    int     rc;
    FS_VAR();

    /*
     * Make the allocation.
     */
    FS_SAVE_LOAD();
    rc = DosGetNamedSharedMem(ppv, pszName, flFlags & ~(OBJ_FORK));
    if (rc)
    {
        FS_RESTORE();
        LIBCLOG_ERROR_RETURN_INT(rc);
    }

    /*
     * Record the allocation if OBJ_FORK is specified.
     */
    if (flFlags & OBJ_FORK)
    {
        PVOID   pv = *ppv;
        rc = DosGetSharedMem(pv, flFlags & ~(OBJ_FORK));
        DosFreeMem(pv);
        if (!rc)
        {
            PDOSEX  pEntry = __libc_dosexFind(DOSEX_TYPE_MEM_OPEN, (unsigned)pv);
            if (pEntry)
            {
                pEntry->u.MemOpen.flFlags |= flFlags & ~(OBJ_FORK);
#ifdef PER_PROCESS_OPEN_COUNTS
                pEntry->u.MemOpen.cOpens++;
#endif
                __libc_dosexRelease(pEntry);
            }
            else
            {
                __libc_dosexAlloc(DOSEX_TYPE_MEM_OPEN);
                if (!pEntry)
                {
                    DosFreeMem(pv);
                    FS_RESTORE();
                    LIBCLOG_ERROR_RETURN_INT(ERROR_NOT_ENOUGH_MEMORY);
                }
                pEntry->u.MemOpen.pv      = pv;
                pEntry->u.MemOpen.flFlags = flFlags & ~(OBJ_FORK);
#ifdef PER_PROCESS_OPEN_COUNTS
                pEntry->u.MemOpen.cOpens  = 1;
#endif
            }
        }
        else
        {
            LIBC_ASSERTM_FAILED("Shared mem '%s' is not gettable and is thus incompatible with OBJ_FORK\n", pszName);
            rc = ERROR_INVALID_FLAG_NUMBER;
        }
    }

    FS_RESTORE();
    LIBCLOG_RETURN_MSG(rc, "rc=%d\n", rc);
}

