/* $Id: DosAllocSharedMemEx.c 2254 2005-07-17 12:25:44Z bird $ */
/** @file
 *
 * DosAllocSharedMemEx.
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
 * Extended DosAllocSharedMem().
 *
 * @returns See DosAllocSharedMem().
 * @param   ppv     Where to store the address of the allocated memory.
 * @param   pszName Name of the shared memory. (optional)
 * @param   cb      Number of bytes to allocate.
 * @param   flFlags Allocation flags. This API supports the same flags as DosAllocSharedMem()
 *                  but adds OBJ_FORK.
 *                  If OBJ_FORK is specified the object will be automatically
 *                  be opened in a forked() process.
 * @remark  The memory must be freed with DosFreeMemEx()!
 */
APIRET APIENTRY DosAllocSharedMemEx(PPVOID ppv, PCSZ pszName, ULONG cb, ULONG flFlags)
{
    LIBCLOG_ENTER("ppv=%p pszName=%p:{'%s'} cb=%ld flFlags=%#lx\n", (void *)ppv, pszName, pszName, cb, flFlags);
    int     rc;
    FS_VAR();

    /*
     * Make the allocation.
     */
    FS_SAVE_LOAD();
    rc = DosAllocSharedMem(ppv, pszName, cb, (flFlags & ~(OBJ_FORK)) | (flFlags & OBJ_FORK ? OBJ_GETTABLE : 0));
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
        PVOID   pv = *ppv;              /* this is a good time to crash - paranoia!!! */
        PDOSEX  pEntry = __libc_dosexAlloc(DOSEX_TYPE_MEM_OPEN);
        if (!pEntry)
        {
            DosFreeMem(pv);
            FS_RESTORE();
            LIBCLOG_ERROR_RETURN_INT(ERROR_NOT_ENOUGH_MEMORY);
        }
        pEntry->u.MemOpen.pv      = pv;
        pEntry->u.MemOpen.flFlags = flFlags & (PAG_READ | PAG_WRITE | PAG_EXECUTE | PAG_GUARD);
#ifdef PER_PROCESS_OPEN_COUNTS
        pEntry->u.MemOpen.cOpens  = 1;
#endif
    }

    FS_RESTORE();
    if (!rc)
        LIBCLOG_RETURN_MSG(rc, "rc=%d *ppv=%p\n", rc, *ppv);
    LIBCLOG_ERROR_RETURN_MSG(rc, "rc=%d *ppv=%p\n", rc, *ppv);
}

