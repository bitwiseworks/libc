/* $Id: DosAllocMemEx.c 2254 2005-07-17 12:25:44Z bird $ */
/** @file
 *
 * DosAllocMemEx.
 *
 * Copyright (c) 2004 knut st. osmundsen <bird-srcspam@anduin.net>
 * Copyright (c) 2004 nickk
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
#include <386/builtin.h>
#define __LIBC_LOG_GROUP    __LIBC_LOG_GRP_DOSEX
#include <InnoTekLIBC/logstrict.h>
#include "DosEx.h"


/*******************************************************************************
*   Internal Functions                                                         *
*******************************************************************************/
static int allocAtAddress(void *pvReq, ULONG cbReq, ULONG fReq);
#ifdef DEBUG_ME
static void dumpMemFlags(void *pvIn);
#endif



/**
 * Extended DosAllocMem().
 *
 * @returns See DosAllocMem().
 * @param   ppv     Where to store the address of the allocated memory.
 *                  If OBJ_LOCATION is specified in the flFlags *ppv will be the
 *                  address the object must be allocated at.
 * @param   cb      Number of bytes to allocate.
 * @param   flFlags Allocation flags. This API supports the same flags as DosAllocMem()
 *                  but adds OBJ_FORK and OBJ_LOCATION.
 *                  If OBJ_FORK is specified the object will be automatically
 *                  duplicated in the new process.
 *                  If OBJ_LOCATION is specified the object will be allocated
 *                  at the address specified by *ppv.
 * @remark  The memory must be freed with DosFreeMemEx()!
 */
APIRET APIENTRY DosAllocMemEx(PPVOID ppv, ULONG cb, ULONG flFlags)
{
    LIBCLOG_ENTER("ppv=%p:{%p} cb=%ld flFlags=%#lx\n", (void *)ppv, *ppv, cb, flFlags);
    int     rc;
    FS_VAR();

    /*
     * Validate input first.
     */
    if (    flFlags & OBJ_LOCATION
        &&  (   (uintptr_t)*ppv < 0x10000
             || (uintptr_t)*ppv >= 0xc0000000)
        )
        LIBCLOG_ERROR_RETURN_INT(ERROR_INVALID_ADDRESS);

    /*
     * Make the allocation.
     */
    FS_SAVE_LOAD();
#ifdef DEBUG_ME
    LIBCLOG_MSG2("Before:\n");
    if (flFlags & OBJ_LOCATION)
        dumpMemFlags(*ppv);
    else if (flFlags & OBJ_ANY)
        dumpMemFlags((void *)0x20000001);
    else
        dumpMemFlags((void *)0x10000001);
#endif
    if (flFlags & OBJ_LOCATION)
        rc = allocAtAddress(*ppv, cb, flFlags & ~(OBJ_LOCATION | OBJ_FORK));
    else
        rc = DosAllocMem(ppv, cb, flFlags & ~(OBJ_FORK));
#ifdef DEBUG_ME
    LIBCLOG_MSG2("After:\n");
    dumpMemFlags(*ppv);
#endif
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
        PDOSEX  pEntry = __libc_dosexAlloc(DOSEX_TYPE_MEM_ALLOC);
        if (!pEntry)
        {
            DosFreeMem(pv);
            FS_RESTORE();
            LIBCLOG_ERROR_RETURN_INT(ERROR_NOT_ENOUGH_MEMORY);
        }
        pEntry->u.MemAlloc.pv = pv;
        pEntry->u.MemAlloc.cb = cb;
        pEntry->u.MemAlloc.flFlags = flFlags & ~(OBJ_LOCATION | OBJ_FORK);
        __atomic_add(&__libc_gcbDosExMemAlloc, (cb + 0xfff) & ~0xfff);
    }

    FS_RESTORE();
    if (!rc)
        LIBCLOG_RETURN_MSG(rc, "rc=%d *ppv=%p\n", rc, *ppv);
    LIBCLOG_ERROR_RETURN_MSG(rc, "rc=%d *ppv=%p\n", rc, *ppv);
}


/**
 * Allocate memory at a given address.
 *
 * @remark  This algorithm is an improoved version of the one Odin uses in it's Ring-3 PeLdr.
 */
int allocAtAddress(void *pvReq, ULONG cbReq, ULONG fReq)
{
    LIBCLOG_ENTER("pvReq=%p cbReq=%lu fReq=%#lx\n", pvReq, cbReq, fReq);
    PVOID           apvTmps[3000];
    ULONG           cbTmp;
    ULONG           fTmp;
    int             iTmp;
    int             rcRet = ERROR_NOT_ENOUGH_MEMORY;

    /*
     * Adjust flags and size.
     */
    if ((uintptr_t)pvReq < 0x20000000 /*512MB*/)
        fReq &= ~OBJ_ANY;
    else
        fReq |= OBJ_ANY;
    cbReq = (cbReq + 0xfff) & ~0xfff;

    /*
     * Allocation loop.
     * This algorithm is not optimal!
     */
    fTmp  = fReq & ~(PAG_COMMIT);
    cbTmp = 1*1024*1024; /* 1MB*/
    for (iTmp = 0; iTmp < sizeof(apvTmps) / sizeof(apvTmps[0]); iTmp++)
    {
        PVOID   pvNew = NULL;
        int     rc;

        /* Allocate chunk. */
        rc = DosAllocMem(&pvNew, cbTmp, fTmp);
        apvTmps[iTmp] = pvNew;
        if (rc)
            break;

        /*
         * Passed it?
         * Then retry with the requested size.
         */
        if (pvNew > pvReq)
        {
            if (cbTmp <= cbReq)
                break;
            DosFreeMem(pvNew);
            cbTmp = cbReq;
            iTmp--;
            continue;
        }

        /*
         * Does the allocated object touch into the requested one?
         */
        if ((char *)pvNew + cbTmp > (char *)pvReq)
        {
            /*
             * Yes, we've found the requested address!
             */
            apvTmps[iTmp] = NULL;
            DosFreeMem(pvNew);

            /*
             * Adjust the allocation size to fill the gap between the
             * one we just got and the requested one.
             * If no gap we'll attempt the real allocation.
             */
            cbTmp = (uintptr_t)pvReq - (uintptr_t)pvNew;
            if (cbTmp)
            {
                iTmp--;
                continue;
            }

            rc = DosAllocMem(&pvNew, cbReq, fReq);
            if (rc || (char *)pvNew > (char *)pvReq)
                break; /* we failed! */
            if (pvNew == pvReq)
            {
                rcRet = 0;
                break;
            }

            /*
             * We've got an object which start is below the one we
             * requested. This is probably caused by the requested object
             * fitting in somewhere our tmp objects didn't.
             * So, we'll have loop and retry till all such holes are filled.
             */
            apvTmps[iTmp] = pvNew;
        }
    }

    /*
     * Cleanup reserved memory and return.
     */
    while (iTmp-- > 0)
        if (apvTmps[iTmp])
            DosFreeMem(apvTmps[iTmp]);

    if (!rcRet)
        LIBCLOG_RETURN_INT(rcRet);
    LIBCLOG_ERROR_RETURN_INT(rcRet);
}

#ifdef DEBUG_ME
/**
 * Dumps the flags of a memory region.
 * @param   pv      Address in that region.
 */
static void dumpMemFlags(void *pvIn)
{
    LIBCLOG_MSG2("address  size     flags (dumpMemFlags)\n");
    void *pv    = (uintptr_t)pvIn < 0x20000000 ?  (char *)0x10000 : (char *)0x20000000;
    void *pvMax = (uintptr_t)pvIn < 0x20000000 ?  (char *)0x20000000 : (char *)0xc0000000;
    int fLastErr = 0;
    while ((uintptr_t)pv < (uintptr_t)pvMax)
    {
        ULONG flRegion = ~0;
        ULONG cbRegion = ~0;
        APIRET rc = DosQueryMem(pv, &cbRegion, &flRegion);
        if (!rc)
        {
            LIBCLOG_MSG2("%08lx %08lx %08lx\n", (ULONG)pv, cbRegion, flRegion);
            pv = (char *)pv + ((cbRegion + 0xfff) & ~0xfff);
            fLastErr = 0;
        }
        else
        {
            if (!fLastErr)
                LIBCLOG_MSG2("%08lx rc=%ld\n", (ULONG)pv, rc);
            fLastErr = 1;
            pv = (char *)(((uintptr_t)pv + 0x1fff) & ~0xfff);
        }
    }
}
#endif
