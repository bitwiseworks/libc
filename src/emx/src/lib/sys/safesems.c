/* $Id: safesems.c 3947 2015-12-26 18:48:17Z bird $ */
/** @file
 *
 * LIBC SYS Backend - Internal Signal-Safe Semaphores.
 *
 * Copyright (c) 2005 knut st. osmundsen <bird@anduin.net>
 *
 *
 * This file is part of InnoTek LIBC.
 *
 * InnoTek LIBC is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * InnoTek LIBC is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with InnoTek LIBC; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */


/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#define INCL_DOSSEMAPHORES
#define INCL_DOSEXCEPTIONS
#define INCL_DOSERRORS
#define INCL_DOSPROCESS
#define INCL_FSMACROS
#define INCL_EXAPIS
#include <os2emx.h>
#include <sys/errno.h>
#include "syscalls.h"
#include <sys/builtin.h>
#include <InnoTekLIBC/FastInfoBlocks.h>
#include <InnoTekLIBC/thread.h>
#include <InnoTekLIBC/backend.h>
#define __LIBC_LOG_GROUP    __LIBC_LOG_GRP_BACK_IPC
#include <InnoTekLIBC/logstrict.h>


/*******************************************************************************
*   Structures and Typedefs                                                    *
*******************************************************************************/
/**
 * Arguments to semEvSleepSignalCallback().
 */
struct SignalArgs
{
    /** Set if pfnComplete have already been executed. */
    int volatile            fDone;
    /** Number of attempts at doing it. (in case of crashes in pfnComplete) */
    int volatile            cTries;
    /** Callback to execute. */
    void                  (*pfnComplete)(void *pvUser);
    /** User arg. */
    void                   *pvUser;
    /** The old priority - to be restored when we're unblocked. */
    ULONG                   ulOldPri;
    /** Pointer to the semaphore structure. */
    __LIBC_PSAFESEMEV       pEv;
};


/**
 * Creates a safe mutex sem.
 *
 * @returns 0 on success.
 * @returns Negative error code (errno.h) on failure.
 * @param   pmtx        Pointer to the semaphore structure to initialize.
 * @param   fShared     Set if the semaphore should be sharable between processes.
 */
int __libc_Back_safesemMtxCreate(__LIBC_PSAFESEMMTX pmtx, int fShared)
{
    LIBCLOG_ENTER("pmtx=%p fShared=%d\n", (void *)pmtx, fShared);
    pmtx->hmtx = NULLHANDLE;
    pmtx->fShared = fShared;
    int rc = DosCreateMutexSemEx(NULL, &pmtx->hmtx, fShared ? DC_SEM_SHARED : 0, FALSE);
    if (rc)
        rc = -__libc_native2errno(rc);
    LIBCLOG_RETURN_MSG(rc, "ret %d hmtx=%#lx\n", rc, pmtx->hmtx);
}


/**
 * Opens a shared safe mutex sem.
 *
 * @returns 0 on success.
 * @returns Negative error code (errno.h) on failure.
 * @param   pmtx        Pointer to the semaphore structure.
 */
int __libc_Back_safesemMtxOpen(__LIBC_PSAFESEMMTX pmtx)
{
    LIBCLOG_ENTER("pmtx=%p:{.hmtx=%#lx}\n", (void *)pmtx, pmtx->hmtx);
    int rc = DosOpenMutexSemEx(NULL, &pmtx->hmtx);
    if (rc)
        rc = -__libc_native2errno(rc);
    LIBCLOG_RETURN_INT(rc);
}


/**
 * Closes a shared safe mutex sem.
 *
 * @returns 0 on success.
 * @returns Negative error code (errno.h) on failure.
 * @param   pmtx        Pointer to the semaphore structure.
 */
int __libc_Back_safesemMtxClose(__LIBC_PSAFESEMMTX pmtx)
{
    LIBCLOG_ENTER("pmtx=%p:{.hmtx=%#lx}\n", (void *)pmtx, pmtx->hmtx);
    int rc = DosCloseMutexSemEx(pmtx->hmtx);
    if (rc)
        rc = -__libc_native2errno(rc);
    LIBCLOG_RETURN_INT(rc);
}


/**
 * This function checks that there is at least 2k of writable
 * stack available. If there isn't, a crash is usually the
 * result.
 *
 * @returns     0 on success.
 * @returns     -1 on failure.
 * @internal
 */
static int __libc_back_safesemStackChecker(void)
{
    char volatile *pch;
    unsigned u;
    __asm__ __volatile__("movl  %%esp, %0\n\t"
                         "movl  -4(%0), %1\n\t"
                         "movl  %1, -4(%0)\n\t"
                         "movl  -1024(%0), %1\n\t"
                         "movl  %1, -1024(%0)\n\t"
                         "movl  -2048(%0), %1\n\t"
                         "movl  %1, -2048(%0)\n\t"
                         : "=r" (pch), "=r" (u));
    return 0;
}


/**
 * Locks a mutex semaphore.
 *
 * @returns 0 on success.
 * @returns Negative errno on failure.
 * @param   pmtx        Pointer to the semaphore structure.
 */
int __libc_Back_safesemMtxLock(__LIBC_PSAFESEMMTX pmtx)
{
    LIBCLOG_ENTER("pmtx=%p:{.hmtx=%#lx}\n", (void *)pmtx, pmtx->hmtx);
    ULONG       ul = 0;
    HMTX        hmtx = pmtx->hmtx;
    int         rc;
    FS_VAR();

    /*
     * Check stack.
     */
    if (__predict_false(__libc_back_safesemStackChecker() != 0))
    {
        LIBC_ASSERTM_FAILED("Too little stack left!\n");
        LIBCLOG_RETURN_INT(-EFAULT);
    }

    /*
     * Request semaphore and enter "must-complete section" to avoid signal trouble.
     *
     * We start by trying for 1ms inside a must-complete section. This will usually
     * succeed and is the safest way of acquiring the mutex. While waiting from inside
     * a must-complete section we're unkillable, so we can only do this for very short
     * periods.
     */
    FS_SAVE_LOAD();
    DosEnterMustComplete(&ul);
    rc = DosRequestMutexSem(hmtx, 1);
    if (__predict_false(rc != NO_ERROR))
    {
        /*
         * Ok, do an interruptable wait then and handle all kind of errors that might occur.
         */
        unsigned cOwnerDied = 30;
        unsigned msStart = fibGetMsCount();
        unsigned msSleep = 30*1000;
        for (;;)
        {
            DosExitMustComplete(&ul);
            rc = DosRequestMutexSem(hmtx, msSleep);
            DosEnterMustComplete(&ul);
            if (!rc)
                break;

            if (rc == ERROR_INTERRUPT || rc == ERROR_SEM_TIMEOUT || rc == ERROR_TIMEOUT)
            {
                /*
                 * If the waiting was interrupted someone wants our attention, possibly because
                 * we're dying. If we timed out we've got a potential deadlock on our hands,
                 * this can also happen during termination.
                 *
                 * Check if the process is dying before retrying to get the semaphore. Being
                 * stubborn during process termination usually lead to bad deadlocks.
                 */
                if (fibIsInExit())
                {
                    DosExitMustComplete(&ul); /* we're terminating, don't give a shit about signals now, just hurry up and die! */
                    FS_RESTORE();
                    LIBCLOG_ERROR_RETURN_MSG(-EDEADLK, "ret -%d (we're dying)\n", EDEADLK);
                }
                /* Not dying, retry. */
            }
            else if (rc == ERROR_SEM_OWNER_DIED && --cOwnerDied > 0)
            {
                /*
                 * Semaphore owner dies, try remedy the situation.
                 */
                HMTX hmtxNew;
                LIBCLOG_MSG("Semaphore owner died. !SHIT!\n");
                rc = DosCreateMutexSemEx(NULL, &hmtxNew, pmtx->fShared ? DC_SEM_SHARED : 0, TRUE);
                if (rc)
                    __libc_Back_panic(0, NULL, "safesem mutex owner died: create mutex failed, rc=%d. pmtx=%p:{.hmtx=%x .fShared=%d}\n",
                                      rc, (void *)pmtx, (unsigned)pmtx->hmtx, pmtx->fShared);
                /* we race here. */
                if (__atomic_cmpxchg32((uint32_t volatile *)&pmtx->hmtx, (uint32_t)hmtx, (uint32_t)hmtxNew))
                    break;

                /* beaten. */
                DosCloseMutexSemEx(hmtxNew);
                msSleep = 2;        /* force retry */
                hmtx = pmtx->hmtx;
            }
            else
                __libc_Back_panic(0, NULL, "safesem mutex requested failed, rc=%d. pmtx=%p:{.hmtx=%x .fShared=%d}\n",
                                  rc, (void *)pmtx, (unsigned)pmtx->hmtx, pmtx->fShared);

            /*
             * Calculate sleep time / check for timeout.
             */
            if (msSleep <= 1)
                __libc_Back_panic(0, NULL, "safesem mutex timeout, %u ms: pmtx=%p:{.hmtx=%x .fShared=%d}\n",
                                  fibGetMsCount() - msStart, (void *)pmtx, (unsigned)pmtx->hmtx, pmtx->fShared);
            msSleep = fibGetMsCount() - msStart;
            if (msSleep < 30*1000)
                msSleep = 30*1000 - msSleep;
            else
                msSleep = 1; /* one last try before we panic. */

            LIBCLOG_MSG("Interrupted, retry %d ms\n", msSleep);
        } /* for (;;) */
    }

    /* success */
    FS_RESTORE();
    LIBCLOG_RETURN_INT(0);
}


/**
 * Unlocks a mutex semaphore.
 *
 * @returns 0 on success.
 * @returns Negative errno on failure.
 * @param   pmtx        Pointer to the semaphore structure.
 */
int __libc_Back_safesemMtxUnlock(__LIBC_PSAFESEMMTX pmtx)
{
    LIBCLOG_ENTER("pmtx=%p:{.hmtx=%#lx}\n", (void *)pmtx, pmtx->hmtx);
    ULONG       ul = 0;
    int         rc;
    FS_VAR();

    /*
     * Release the semaphore.
     */
    FS_SAVE_LOAD();
    rc = DosReleaseMutexSem(pmtx->hmtx);
    if (__predict_false(rc != NO_ERROR))
        __libc_Back_panic(0, NULL, "safesem mutex release failed, rc=%d. pmtx=%p:{.hmtx=%x .fShared=%d}\n",
                          rc, (void *)pmtx, (unsigned)pmtx->hmtx, pmtx->fShared);

    DosExitMustComplete(&ul);
    FS_RESTORE();
    LIBCLOG_RETURN_INT(0);
}




/**
 * Creates a safe event sem.
 *
 * @returns 0 on success.
 * @returns Negative error code (errno.h) on failure.
 * @param   pev         Pointer to the semaphore structure to initialize.
 * @param   pmtx        Pointer to the mutex semaphore which protects the event semaphore.
 * @param   fShared     Set if the semaphore should be sharable between processes.
 */
int __libc_Back_safesemEvCreate(__LIBC_PSAFESEMEV pev, __LIBC_PSAFESEMMTX pmtx, int fShared)
{
    LIBCLOG_ENTER("phev=%p pmtx=%p:{.hmtx=%#lx} fShared=%d\n", (void *)pev, (void *)pmtx, pmtx->hmtx, fShared);

    pev->hev        = NULLHANDLE;
    pev->cWaiters   = 0;
    pev->pmtx       = pmtx;
    pev->fShared    = fShared;

    int rc = DosCreateEventSemEx(NULL, &pev->hev, fShared ? DC_SEM_SHARED : 0, FALSE);
    if (rc)
        rc = -__libc_native2errno(rc);

    LIBCLOG_RETURN_MSG(rc, "ret %d pev->hev=%#lx\n", rc, pev->hev);
}


/**
 * Opens a shared safe event sem.
 *
 * The caller is responsible for opening the associated mutex
 * semaphore before calling this function.
 *
 * @returns 0 on success.
 * @returns Negative error code (errno.h) on failure.
 * @param   pev         Pointer to the semaphore structure to open.
 */
int __libc_Back_safesemEvOpen(__LIBC_PSAFESEMEV pev)
{
    LIBCLOG_ENTER("pev=%p:{.hev=%#lx .cWaiters=%d .pmtx=%p .fShared=%d}\n",
                  (void *)pev, pev->hev, pev->cWaiters, (void *)pev->pmtx, pev->fShared);

    int rc = DosOpenEventSemEx(NULL, &pev->hev);
    if (rc)
        rc = -__libc_native2errno(rc);

    LIBCLOG_RETURN_INT(rc);
}


/**
 * Closes a shared safe mutex sem.
 *
 * @returns 0 on success.
 * @returns Negative error code (errno.h) on failure.
 * @param   pev         Pointer to the semaphore structure to close.
 */
int __libc_Back_safesemEvClose(__LIBC_PSAFESEMEV pev)
{
    LIBCLOG_ENTER("pev=%p:{.hev=%#lx .cWaiters=%d .pmtx=%p .fShared=%d}\n",
                  (void *)pev, pev->hev, pev->cWaiters, (void *)pev->pmtx, pev->fShared);

    int rc = DosCloseEventSemEx(pev->hev);
    if (rc)
        rc = -__libc_native2errno(rc);

    LIBCLOG_RETURN_INT(rc);
}


/**
 * Signal notification callback.
 *
 * This will call the completion handler and restore the thread priority before
 * any signal is executed. We're protect by the signal mutex/must-complete here.
 */
static void semEvSleepSignalCallback(int iSignal, void *pvUser)
{
    LIBCLOG_ENTER("iSignal=%d pvUser=%p\n", iSignal, pvUser);
    struct SignalArgs *pArgs = (struct SignalArgs *)pvUser;

    if (!pArgs->fDone && pArgs->cTries++ < 5)
    {
        pArgs->pfnComplete(pArgs->pvUser);
        int rc = DosSetPriority(PRTYS_THREAD, pArgs->ulOldPri >> 8, pArgs->ulOldPri & 0xff, 0);
        if (__predict_false(rc != NO_ERROR))
            __libc_Back_panic(0, NULL, "DosSetPriority(PRTYS_THREAD, %x, %x, 0) -> %d (3)\n",
                              (unsigned)pArgs->ulOldPri >> 8, (unsigned)pArgs->ulOldPri & 0xff, rc);
        pArgs->fDone = 1;
    }
    LIBCLOG_RETURN_VOID();
}


/**
 * Sleep on a semaphore.
 *
 * The caller must own the associated mutex semaphore. The mutex semaphore will
 * be released as we go to sleep and reclaimed when we wake up.
 *
 * The pfnComplete callback is used to correct state before signals are handled.
 * It will always be called be for this function returns, and it'll either be under
 * the protection of the signal mutex or the associated mutex (both safe sems).
 *
 * This is the most difficult thing we're doing in this API. On OS/2 we have
 * potential (at least theoretically) race conditions...
 *
 * @returns 0 on success.
 * @returns Negative error code (errno.h) on failure.
 *
 * @param   pev         Pointer to the semaphore structure to sleep on.
 * @param   pfnComplete Function to execute on signal or on wait completion.
 * @param   pvUser      User argument to pfnComplete.
 */
int __libc_Back_safesemEvSleep(__LIBC_PSAFESEMEV pev, void (*pfnComplete)(void *pvUser), void *pvUser)
{
    LIBCLOG_ENTER("pev=%p:{.hev=%#lx .cWaiters=%d .pmtx=%p .fShared=%d} pfnComplete=%p pvUser=%p\n",
                  (void *)pev, pev->hev, pev->cWaiters, (void *)pev->pmtx, pev->fShared, (void *)pfnComplete, pvUser);

    /*
     * Setup signal notification callback.
     */
    int rc;
    __LIBC_PTHREAD pThrd = __libc_threadCurrentNoAuto();
    if (__predict_true(pThrd != NULL))
    {
        FS_VAR_SAVE_LOAD();
        PTIB    pTib;
        PPIB    pPib;
        DosGetInfoBlocks(&pTib, &pPib);

        struct SignalArgs Args;
        Args.ulOldPri       = pTib->tib_ptib2->tib2_ulpri;
        Args.fDone          = 0;
        Args.cTries         = 0;
        Args.pfnComplete    = pfnComplete;
        Args.pvUser         = pvUser;

        /* install signal callback. */
        pThrd->pvSigCallbackUser = &Args;
        pThrd->pfnSigCallback    = semEvSleepSignalCallback;

        /*
         * Raise priority to time critical to increase our chances of actually getting
         * blocked before something bad like rescheduling or signaling strikes us.
         */
        rc = DosSetPriority(PRTYS_THREAD, PRTYC_TIMECRITICAL, 31, 0);
        LIBC_ASSERTM(!rc, "DosSetPriority(PRTYS_THREAD, PRTYC_TIMECRITICAL, 31, 0) -> %d\n", rc);
        if (__predict_false(rc && (Args.ulOldPri >> 8) != PRTYC_TIMECRITICAL))
        {
            rc = DosSetPriority(PRTYS_THREAD, PRTYC_TIMECRITICAL, 0, 0);
            LIBC_ASSERTM(!rc, "DosSetPriority(PRTYS_THREAD, PRTYC_TIMECRITICAL, 0, 0) -> %d\n", rc);
        }

        /*
         * Release the sempahore and exit the must complete section.
         */
        __atomic_increment_u32(&pev->cWaiters);
        rc = __libc_Back_safesemMtxUnlock(pev->pmtx);
        if (__predict_true(!rc))
        {
            /*
             * Now, lets block until something happens.
             */
            int rc2 = 0;
            if (__predict_true(!Args.fDone))
                rc2 = DosWaitEventSem(pev->hev, SEM_INDEFINITE_WAIT);
            /** @todo check for fibIsInExit() in some way? */

            /*
             * Reclaim the ownership of the mutex and handle event semaphore errors.
             */
            __atomic_decrement_u32(&pev->cWaiters);
            LIBCLOG_MSG("woke up - rc2=%d\n", rc2);
            rc = __libc_Back_safesemMtxLock(pev->pmtx);
            if (!rc && rc2)
                rc = -__libc_native2errno(rc2);

            /*
             * Restore priority before resetting the semaphore.
             */
            rc2 = DosSetPriority(PRTYS_THREAD, Args.ulOldPri >> 8, Args.ulOldPri & 0xff, 0);
            if (__predict_false(rc2 != NO_ERROR))
                __libc_Back_panic(0, NULL, "DosSetPriority(PRTYS_THREAD, %x, %x, 0) -> %d (1)\n",
                                  (unsigned)Args.ulOldPri >> 8, (unsigned)Args.ulOldPri & 0xff, rc2);
            if (pev->cWaiters > 0)
                DosSleep(0); /* hurry up guys */

            /*
             * Reset the event semaphore.
             */
            ULONG ulIgnore;
            rc2 = DosResetEventSem(pev->hev, &ulIgnore);
            LIBC_ASSERTM(!rc2 && ERROR_ALREADY_RESET, "DosResetEventSem(%#lx,)->%d\n", pev->hev, rc2);
        }
        else
        {
            /*
             * Restore priority.
             */
            int rc2 = DosSetPriority(PRTYS_THREAD, Args.ulOldPri >> 8, Args.ulOldPri & 0xff, 0);
            if (rc2)
                __libc_Back_panic(0, NULL, "DosSetPriority(PRTYS_THREAD, %x, %x, 0) -> %d (2)\n",
                                  (unsigned)Args.ulOldPri >> 8, (unsigned)Args.ulOldPri & 0xff, rc2);
        }

        /*
         * Uninstall the signal callback.
         */
        pThrd->pfnSigCallback = NULL;
        pThrd->pvSigCallbackUser = NULL;

        /*
         * Check for interruption and do completion.
         */
        if (__predict_true(!Args.fDone))
        {
            pfnComplete(pvUser);
            Args.fDone = 1;
        }
        else if (!rc)
            rc = -EINTR;

        FS_RESTORE();
    }
    else
    {
        pfnComplete(pvUser);
        rc = -ENOSYS;
    }

    LIBCLOG_RETURN_INT(rc);
}


/**
 * Wakes up all threads sleeping on a given event semaphore.
 *
 * The caller must own the associated mutex semaphore when calling this
 * function.
 *
 * @returns 0 on success.
 * @returns Negative error code (errno.h) on failure.
 * @param   pev         Pointer to the semaphore structure to post.
 */
int __libc_Back_safesemEvWakeup(__LIBC_PSAFESEMEV pev)
{
    LIBCLOG_ENTER("pev=%p:{.hev=%#lx .cWaiters=%d .pmtx=%p .fShared=%d}\n",
                  (void *)pev, pev->hev, pev->cWaiters, (void *)pev->pmtx, pev->fShared);
    FS_VAR_SAVE_LOAD();
    int rc;

#ifdef __LIBC_STRICT
    /*
     * Check mutex ownership.
     */
    ULONG   cNesting;
    PID     pid;
    TID     tid;
    rc = DosQueryMutexSem(pev->pmtx->hmtx, &pid, &tid, &cNesting);
    LIBC_ASSERTM(!rc, "DosQueryMutexSem(%#lx,,,) -> %d\n", pev->pmtx->hmtx, rc);
    if (!rc)
        LIBC_ASSERTM(pid == fibGetPid() && tid == fibGetTid(),
                     "pid=%d fibGetPid()->%d  tid=%d fibGetTid()->%d\n", (int)pid, fibGetPid(), (int)tid, fibGetTid());
#endif

    /*
     * Post it.
     */
    rc = DosPostEventSem(pev->hev);
    if (!rc)
    {
        if (pev->cWaiters)
        {
            /* hurry up guys! get past the DosWaitEventSem! */
            unsigned cYields = 3;
            do
            {
                DosSleep(0);
                LIBCLOG_MSG("cWaiters=%d cYields=%d\n", pev->cWaiters, cYields);
            } while (pev->cWaiters && --cYields > 0);
        }
    }
    else
    {
        if (rc == ERROR_ALREADY_POSTED || rc == ERROR_TOO_MANY_POSTS)
            rc = 0;
        else
            rc = -__libc_native2errno(rc);
    }

    FS_RESTORE();
    LIBCLOG_RETURN_INT(rc);
}

