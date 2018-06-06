/* $Id: b_threadSleep.c 2254 2005-07-17 12:25:44Z bird $ */
/** @file
 *
 * LIBC SYS Backend - nanosleep().
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
#define INCL_BASE
#define INCL_FSMACROS
#include <os2emx.h>

#include <unistd.h>
#include <errno.h>
#include <InnoTekLIBC/thread.h>
#include <InnoTekLIBC/backend.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_BACK_THREAD
#include <InnoTekLIBC/logstrict.h>
#include <InnoTekLIBC/FastInfoBlocks.h>
#include "b_signal.h"


/**
 * Suspend execution of the current thread for a given number of nanoseconds
 * or till a signal is received.
 *
 * @returns 0 on success.
 * @returns -EINVAL if the pReqTS is invalid.
 * @returns -EINTR if the interrupted by signal.

 * @param   ullNanoReq      Time to sleep, in nano seconds.
 * @param   pullNanoRem     Where to store remaining time (also nano seconds).

 * @remark  For relativly small sleeps this api temporarily changes the thread
 *          priority to timecritical (that is, if it's in the normal or idle priority
 *          classes) to increase precision. This means that if a signal or other
 *          asyncronous event is executed, it will be executed at wrong priority.
 *          It also means that if such code changes the priority it will be undone.
 */
int __libc_Back_threadSleep(unsigned long long ullNanoReq, unsigned long long *pullNanoRem)
{
    LIBCLOG_ENTER("ullNanoReq=%lld pullNanoRem=%p", ullNanoReq, (void *)pullNanoRem);
    ULONG           msTSStart = fibGetMsCount();
    ULONG           msSleep;
    __LIBC_PTHREAD  pThrd = __libc_threadCurrent();
    int             fInterrupted;
    FS_VAR_SAVE_LOAD();

    /*
     * If zero, yield.
     */
    if (ullNanoReq == 0)
    {
        LIBCLOG_MSG("DosSleep(0)\n");
        pThrd->ulSigLastTS = 0;
        DosSleep(msSleep = 0);
        fInterrupted = pThrd->ulSigLastTS != 0;
    }
    /*
     * Infinite wait
     */
    else if (ullNanoReq == ~0ULL)
    {
        LIBCLOG_MSG("DosSleep(~0)\n");
        pThrd->ulSigLastTS = 0;
        DosSleep(msSleep = ~0);
        fInterrupted = pThrd->ulSigLastTS != 0;
    }
    /*
     * Less than 49 days wait but not infinite..
     */
    else if (ullNanoReq < 0xfffffff0ULL * 1000000ULL)
    {
        /*
         * Calc millisecond wait.
         */
        msSleep = (ULONG)( (ullNanoReq + 999999) / 1000000 );

        /*
         * For small sleeps, precision might be important. OS/2 have a very bad
         * precision on it's DosSleep api, the accuracy can easily be off by a
         * timeslice. * It might be bad to return 32milliseconds late for small
         * sleeps, thus we do a little hack for values of two timeslices or less.
         * Timecritical threads are scheduled more precisely thank normal and idle
         * threads, so we temporarily change the priority of the thread while
         * waiting.
         */
        if (msSleep < 65)
        {
            PTIB    pTib;
            PPIB    pPib;
            ULONG   ulOldPri;

            DosGetInfoBlocks(&pTib, &pPib);
            ulOldPri = pTib->tib_ptib2->tib2_ulpri;

            if (ulOldPri < (PRTYC_TIMECRITICAL << 8))
            {
                LIBCLOG_MSG("current priority %#05lx temporarily boosting priority\n", ulOldPri);
                DosSetPriority(PRTYS_THREAD, PRTYC_TIMECRITICAL, 0, 0);
            }

            /*
             * For some reason we're frequently timing out too early the range
             * 30-32ms. We're compensating here, just to make sure we're never
             * too early without reason. (bird lives by that policy.)
             */
            if (msSleep >= 30 && msSleep < 33)
                msSleep = 33;
            LIBCLOG_MSG("DosSleep(%ld)\n", msSleep);
            pThrd->ulSigLastTS = 0;
            DosSleep(msSleep);
            fInterrupted = pThrd->ulSigLastTS != 0;

            if (ulOldPri < (PRTYC_TIMECRITICAL << 8))
            {
                int rc = DosSetPriority(PRTYS_THREAD, ulOldPri >> 8, ulOldPri & 0xff, 0);
                if (rc)
                {
                    LIBCLOG_MSG("DosSetPriority(,%lx,%lx,0) failed with rc=%d\n",
                                ulOldPri >> 8, ulOldPri & 0xff, rc);
                    //@todo add assertion.
                    DosSetPriority(PRTYS_PROCESS, PRTYC_REGULAR, 0, 0);
                }
            }
        }
        else
        {
            /*
             * Simple DosSleep().
             */
            LIBCLOG_MSG("DosSleep(%ld)\n", msSleep);
            pThrd->ulSigLastTS = 0;
            DosSleep(msSleep);
            fInterrupted = pThrd->ulSigLastTS != 0;
        }
    }
    else
    {
        /*
         * OS/2 can only sleep for 4G-1 milliseonds at the time
         * so we'll have to work our way thru it... (theoretical case just for fun)
         */
        /** @todo the interrupt case is wrong if we're interrupted after more than 49 days... */
        unsigned long long ullMilliesSleep = (ullNanoReq + 999999) / 1000000;
        do
        {
            if (ullMilliesSleep > 0xfffffff0)
                msSleep = 0xfffffff0;
            else
                msSleep = (ULONG)ullMilliesSleep;
            pThrd->ulSigLastTS = 0;
            DosSleep(msSleep);
            fInterrupted = pThrd->ulSigLastTS != 0;
            ullMilliesSleep -= msSleep;
        } while (   !fInterrupted
                 && ullMilliesSleep);
    }
    FS_RESTORE();

    /*
     * Calculate remainding time if interrupted.
     */
    if (    fInterrupted
        &&  pullNanoRem)
    {
        ULONG msSlept = fibGetMsCount() - msTSStart;
        if (msSleep <= msSlept)
            *pullNanoRem = 0;
        else
        {
            unsigned long long ullNanoRem = (msSleep - msSlept) * 10000000ULL;
            *pullNanoRem = ullNanoRem >= ullNanoReq ? ullNanoReq - 1 : ullNanoRem;
        }
    }

    if (!fInterrupted)
        LIBCLOG_RETURN_INT(0);
    LIBCLOG_ERROR_RETURN_INT(-EINTR);
}
