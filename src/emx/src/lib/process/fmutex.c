/* fmutex.c (emx+gcc) -- Copyright (c) 1996 by Eberhard Mattes */

#include "libc-alias.h"
#define INCL_DOSSEMAPHORES
#define INCL_DOSPROCESS
#define INCL_DOSERRORS
#define INCL_DOSEXCEPTIONS
#define INCL_FSMACROS
#define INCL_EXAPIS
#include <os2.h>
#include <stdlib.h>
#include <string.h>
#include <sys/builtin.h>
#include <sys/fmutex.h>
#include <sys/smutex.h>
#include <InnoTekLIBC/FastInfoBlocks.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_MUTEX
#include <InnoTekLIBC/logstrict.h>
#include <InnoTekLIBC/thread.h>
#include <InnoTekLIBC/backend.h>

/*******************************************************************************
*   Internal Functions                                                         *
*******************************************************************************/
static void __fmutex_deadlock(_fmutex *pSem, const char *pszMsg);


/* These functions are available even in single-thread libraries. */


unsigned _fmutex_create(_fmutex *sem, unsigned flags)
{
    LIBCLOG_ENTER("sem=%p flags=%#x\n", (void *)sem, flags);
    unsigned rc = _fmutex_create2(sem, flags, NULL);
    if (!rc)
        LIBCLOG_RETURN_UINT(rc);
    LIBCLOG_ERROR_RETURN_UINT(rc);
}


unsigned _fmutex_create2(_fmutex *sem, unsigned flags, const char *pszDesc)
{
    LIBCLOG_ENTER("sem=%p flags=%#x pszDesc=%s\n", (void *)sem, flags, pszDesc);
    unsigned rc;

    sem->hev        = 0;
    sem->Owner      = 0;
    sem->fs         = _FMS_AVAILABLE;
    sem->flags      = flags;
    sem->padding[0] = 'f';
    sem->padding[1] = 'm';
    sem->pszDesc    = pszDesc;
    rc = DosCreateEventSemEx(NULL, (PHEV)&sem->hev,
                             (flags & _FMC_SHARED) ? DC_SEM_SHARED : 0,
                             FALSE);
    if (!rc)
        LIBCLOG_RETURN_UINT(rc);
    LIBCLOG_ERROR_RETURN_UINT(rc);
}


unsigned _fmutex_open(_fmutex *sem)
{
    LIBCLOG_ENTER("sem=%p{.pszDesc=%s}\n", (void *)sem, sem->pszDesc);
    unsigned rc = DosOpenEventSemEx(NULL, (PHEV)&sem->hev);
    if (!rc)
        LIBCLOG_RETURN_UINT(rc);
    LIBCLOG_ERROR_RETURN_UINT(rc);
}


unsigned _fmutex_close(_fmutex *sem)
{
    LIBCLOG_ENTER("sem=%p{.pszDesc=%s}\n", (void *)sem, sem->pszDesc);
    unsigned rc = DosCloseEventSemEx(sem->hev);
    if (!rc)
        LIBCLOG_RETURN_UINT(rc);
    LIBCLOG_ERROR_RETURN_UINT(rc);
}


unsigned __fmutex_request_internal(_fmutex *sem, unsigned flags, signed char fs)
{
    LIBCLOG_ENTER("sem=%p{.pszDesc=%s} flags=%#x fs=%#x\n", (void *)sem, sem->pszDesc, flags, (int)fs);
    int     rc;

    if (fs == _FMS_UNINIT)
    {
        LIBC_ASSERTM_FAILED("Invalid handle, fs == _FMS_UNINIT\n");
        LIBCLOG_RETURN_UINT(ERROR_INVALID_HANDLE);
    }

    if (fs == _FMS_AUTO_INITIALIZE || !sem->hev)
    {
        HEV hev;
        rc = DosCreateEventSemEx(NULL, &hev, sem->flags & _FMC_SHARED ? DC_SEM_SHARED : 0, FALSE);
        if (rc)
        {
            LIBC_ASSERTM_FAILED("Failed to create event semaphore for fmutex '%s', rc=%d. flags=%#x\n", sem->pszDesc, rc, sem->flags);
            LIBCLOG_RETURN_UINT(ERROR_INVALID_HANDLE);
        }
        if (__atomic_cmpxchg32((volatile uint32_t *)&sem->hev, hev, 0))
        {
            LIBCLOG_MSG("auto initialized fmutex %p '%s'\n", (void *)sem, sem->pszDesc);
            __atomic_xchg(&sem->Owner, fibGetTidPid());
            LIBCLOG_RETURN_UINT(NO_ERROR);
        }
        LIBCLOG_MSG("lost auto initialization race for fmutex %p '%s'\n", (void *)sem, sem->pszDesc);
        DosCloseEventSemEx(hev);
        fs = _FMS_OWNED_HARD;
    }

    if (flags & _FMR_NOWAIT)
    {
        if (fs == _FMS_OWNED_HARD)
        {
            if (__cxchg(&sem->fs, _FMS_OWNED_HARD) == _FMS_AVAILABLE)
            {
                __atomic_xchg(&sem->Owner, fibGetTidPid());
                LIBCLOG_RETURN_UINT(0);
            }
        }
        LIBCLOG_RETURN_UINT(ERROR_MUTEX_OWNED);
    }

    for (;;)
    {
        ULONG ulCount;
        FS_VAR();
        FS_SAVE_LOAD();
        rc = DosResetEventSem(sem->hev, &ulCount);
        FS_RESTORE();
        if (rc != 0 && rc != ERROR_ALREADY_RESET)
            LIBCLOG_RETURN_UINT(rc);
        if (__cxchg(&sem->fs, _FMS_OWNED_HARD) == _FMS_AVAILABLE)
        {
            __atomic_xchg(&sem->Owner, fibGetTidPid());
            LIBCLOG_RETURN_UINT(0);
        }
        if (sem->Owner == fibGetTidPid())
        {
            __fmutex_deadlock(sem, "Recursive mutex!");
            LIBCLOG_RETURN_UINT(-1);
        }

        FS_SAVE_LOAD();

        for (;;)
        {
            ULONG cMsWait = sem->flags & _FMC_SHARED ? SEM_INDEFINITE_WAIT : 3000;
            if (fibIsInExit())
                cMsWait = 250;
            rc = DosWaitEventSem(sem->hev, cMsWait);
            if (rc == ERROR_INTERRUPT)
            {
                /*
                 * An interrupt occured, this might be a bug in the wait, or
                 * more likely someone is killing us. If we're dying/exiting or
                 * something along those lines, we return to the caller no matter
                 * what flags it specified.
                 */
                if (!(flags & _FMR_IGNINT))
                    break;
                if (fibIsInExit())
                    break;
                rc = ERROR_TIMEOUT;
            }
            if (rc != ERROR_TIMEOUT)
                break;

            /*
             * Deadlock detection - check if owner is around.
             */
            unsigned Owner = sem->Owner;
            if (!Owner)
            {
                /*
                 * The owner could have released the mutex after we timed out.
                 * Reset rc to loop again (and possibly grab the mutex) rather
                 * than return a failure.
                 */
                rc = 0;
                break;
            }
            rc = DosVerifyPidTid(Owner >> 16, Owner & 0xffff);
            if (rc)
            {
                __fmutex_deadlock(sem, "Owner died!");
                rc = ERROR_SEM_OWNER_DIED;
                break;
            }
        }

        FS_RESTORE();
        if (rc != 0)
            LIBCLOG_RETURN_UINT(rc);
    }
}

unsigned __fmutex_request_internal_must_complete(_fmutex *sem, unsigned flags)
{
    LIBCLOG_ENTER("sem=%p{.pszDesc=%s} flags=%#x\n", (void *)sem, sem->pszDesc, flags);
    int     rc;
    ULONG   ulNesting = 0;
    signed char fs;
    FS_VAR();

    if (sem->fs == _FMS_UNINIT)
    {
        LIBC_ASSERTM_FAILED("Invalid handle, fs == _FMS_UNINIT\n");
        LIBCLOG_RETURN_UINT(ERROR_INVALID_HANDLE);
    }

    FS_SAVE_LOAD();
    DosEnterMustComplete(&ulNesting);
    if (ulNesting == 1) /* This is a hack to catch lost poke signals. */
    {
        __LIBC_PTHREAD pThrd = __libc_threadCurrentNoAuto();
        if (pThrd && pThrd->fSigBeingPoked)
        {
            DosExitMustComplete(&ulNesting);
            __libc_Back_signalLostPoke();
            DosEnterMustComplete(&ulNesting);
        }
    }
    fs = __cxchg(&sem->fs, _FMS_OWNED_SIMPLE);
    if (fs == _FMS_AVAILABLE)
    {
        __atomic_xchg(&sem->Owner, fibGetTidPid());
        FS_RESTORE();
        LIBCLOG_RETURN_UINT(0);
    }

    if (fs == _FMS_AUTO_INITIALIZE || !sem->hev)
    {
        HEV hev;
        rc = DosCreateEventSemEx(NULL, &hev, sem->flags & _FMC_SHARED ? DC_SEM_SHARED : 0, FALSE);
        if (rc)
        {
            DosExitMustComplete(&ulNesting);
            LIBC_ASSERTM_FAILED("Failed to create event semaphore for fmutex '%s', rc=%d. flags=%#x\n", sem->pszDesc, rc, sem->flags);
            FS_RESTORE();
            LIBCLOG_RETURN_UINT(ERROR_INVALID_HANDLE);
        }
        if (__atomic_cmpxchg32((volatile uint32_t *)&sem->hev, hev, 0))
        {
            LIBCLOG_MSG("auto initialized fmutex %p '%s'\n", (void *)sem, sem->pszDesc);
            __atomic_xchg(&sem->Owner, fibGetTidPid());
            FS_RESTORE();
            LIBCLOG_RETURN_UINT(NO_ERROR);
        }
        LIBCLOG_MSG("lost auto initialization race for fmutex %p '%s'\n", (void *)sem, sem->pszDesc);
        DosCloseEventSemEx(hev);
        fs = _FMS_OWNED_HARD;
    }

    if (flags & _FMR_NOWAIT)
    {
        if (fs == _FMS_OWNED_HARD)
        {
            if (__cxchg(&sem->fs, _FMS_OWNED_HARD) == _FMS_AVAILABLE)
            {
                __atomic_xchg(&sem->Owner, fibGetTidPid());
                FS_RESTORE();
                LIBCLOG_RETURN_UINT(0);
            }
        }
        DosExitMustComplete(&ulNesting);
        FS_RESTORE();
        LIBCLOG_RETURN_UINT(ERROR_MUTEX_OWNED);
    }

    for (;;)
    {
        ULONG ulCount;
        rc = DosResetEventSem(sem->hev, &ulCount);
        if (rc != 0 && rc != ERROR_ALREADY_RESET)
        {
            DosExitMustComplete(&ulNesting);
            FS_RESTORE();
            LIBCLOG_RETURN_UINT(rc);
        }
        if (__cxchg(&sem->fs, _FMS_OWNED_HARD) == _FMS_AVAILABLE)
        {
            __atomic_xchg(&sem->Owner, fibGetTidPid());
            FS_RESTORE();
            LIBCLOG_RETURN_UINT(0);
        }
        if (sem->Owner == fibGetTidPid())
        {
            DosExitMustComplete(&ulNesting);
            __fmutex_deadlock(sem, "Recursive mutex!");
            FS_RESTORE();
            LIBCLOG_RETURN_UINT(-1);
        }

        for (;;)
        {
            ULONG cMsWait = sem->flags & _FMC_SHARED ? SEM_INDEFINITE_WAIT : 3000;
            if (fibIsInExit())
                cMsWait = 250;
            DosExitMustComplete(&ulNesting);
            rc = DosWaitEventSem(sem->hev, cMsWait);
            DosEnterMustComplete(&ulNesting);
            if (rc == ERROR_INTERRUPT)
            {
                /*
                 * An interrupt occured, this might be a bug in the wait, or
                 * more likely someone is killing us. If we're dying/exiting or
                 * something along those lines, we return to the caller no matter
                 * what flags it specified.
                 */
                if (!(flags & _FMR_IGNINT))
                    break;
                if (fibIsInExit())
                    break;
                rc = ERROR_TIMEOUT;
            }
            if (rc != ERROR_TIMEOUT)
                break;

            /*
             * Deadlock detection - check if owner is around.
             */
            unsigned Owner = sem->Owner;
            if (!Owner)
            {
                /*
                 * The owner could have released the mutex after we timed out.
                 * Reset rc to loop again (and possibly grab the mutex) rather
                 * than return a failure.
                 */
                rc = 0;
                break;
            }
            rc = DosVerifyPidTid(Owner >> 16, Owner & 0xffff);
            if (rc)
            {
                DosExitMustComplete(&ulNesting);
                __fmutex_deadlock(sem, "Owner died!");
                rc = ERROR_SEM_OWNER_DIED;
                DosEnterMustComplete(&ulNesting);
                break;
            }
        }

        FS_RESTORE();
        if (rc != 0)
        {
            DosExitMustComplete(&ulNesting);
            FS_RESTORE();
            LIBCLOG_RETURN_UINT(rc);
        }
    }
}

static void __fmutex_deadlock(_fmutex *pSem, const char *pszMsg)
{
    if (!fibIsInExit())
        __libc_Back_panic(0, NULL,
                          "fmutex deadlock: %s\n"
                          "%x: Owner=%x Self=%x fs=%x flags=%x hev=%x\n"
                          "            Desc=\"%s\"\n",
                          pszMsg,
                          pSem, pSem->Owner, fibGetTidPid(), pSem->fs, pSem->flags, pSem->hev,
                          pSem->pszDesc);
}

unsigned __fmutex_release_internal_must_complete(_fmutex *sem)
{
    LIBCLOG_ENTER("sem=%p{.pszDesc=%s}\n", (void *)sem, sem->pszDesc);
    ULONG ulNesting;

    int rc = 0;
    __atomic_xchg(&sem->Owner, 0);
    signed char fs = __cxchg(&sem->fs, _FMS_AVAILABLE);
    if (fs == _FMS_OWNED_HARD)
        rc = __fmutex_release_internal (sem);

    FS_VAR_SAVE_LOAD();
    DosExitMustComplete(&ulNesting);
    FS_RESTORE();

    /* This is a hack to catch lost poke signals. */
    if (!ulNesting)
    {
        __LIBC_PTHREAD pThrd = __libc_threadCurrentNoAuto();
        if (pThrd && pThrd->fSigBeingPoked)
            __libc_Back_signalLostPoke();
    }

    LIBCLOG_RETURN_UINT(rc);
}

unsigned __fmutex_release_internal(_fmutex *sem)
{
    LIBCLOG_ENTER("sem=%p{.pszDesc=%s}\n", (void *)sem, sem->pszDesc);
    unsigned rc;
    FS_VAR();

    FS_SAVE_LOAD();
    rc = DosPostEventSem(sem->hev);
    if (rc != 0 && rc != ERROR_ALREADY_POSTED)
    {
        FS_RESTORE();
        LIBCLOG_ERROR_RETURN_UINT(rc);
    }

    /* Give up our time slice to give other threads a chance.  Without
       doing so, a thread which continuously requests and releases a
       _fmutex semaphore may prevent all other threads requesting the
       _fmutex semaphore from running, forever.

       Why?  Without DosSleep, the thread will keep the CPU for the rest
       of the time slice.  It may request (and get) the semaphore again
       in the same time slice, perhaps multiple times.  Assume that the
       semaphore is still owned at the end of the time slice.  In one of
       the next time slices, all the other threads blocking on the
       semaphore will wake up for a short while only to see that the
       semaphore is owned; they will all go sleeping again.  The thread
       which owns the semaphore will get the CPU again.  And may happen
       to own the semaphore again at the end of the time slice.  And so
       on.  DosSleep reduces that problem a bit.

       Even with DosSleep(0), assignment of time slices to threads
       requesting a _fmutex semaphore can be quite unbalanced: The more
       frequently a thread requests a _fmutex semaphore, the more CPU
       time it will get.  In consequence, a thread which only rarely
       needs the resource protected by the _fmutex semaphore will get
       much less CPU time than threads competing heavily for the
       resource.

       DosSleep(1) would make _fmutex semaphores behave a bit better,
       but would also give up way too much CPU time (in the average,
       half a time slice (or one and a half?) per call of this
       function).

       DosReleaseMutexSem does a better job; it gives the time slice to
       the thread which has waited longest on the semaphore (if all the
       threads have the same priority).

       Unfortunately, we cannot use an HMTX instead of an HEV because
       only the owner (the thread which got assigned ownership by
       DosRequestMutexSem) can release the HMTX with DosReleaseMutexSem.
       If we could, that would take advantage of the FIFO of blocked
       threads maintained by the kernel for each HMTX.

       Using an HMTX would require the first thread requesting an owned
       _fmutex semaphore to request the HMTX on behalf of the thread
       owning the _fmutex semaphore. */

    DosSleep(0);
    FS_RESTORE();
    LIBCLOG_RETURN_UINT(0);
}


void _fmutex_dummy(_fmutex *sem)
{
    LIBCLOG_ENTER("sem=%p\n", (void *)sem);
    sem->hev = 0;
    sem->fs = _FMS_AVAILABLE;
    sem->flags = _FMC_DUMMY;
    sem->Owner = 0;
    sem->pszDesc = "dummy";
    LIBCLOG_RETURN_VOID();
}

