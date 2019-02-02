/* $Id: b_signalTimer.c 2254 2005-07-17 12:25:44Z bird $ */
/** @file
 *
 * LIBC SYS Backend - setitimer() & getitimer().
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
#define INCL_BASE
#define INCL_FSMACROS
#define INCL_DOSINFOSEG
#define INCL_ERRORS
#include <os2emx.h>
#include <signal.h>
#include <errno.h>
#include <386/builtin.h>
#include <InnoTekLIBC/backend.h>
#include <InnoTekLIBC/thread.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_BACK_SIGNAL
#include <InnoTekLIBC/logstrict.h>
#include "b_signal.h"
#include "backend.h"


/*******************************************************************************
*   Global Variables                                                           *
*******************************************************************************/
/** The Thread Id of the worker thread. */
static volatile unsigned        gtidWorker;
/** Set if the timer is armed. */
static volatile unsigned        gfArmed;
/** Set if the timer needs rearming using guInterval. */
static volatile unsigned        gfOneShot;
/** The repeate interval (millies). */
static volatile unsigned        guInterval;
/** When we think the next timer shot will be. */
static volatile unsigned        guNext;
/** The event semaphore handle. */
static volatile HEV             ghevTimer;
/** Indicate that it's time to terminate. */
static volatile unsigned        gfTerminate;
/** Timer handle. */
static volatile HTIMER          ghTimer = ~0U;


/*******************************************************************************
*   Internal Functions                                                         *
*******************************************************************************/
static void signalTimerWorkerThread(void *pvArg);
static unsigned signalTimerClock(void);

/**
 * Internal worker thread.
 * @param   pvArg   Ignored.
 */
static void signalTimerWorkerThread(void *pvArg)
{
    LIBCLOG_ENTER("pvArg=%p\n", pvArg);
    pvArg = pvArg;

    /*
     * Gather stuff we need.
     */
    __LIBC_PTHREAD pThrd = __libc_threadCurrent();
    LIBC_ASSERT(pThrd && pThrd->fInternalThread);
    GINFOSEG volatile *pGIS = GETGINFOSEG();
    LIBC_ASSERT(pGIS && pGIS->uchMajorVersion);
    PPIB pPib = NULL;
    PTIB pTib = NULL;
    int rc = DosGetInfoBlocks(&pTib, &pPib);
    LIBC_ASSERTM(rc == 0 && pPib && pPib->pib_ulpid && pTib && pTib->tib_ptib2->tib2_ultid, "DosGetInfoBlocks -> %d\n", rc);

    /*
     * Change the priority to TC to make the timer more accurate.
     */
    rc = DosSetPriority(PRTYS_THREAD, PRTYC_TIMECRITICAL, 0, 0);
    LIBC_ASSERTM(rc == 0, "DosSetPriority(PRTYS_THREAD, PRTYC_TIMECRITICAL, 0, 0) -> %d\n", rc);

    /*
     * Timer loop.
     */
    siginfo_t SigInfo = {0};
    SigInfo.si_signo = SIGALRM;
    SigInfo.si_code  = SI_TIMER;
    for (;!gfTerminate;)
    {
        /*
         * Wait for the alarm to fire.
         */
        rc = DosWaitEventSem(ghevTimer, SEM_INDEFINITE_WAIT);
        if (rc || gfTerminate)
        {
            LIBCLOG_MSG("Timer worker stopping: DosWaitEventSem -> %d gfTerminate=%d\n", rc, gfTerminate);
            break;
        }
        if (pPib->pib_flstatus & (0x40/*dying*/ | 0x04/*exiting all*/ | 0x02/*Exiting Thread 1*/ | 0x01/*ExitList */))
        {
            LIBCLOG_MSG("Timer worker stopping: pPib->pib_flstatus=%#02lx\n", pPib->pib_flstatus);
            break;
        }

        /*
         * Calc next.
         */
        unsigned uTime = pGIS->msecs;

        /*
         * Take the semaphore.
         * Recheck for termination afterwards.
         */
        int rc = __libc_back_signalSemRequest();
        if (rc)
        {
            LIBCLOG_MSG("Timer worker stopping: __libc_back_signalSemRequest() -> %d\n", rc);
            break;
        }
        if (gfTerminate)
        {
            __libc_back_signalSemRelease();
            LIBCLOG_MSG("Timer worker stopping: gfTerminate=%d\n", gfTerminate);
            break;
        }
        if (pPib->pib_flstatus & (0x40/*dying*/ | 0x04/*exiting all*/ | 0x02/*Exiting Thread 1*/ | 0x01/*ExitList */))
        {
            __libc_back_signalSemRelease();
            LIBCLOG_MSG("Timer worker stopping: pPib->pib_flstatus=%#02lx\n", pPib->pib_flstatus);
            break;
        }


        /*
         * Update guNext and if needed rearm the timer.
         */
        if (gfArmed)
        {
            if (!gfOneShot)
                __atomic_xchg(&guNext, uTime + guInterval);
            else
            {
                __atomic_xchg(&gfOneShot, 0);
                __atomic_xchg((volatile unsigned *)(void *)&ghTimer, ~0U);
                if (guInterval)
                {
                    /*
                     * Start a new timer.
                     * Note that we've not compensated for any delays, thus the 2nd SIGALRM may
                     * be a little bit later than the others. Should matter that much I hope...
                     */
                    __atomic_xchg(&guNext, pGIS->msecs + guInterval + 1);
                    rc = DosStartTimer(guInterval, (HSEM)ghevTimer, (PHTIMER)&ghTimer);
                    if (rc)
                    {
                        LIBC_ASSERTM_FAILED("DosStartTimer(%d,,) -> rc=%d\n", guInterval, rc);
                        __atomic_xchg(&gfArmed, 0);
                    }
                }
                /*
                 * It was a one shot setup, we're done.
                 */
                else
                    __atomic_xchg(&gfArmed, 0);
            }

            /*
             * Raise SIGALRM.
             */
            rc = __libc_back_signalRaiseInternal(pThrd, SIGALRM, &SigInfo, NULL, 0);
            if (rc < 0)
                LIBCLOG_MSG("__libc_back_signalRaiseInternal -> %d\n", rc);
        }

        /*
         * Done with the semaphore.
         */
        __libc_back_signalSemRelease();

        /*
         * Reset the event semaphore and resume waiting.
         */
        ULONG cCount = 0;
        rc = DosResetEventSem(ghevTimer, &cCount);
        if (rc)
        {
            LIBCLOG_MSG("Timer worker stopping: DosResetEventSem -> %d gfTerminate=%d\n", rc, gfTerminate);
            break;
        }
        if (cCount > 1)
            LIBCLOG_MSG("cCount=%ld missed=%ld shots!\n", cCount, cCount - 1);
    } /* loop forever */

    /*
     * Cleanup.
     */
    /* kill event semaphore. */
    HEV hev = ghevTimer;
    if (hev)
    {
        ghevTimer = NULLHANDLE;
        DosPostEventSem(hev);
        DosCloseEventSem(hev);
    }

    /* kill timer. */
    HTIMER hTimer = (HTIMER)__atomic_xchg((volatile unsigned *)(void *)&ghTimer, ~0U);
    if (hTimer != ~0U)
        DosStopTimer(hTimer);

    __atomic_xchg(&gtidWorker, 0);
    LIBCLOG_RETURN_VOID();
}


/**
 * The the current millisecond tick.
 *
 * @returns current timestamp.
 */
static unsigned signalTimerClock(void)
{
    /* No need for fs stuff here, caller handles that. */
    ULONG ul = 0;
    DosQuerySysInfo(QSV_MS_COUNT, QSV_MS_COUNT, &ul, sizeof(ul));
    return ul;
}


/**
 * Notification that we're terminating.
 * Will terminate the timer thread.
 */
void __libc_back_signalTimerNotifyTerm(void)
{
    if (gtidWorker)
    {
        /* tell thread to terminate. */
        __atomic_xchg(&gfTerminate, 1);
        __atomic_xchg(&gfArmed, 0);

        /* kill event semaphore. */
        HEV hev = ghevTimer;
        if (hev)
        {
            ghevTimer = NULLHANDLE;
            DosPostEventSem(hev);
            DosCloseEventSem(hev);
        }

        /* kill timer. */
        HTIMER hTimer = (HTIMER)__atomic_xchg((volatile unsigned *)(void *)&ghTimer, ~0U);
        if (hTimer != ~0U)
            DosStopTimer(hTimer);

        /* kill the thread (it should've taken the hints by now, but just in case) */
        TID tid = gtidWorker;
        if (tid)
        {
            DosSleep(0);
            tid = gtidWorker;
            if (tid)
            {
                DosKillThread(tid);
                __atomic_xchg(&gtidWorker, 0);
            }
        }
    }
}


/**
 * Queries and/or starts/stops a timer.
 *
 * @returns 0 on success.
 * @returns Negative error code (errno.h) on failure.
 * @param   iWhich      Which timer to get, any of the ITIMER_* #defines.
 *                      OS/2 only supports ITIMER_REAL.
 * @param   pValue      Where to store the value.
 *                      Optional. If NULL pOldValue must not be NULL.
 * @param   pOldValue   Where to store the old value.
 *                      Optional. If NULL pValue must not be NULL.
 */
int __libc_Back_signalTimer(int iWhich, const struct itimerval *pValue, struct itimerval *pOldValue)
{
    LIBCLOG_ENTER("iWhich=%d pValue=%p{.ti_value={.ti_sec=%ld, .ti_usec=%ld}, ti_interval={.ti_sec=%ld, .ti_usec=%ld}} pOldValue=%p\n",
                  iWhich, (void *)pValue,
                  pValue ? pValue->it_value.tv_sec : -1,
                  pValue ? pValue->it_value.tv_usec : -1,
                  pValue ? pValue->it_interval.tv_sec : -1,
                  pValue ? pValue->it_interval.tv_usec : -1,
                  (void *)pOldValue);

    /*
     * Validate.
     */
    if (iWhich != ITIMER_REAL && iWhich != ITIMER_VIRTUAL && iWhich != ITIMER_PROF)
        LIBCLOG_ERROR_RETURN(-EINVAL, "ret -EINVAL - Invalid timer iWhich=%d!\n", iWhich);
    if (!pValue && !pOldValue)
        LIBCLOG_ERROR_RETURN(-EFAULT, "ret -EFAULT - both pointers are NULL - will fault on BSD!\n");
    struct itimerval    Value = {{0,0},{0,0}};
    unsigned            uNext = 0;
    unsigned            uInterval = 0;
    if (pValue)
    {
        Value = *pValue;
        if (    Value.it_interval.tv_sec < 0
            ||  Value.it_interval.tv_sec > 1000000
            ||  Value.it_interval.tv_usec < 0
            ||  Value.it_interval.tv_usec > 1000000
            ||  Value.it_value.tv_sec < 0
            ||  Value.it_value.tv_sec > 1000000
            ||  Value.it_value.tv_usec < 0
            ||  Value.it_value.tv_usec > 1000000)
        {
            LIBC_ASSERTM_FAILED("Invalid Value! {.ti_value={.ti_sec=%ld, .ti_usec=%ld}, ti_interval={.ti_sec=%ld, .ti_usec=%ld}}\n",
                                Value.it_value.tv_sec, Value.it_value.tv_usec, Value.it_interval.tv_sec, Value.it_interval.tv_usec);
            LIBCLOG_RETURN_INT(-EINVAL);
        }

        /*
         * Convert to milliseconds.
         */
        if (Value.it_value.tv_sec || Value.it_value.tv_usec)
        {
            uNext = Value.it_value.tv_sec * 1000 + Value.it_value.tv_usec / 1000;
            if (!uNext)
                uNext = 16;
            else
                uNext = (uNext + 15) & ~15;

            if (Value.it_interval.tv_sec || Value.it_interval.tv_usec)
            {
                uInterval = Value.it_interval.tv_sec * 1000 + Value.it_interval.tv_usec / 1000;
                if (!uInterval)
                    uInterval = 16;
                else
                    uInterval = (uInterval + 15) & ~15;
            }
        }
    }
    if (iWhich != ITIMER_REAL)
        LIBCLOG_ERROR_RETURN(-ENOSYS, "ret -ENOSYS - %s is not implemented on OS/2!\n", iWhich == ITIMER_VIRTUAL ? "ITIMER_VIRTUAL" : "ITIMER_PROF");


    /*
     * Gain exclusive access to the signal stuff.
     */
    int rc = __libc_back_signalSemRequest();
    if (rc)
        LIBCLOG_RETURN_INT(rc);
    FS_VAR();
    FS_SAVE_LOAD();

    /*
     * If this is a request to modify the timer, let's try make sure it
     * doesn't fire while we're in here and modifies it.
     */
    if (pValue)
    {
        __atomic_xchg(&gfArmed, 0);
        HTIMER hTimer = (HTIMER)__atomic_xchg((volatile unsigned *)(void *)&ghTimer, ~0U);
        if (hTimer != ~0U)
            rc = DosStopTimer(hTimer);
    }

    /*
     * Get the current timer.
     * This is *not* gonna be very accurate!
     */
    if (pOldValue)
    {
        if (gfArmed)
        {
            pOldValue->it_interval.tv_sec = guInterval / 1000;
            pOldValue->it_interval.tv_usec = (guInterval % 1000) * 1000;

            unsigned u = guNext;
            u -= signalTimerClock();
            if (u <= guInterval)
            {
                pOldValue->it_value.tv_sec = u / 1000;
                pOldValue->it_value.tv_usec = (u % 1000) * 1000;
            }
        }
        else
            memset(pOldValue, 0, sizeof(*pOldValue));
    }

    /*
     * Set the timer.
     */
    if (pValue)
    {
        /*
         * Start the worker thread if it's not already running.
         */
        if (!gtidWorker && uNext)
        {
            gfArmed = 0;
            gfOneShot = 0;
            guInterval = 0;
            guNext = 0;
            ghevTimer = NULLHANDLE;
            gfTerminate = 0;
            ghTimer = ~0U;
            rc = DosCreateEventSem(NULL, (PHEV)&ghevTimer, DC_SEM_SHARED, FALSE);
            if (!rc)
            {
                rc = __libc_back_threadCreate(signalTimerWorkerThread, 0x10000, NULL, 1 /* Internal */);
                if (rc > 0)
                {
                    gtidWorker = rc;
                    LIBCLOG_MSG("Create timer thread %d\n", gtidWorker);
                    rc = 0;
                }
                else
                {
                    rc = gtidWorker;
                    DosCloseEventSem(ghevTimer);
                    LIBC_ASSERTM_FAILED("Failed to create timer thread! rc=%d\n", rc);
                    ghevTimer = NULLHANDLE;
                }
            }
            else
                ghevTimer = NULLHANDLE;
        }
        if (!rc && uNext) /* (Timer is already stopped.) */
        {
            if (uInterval == uNext)
            {
                /*
                 * Start a reoccuring timer.
                 */
                __atomic_xchg(&gfArmed, 1);
                __atomic_xchg(&gfOneShot, 0);
                __atomic_xchg(&guInterval, uInterval);
                __atomic_xchg(&guNext, signalTimerClock() + uInterval + 1);
                rc = DosStartTimer(uInterval, (HSEM)ghevTimer, (PHTIMER)&ghTimer);
            }
            else
            {
                /*
                 * We need to start a one-shot timer first, and
                 * then the timer thread will arm a new timer with
                 * the specified interval.
                 */
                __atomic_xchg(&gfArmed, 1);
                __atomic_xchg(&gfOneShot, 1);
                __atomic_xchg(&guInterval, uInterval);
                __atomic_xchg(&guNext, signalTimerClock() + uNext + 1);
                rc = DosAsyncTimer(uNext, (HSEM)ghevTimer, (PHTIMER)&ghTimer);
            }
        }
    }

    /*
     * Release semaphore.
     */
    __libc_back_signalSemRelease();
    FS_RESTORE();

    /*
     * Check for failure and return.
     */
    if (!rc)
        LIBCLOG_RETURN_MSG(0, "ret 0 *pOldValue={.ti_value={.ti_sec=%ld, .ti_usec=%ld}, ti_interval={.ti_sec=%ld, .ti_usec=%ld}}\n",
                           pOldValue ? pOldValue->it_value.tv_sec : -1,
                           pOldValue ? pOldValue->it_value.tv_usec : -1,
                           pOldValue ? pOldValue->it_interval.tv_sec : -1,
                           pOldValue ? pOldValue->it_interval.tv_usec : -1);
    if (rc > 0)
    {
        if (rc == ERROR_TS_NOTIMER)
            rc = -EAGAIN;
        else
            rc = -__libc_native2errno(rc);
    }
    LIBCLOG_ERROR_RETURN_INT(rc);
}

