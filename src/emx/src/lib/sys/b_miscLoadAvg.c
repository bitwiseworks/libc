/* $Id: b_miscLoadAvg.c 2254 2005-07-17 12:25:44Z bird $ */
/** @file
 *
 * LIBC SYS Backend - System Load Averages.
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
#include <stdlib.h>
#include <errno.h>
#include <math.h>
#include <emx/umalloc.h>
#include <InnoTekLIBC/backend.h>
#include <InnoTekLIBC/sharedpm.h>
#define __LIBC_LOG_GROUP    __LIBC_LOG_GRP_MISC
#include <InnoTekLIBC/logstrict.h>

#define INCL_DOSPROFILE
#define INCL_DOSERRORS
#define INCL_FSMACROS
#include <os2emx.h>


/*
 * Stuff used when calculating the average.
 */
#define FSHIFT  11
#define FSCALE  (1 << FSHIFT)


/*******************************************************************************
*   Internal Functions                                                         *
*******************************************************************************/

/**
 * Update the load averages.
 */
static int miscUpdateLoadAvg(__LIBC_PSPMLOADAVG pLoadAvg, unsigned uTimestamp)
{
    LIBCLOG_ENTER("pLoadAvg=%p uTimestamp=%08x\n", (void *)pLoadAvg, uTimestamp);
    int         rc = -1;
    void       *pv;
    size_t      cb;

    /*
     * Allocate temporary buffer.
     */
    cb = 256 * 1024;
    pv = _hmalloc(cb);
    if (pv)
    {
        /*
         * Query the processes and threads in the system.
         */
        FS_VAR();
        FS_SAVE_LOAD();
        unsigned    cTries = 3;
        for (;;)
        {
            rc = DosQuerySysState(QS_PROCESS, 0, 0, 0, pv, cb);
            if (!rc || cTries-- > 0)
                break;
            cb *= 2;
            void *pvNew = realloc(pv, cb);
            if (!pvNew)
                rc = ERROR_NOT_ENOUGH_MEMORY;
        }
        if (!rc)
        {
            /*
             * Count ready threads.
             */
            unsigned    cThreads = 0;
            QSPTRREC   *pPtrRec = (QSPTRREC *)pv;
            QSPREC     *pProcRec = pPtrRec->pProcRec;
            while (pProcRec && pProcRec->RecType == QS_PROCESS)
            {
                unsigned iThrd = pProcRec->cTCB;
                while (iThrd-- > 0)
                {
                    unsigned uState = pProcRec->pThrdRec[iThrd].state;
                    if (    uState == 1 /* ready */
                        ||  uState == 5 /* running */
                        ||  uState == 6 /* ready, boosted */)
                        cThreads++;
                }

                /* next - DEPENDS on the ThrdRecs to be last! */
                pProcRec = (QSPREC *)(void *)(pProcRec->pThrdRec + pProcRec->cTCB);
            }

            /*
             * Calc the averages.
             */
            static   unsigned uaDeltas[3] = {60*1000, 5*60*1000, 15*60*1000}; /* millies */
            unsigned uDelta = pLoadAvg->uTimestamp ? uTimestamp - pLoadAvg->uTimestamp : ~0;
            unsigned i;
            for (i = 0; i < 3; i++)
            {
                if (uDelta < uaDeltas[i])
                {
                    /*
                     * Use a modified BSD algorithm for calculating the averages.
                     */
                    /** @todo Check out the maths on this!!! */
                    fixpt_t     frdExp = exp(-(double)uDelta / (double)uaDeltas[i]) * FSCALE;
                    uint32_t    u32 = pLoadAvg->u32Samples[i];
                    pLoadAvg->u32Samples[i] = (u32 * frdExp + cThreads * FSCALE * (FSCALE - frdExp)) >> FSHIFT;
                }
                else
                    pLoadAvg->u32Samples[i] = cThreads << FSHIFT;
            }

            /*
             * Update.
             * (SPM assigns current timestamp.)
             */
            __libc_spmSetLoadAvg(pLoadAvg);
        }
        else
            LIBCLOG_ERROR("Failed to query sys state, rc=%d. cb=%d\n", rc, cb);

        free(pv);
        FS_RESTORE();
    }

    if (!rc)
        LIBCLOG_RETURN_INT(rc);
    LIBCLOG_ERROR_RETURN_INT(rc);
}


/**
 * Gets the system load averages.
 * The load is the average values of ready and running threads(/processes)
 * over the last 1, 5 and 15 minuttes.
 *
 * @returns Number of samples returned on success.
 * @returns -1 and errno on failure.
 * @param   pardAvgs    Where to store the samples.
 * @param   cAvgs       Number of samples to get. Max is 3.
 * @remark  See OS/2 limitations in getloadavg().
 */
int __libc_Back_miscLoadAvg(double *pardAvgs, unsigned cAvgs)
{
    LIBCLOG_ENTER("pardAvgs=%p cAvgs=%d\n", (void *)pardAvgs, cAvgs);
    __LIBC_SPMLOADAVG   LoadAvg;
    unsigned            uTimestamp;
    int                 rc;
    unsigned            i;

    /*
     * Validate input.
     */
    if (!pardAvgs)
        LIBCLOG_ERROR_RETURN(-EFAULT, "ret -EINVAL - Invalid address!\n");

    /*
     * Get the current values and check if an update is required.
     */
    rc = __libc_spmGetLoadAvg(&LoadAvg, &uTimestamp);
    if (rc)
        LIBCLOG_ERROR_RETURN_INT(rc);
    if (uTimestamp - LoadAvg.uTimestamp > 5000 /* 5sec */)
    {
        rc = miscUpdateLoadAvg(&LoadAvg, uTimestamp);
        if (rc)
            LIBCLOG_ERROR_RETURN_INT(rc);
    }

    /*
     * Calculate the double values.
     */
    i = cAvgs <= 3 ? cAvgs : 3;
    while (i-- > 0)
        pardAvgs[i] = (double)LoadAvg.u32Samples[i] / FSCALE;

    LIBCLOG_RETURN_MSG(0, "ret 0 LoadAvg:{%08x, %08x, %08x}\n",
                       LoadAvg.u32Samples[0], LoadAvg.u32Samples[1], LoadAvg.u32Samples[2]);
}

