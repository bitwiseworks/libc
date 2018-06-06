/* $Id: b_mmanProtect.c 2254 2005-07-17 12:25:44Z bird $ */
/** @file
 *
 * LIBC Backend - mprotect().
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
#define INCL_FSMACROS
#define INCL_BASE
#include <os2emx.h>
#include "b_mman.h"
#include <string.h>
#include <errno.h>
#include <sys/mman.h>
#include <InnoTekLIBC/backend.h>
#define __LIBC_LOG_GROUP    __LIBC_LOG_GRP_BACK_MMAN
#include <InnoTekLIBC/logstrict.h>



/**
 * Change the memory protection attributes of a range of pages.
 * This function supports the crossing of object boundaries and works
 * on any memory the native apis works on.
 *
 * @returns Negative error code (errno.h) on failure.
 * @param   pv      Pointer to first page - page aligned!
 * @param   cb      Size of the ranage - page aligned!
 * @param   fFlags  The PROT_* flags to replace the current flags with.
 */
int __libc_Back_mmanProtect(void *pv, size_t cb, unsigned fFlags)
{
    LIBCLOG_ENTER("pv=%p cb=%#x fFlags=%#x\n", pv, cb, fFlags);
    FS_VAR();

    /*
     * Convert flags.
     */
    ULONG fOS2Flags;
    if (fFlags == PROT_NONE)
        fOS2Flags = PAG_DECOMMIT;
    else if (   (fFlags & (PROT_EXEC | PROT_READ | PROT_WRITE))
             &&  !(fFlags & ~(PROT_EXEC | PROT_READ | PROT_WRITE)))
    {
        if (    PAG_READ    == PROT_READ
            &&  PAG_WRITE   == PROT_WRITE
            &&  PAG_EXECUTE == PROT_EXEC)
            fOS2Flags = fFlags | PAG_COMMIT;
        else
        {
            fOS2Flags = PAG_COMMIT;
            if (fFlags & PROT_EXEC)
                fOS2Flags |= PAG_EXECUTE;
            if (fFlags & PROT_WRITE)
                fOS2Flags |= PAG_WRITE;
            if (fFlags & PROT_READ)
                fOS2Flags |= PAG_READ;
        }
    }
    else
        LIBCLOG_ERROR_RETURN(-EINVAL, "ret -EINVAL - Invalid flags fFlags=%#x\n", fFlags);

    /*
     * Check if special memory region.
     */

    /*
     * Try apply changes in one go.
     */
    FS_SAVE_LOAD();
    int rc = DosSetMem(pv, cb, fOS2Flags);
    if (rc && cb > PAGE_SIZE)
    {
        /*
         * Have to query first to ensure correct result.
         */
        ULONG cbLeft  = cb;
        PVOID pvChunk = pv;
        while (cbLeft > 0)
        {
            /*
             * Query flags.
             */
            ULONG flChunk = PAG_COMMIT | PAG_DECOMMIT | PAG_READ | PAG_WRITE | PAG_EXECUTE | PAG_BASE | PAG_FREE;
            ULONG cbChunk = cbLeft;
            rc = DosQueryMem(pvChunk, &cbChunk, &flChunk);
            if (rc) /* bug in some warp4 fixpack IIRC */
                rc = DosQueryMem(pvChunk, &cbChunk, &flChunk);
            if (rc)
            {
                FS_RESTORE();
                rc = -__libc_native2errno(rc);
                LIBCLOG_ERROR_RETURN_MSG(rc, "ret %d (pvChunk=%p)\n", rc, pvChunk);
            }
            cbChunk = (cbChunk + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
            if (flChunk & PAG_FREE)
            {
                FS_RESTORE();
                LIBCLOG_ERROR_RETURN_MSG(rc, "ret %d (pvChunk=%p PAG_FREE)\n", rc, pvChunk);
            }

            /* advance */
            cbLeft -= cbChunk;
            pvChunk = (char *)pvChunk + cbChunk;
        }

        /*
         * If we get so far, we'll redo it and apply the changes.
         */
        cbLeft  = cb;
        pvChunk = pv;
        while (cbLeft > 0)
        {
            /*
             * Query flags.
             */
            ULONG flChunk = PAG_COMMIT | PAG_DECOMMIT | PAG_READ | PAG_WRITE | PAG_EXECUTE | PAG_BASE | PAG_FREE;
            ULONG cbChunk = cbLeft;
            rc = DosQueryMem(pvChunk, &cbChunk, &flChunk);
            if (rc) /* bug in some warp4 fixpack IIRC */
                rc = DosQueryMem(pvChunk, &cbChunk, &flChunk);
            if (rc)
            {
                FS_RESTORE();
                rc = -__libc_native2errno(rc);
                LIBCLOG_ERROR_RETURN_MSG(rc, "ret %d (pvChunk=%p)\n", rc, pvChunk);
            }
            cbChunk = (cbChunk + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);

            /*
             * Apply changes.
             */
            if (fFlags == PROT_NONE)
            {
                if (flChunk & PAG_COMMIT)
                    rc = DosSetMem(pvChunk, cbChunk, PAG_DECOMMIT);
            }
            else
            {
                if (fOS2Flags != (flChunk & (PAG_COMMIT | PAG_READ | PAG_WRITE | PAG_EXECUTE)))
                    rc = DosSetMem(pvChunk, cbChunk, (flChunk & PAG_COMMIT) ^ fOS2Flags);
            }
            if (rc)
            {
                FS_RESTORE();
                rc = -__libc_native2errno(rc);
                LIBCLOG_ERROR_RETURN_MSG(rc, "ret %d (pvChunk=%p)\n", rc, pvChunk);
            }

            /* advance */
            cbLeft -= cbChunk;
            pvChunk = (char *)pvChunk + cbChunk;
        }
    }

    FS_RESTORE();
    LIBCLOG_RETURN_INT(0);
}

