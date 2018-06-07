/* $Id: b_processWait.c 3805 2014-02-06 11:37:29Z ydario $ */
/** @file
 *
 * LIBC SYS Backend - waitid.
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
#define INCL_BASE
#define INCL_FSMACROS
#define INCL_EXAPIS
#include <os2emx.h>

#include <sys/wait.h>
#include <errno.h>
#include <string.h>
#include <386/builtin.h>
#include <sys/fmutex.h>
#include "backend.h"
#include "b_process.h"
#include "b_signal.h"
#include <emx/startup.h>
#include <emx/umalloc.h>
#include <InnoTekLIBC/libc.h>
#include <InnoTekLIBC/thread.h>
#include <InnoTekLIBC/sharedpm.h>
#include <InnoTekLIBC/backend.h>
#include <InnoTekLIBC/FastInfoBlocks.h>
#include <InnoTekLIBC/fork.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_BACK_PROCESS
#include <InnoTekLIBC/logstrict.h>


/*******************************************************************************
*   Structures and Typedefs                                                    *
*******************************************************************************/
/** Pointer to wait info node. */
typedef struct WAITINFO *PWAITINFO;

/**
 * Wait info node.
 * This contains the status change information for a
 * child process of the current process.
 */
typedef struct WAITINFO
{
    /** Pointer to the next node in the list. */
    volatile PWAITINFO pNext;
    /** Pointer to the previous node in the list. */
    volatile PWAITINFO pPrev;
    /** Child process id. */
    pid_t       pid;
    /** Child process group. */
    pid_t       pgrp;
    /** Code (si_code for SIGCHLD). */
    int         uCode;
    /** Status (si_status for SIGCHLD). */
    int         uStatus;
    /** Timestamp (si_timestamp for SIGCHLD or exit timestamp). */
    unsigned    uTimestamp;
    /** Real user id of the child process (si_uid for SIGCHLD). */
    uid_t       uid;
} WAITINFO;


/** Pointer to child process info node. */
typedef struct WAITCHILD *PWAITCHILD;
/**
 * Child Process info node.
 */
typedef struct WAITCHILD
{
    /** Pointer to the next one. */
    volatile PWAITCHILD pNext;
    /** Process Id. */
    pid_t               pid;
} WAITCHILD;


/*******************************************************************************
*   Global Variables                                                           *
*******************************************************************************/
/** The mutex protecting the FIFO and event semaphore. */
static _fmutex              gmtxWait;
/** Pointer to the head node of the wait status FIFO. */
static volatile PWAITINFO   gpWaitHead;
/** Pointer to the tail node of the wait status FIFO. */
static volatile PWAITINFO   gpWaitTail;
/** List of free wait nodes. Singly linked LIFO! */
static volatile PWAITINFO   gpWaitFree;
/** Number of free wait nodes. */
static volatile unsigned    gcWaitFree;
/** Preallocated wait nodes. These are used at signal time. */
static WAITINFO             gaWaitPreAlloced[128];
/** Index into the preallocated nodes. */
static volatile unsigned    giWaitPreAlloced;
/** List of known children. */
static volatile PWAITCHILD  gpChildrenHead;
/** List of free known children. */
static volatile PWAITCHILD  gpChildrenFree;
/** Number of known child processes. */
static volatile unsigned    gcChildren;
/** Total number of born child processes. (For statistical purposes only.) */
static volatile unsigned    gcBirths;
/** Total number of died child processes. */
static volatile unsigned    gcDeaths;

/** If this flag is set we don't care for wait status info.
 * This is something which the signal subsystem controls, it
 * will notify us when this state changes. */
static volatile unsigned    gfNoWaitStatus;

/** The event semaphore the callers are sleeping on. */
static volatile HEV         ghevWait;
/** The event semaphore the wait thread are sleeping on. */
static volatile HEV         ghevBirth;

/** Thread ID of the wait thread. */
static volatile TID         gtidThread;
/** Termination indicator. Used to help the wait thread terminate fast. */
static volatile unsigned    gfTerminate;


/*******************************************************************************
*   Internal Functions                                                         *
*******************************************************************************/
static void waitInit(void);
static int  waitChild(PWAITINFO pWait, int fNoWait, pid_t pidWait);
static int  waitAllocInsert(const PWAITINFO pWaitInsert);
static inline void waitInfoToSigInfo(const PWAITINFO pWait, siginfo_t *pSigInfo);
static int processWaitForkChildHook(__LIBC_PFORKHANDLE pForkHandle, __LIBC_FORKOP enmOperation);



_CRT_INIT1(waitInit)
_CRT_EXIT1(__libc_back_processWaitNotifyTerm)

/**
 * CRT Init callback function which initializes the
 * wait facilities.
 */
CRT_DATA_USED
static void waitInit(void)
{
    static void *pfnInited;
    if (!pfnInited)
    {
        /*
         * Create the fmutex.
         */
        _fmutex_checked_create2(&gmtxWait, _FMC_MUST_COMPLETE, "b_processWait.c: gmtxWait");
        int rc = DosCreateEventSemEx(NULL, (PHEV)&ghevWait, 0, TRUE);
        if (rc)
            abort();
        rc = DosCreateEventSemEx(NULL, (PHEV)&ghevBirth, 0, FALSE);
        if (rc)
            abort();


        /* done */
        pfnInited = (void *)waitInit;
    }
}

/**
 * Notify the process wait facilities that
 * the process wants to exit ASAP.
 *
 * This is called from the exitlist handler and other suitable places,
 * the _CRT_EXIT1() stuff isn't good enough alone.
 */
CRT_DATA_USED
void __libc_back_processWaitNotifyTerm(void)
{
    __atomic_xchg(&gfTerminate, 1);

    /* kill the thread - this is probably a waste of time in the exitlist handler... */
    if (gtidThread)
    {
        DosKillThread(gtidThread);
        gtidThread = 0;
    }

    /* destroy the birth event semaphore. */
    HEV hev = ghevBirth;
    ghevBirth = NULLHANDLE;
    if (hev)
    {
        DosPostEventSem(hev);
        DosCloseEventSem(hev);
    }

    /* destroy the wait event semaphore. */
    hev = ghevWait;
    ghevWait = NULLHANDLE;
    if (hev)
    {
        DosPostEventSem(hev);
        DosCloseEventSem(hev);
    }

#if 0 /* Don't massacre our decendants. */
    /*
     * Kill all known decendants.
     */
    /** @todo We should really reparent them to the init process, but only the session manager can do that. */
    PWAITCHILD pChild = gpChildrenHead;
    if (pChild)
    {
        gpChildrenHead = NULL;
        gcChildren = 0;

        /*
         * Kill the process tree so we don't end up having childrens
         * children popping in behind us.
         *
         * We boots our priority trying to make sure we get the message
         * down fast. Going back to regular shouldn't be problem since we're
         * doing exit lists now anyways.
         */
        DosSetPriority(PRTYS_THREAD, PRTYC_TIMECRITICAL, 1, 0);
        for (; pChild; pChild = pChild->pNext)
            DosKillProcess(DKP_PROCESSTREE, pChild->pid);
        DosSetPriority(PRTYS_THREAD, PRTYC_REGULAR, 0, 0);
        DosSleep(0);

        /*
         * We reap all possible decendants to as much as possible prevent anyone
         * waitin for immediate children to not be bothered with it but just get
         * the heck out and terminate.
         */
        RESULTCODES res;
        PID pid;
        while (!DosWaitChild(DCWA_PROCESSTREE, DCWW_NOWAIT, &res, &pid, 0))
            /* nothing */;
        DosSleep(1); /* penalty / chance to exit */
        while (!DosWaitChild(DCWA_PROCESSTREE, DCWW_NOWAIT, &res, &pid, 0))
            /* nothing */;
        DosSleep(0);
        while (!DosWaitChild(DCWA_PROCESSTREE, DCWW_NOWAIT, &res, &pid, 0))
            /* nothing */;
    }
#endif
}

/**
 * Request the wait mutex semaphore.
 *
 * Please note that we're also entering a must complete section to
 * avoid signal deadlocks.
 */
static int waitSemRequest(int fNoInterrupts)
{
    int rc = _fmutex_request(&gmtxWait, fNoInterrupts ? _FMR_IGNINT : 0);
    if (!rc)
        return 0;
    LIBC_ASSERTM_FAILED("_fmutex_request -> %d\n", rc);
    return -__libc_native2errno(rc);
}

/**
 * Release the wait mutex semaphore.
 */
static void waitSemRelease(void)
{
    int rc = _fmutex_release(&gmtxWait);
    LIBC_ASSERTM(!rc, "_fmutex_release -> %d\n", rc);
    rc = rc;
}


/**
 * Inserts node into the FIFO.
 * Caller owns semaphore.
 *
 * @param   pWait   Node to insert.
 */
static inline void waitInsertFIFO(PWAITINFO pWait)
{
    pWait->pNext = NULL;
    pWait->pPrev = gpWaitTail;
    if (pWait->pPrev)
        pWait->pPrev->pNext = pWait;
    else
        gpWaitHead = pWait;
    gpWaitTail = pWait;
}


/**
 * Wait thread.
 */
static void waitThread(void *pvIgnore)
{
    LIBCLOG_ENTER("pvIgnore=%p\n", pvIgnore);
    PPIB        pPib;
    PTIB        pTib;
    DosGetInfoBlocks(&pTib, &pPib);
    DosSetPriority(PRTYS_THREAD, PRTYC_REGULAR, 1, 0);

    /*
     * Wait loop.
     */
    int         fInternalTerm = 0;
    for (;!gfTerminate;)
    {
        /*
         * Check for exit condition.
         */
        fInternalTerm = gfTerminate;
        if (pPib->pib_flstatus & (0x40/*dying*/ | 0x04/*exiting all*/ | 0x02/*Exiting Thread 1*/ | 0x01/*ExitList */))
        {
            LIBCLOG_MSG("Terminating thread, exit conditions detected. pib_flstatus=%#lx\n",  pPib->pib_flstatus);
            fInternalTerm = 1;
        }

        /*
         * Wait for children.
         */
        WAITINFO Wait;
        int rc = waitChild(&Wait, fInternalTerm, 0 /* any child */);
        if (!rc)
        {
            /* enter semaphore protection. */
            rc = waitSemRequest(1);

            /*
             * Need we bother inserting it?
             */
            if (!gfNoWaitStatus)
                waitAllocInsert(&Wait);

            /*
             * Increment death number and decrement number of (known) children.
             */
            __atomic_increment(&gcDeaths);

            /*
             * Remove the child from the list.
             */
            PWAITCHILD pPrev = NULL;
            PWAITCHILD pChild = gpChildrenHead;
            while (pChild)
            {
                if (pChild->pid == Wait.pid)
                {
                    if (pPrev)
                        pPrev->pNext = pChild->pNext;
                    else
                        gpChildrenHead = pChild->pNext;
                    pChild->pNext = gpChildrenFree;
                    gpChildrenFree = pChild;
                    __atomic_decrement_min(&gcChildren, 0);
                    break;
                }

                /* next */
                pPrev = pChild;
                pChild = pChild->pNext;
            }

            /*
             * Wake up any waiters.
             */
            if (DosPostEventSem(ghevWait) == ERROR_INVALID_HANDLE)
                fInternalTerm = 1;

            /*
             * Leave semaphore protection and wait again unless someone's hinting it's time to quit...
             */
            if (!rc)
                waitSemRelease();
            if (fInternalTerm)
                break;
            continue;
        }

        /*
         * Wait for exec to occure.
         *
         * But before we do so, we'll check if we should get the out of here ASAP.
         * Even when we detect this exit condition, we'll do do one more iteration but
         * using the nowait option on the API call.
         */
        if (fInternalTerm || gfTerminate)
            break;
        if (pPib->pib_flstatus & (0x40/*dying*/ | 0x04/*exiting all*/ | 0x02/*Exiting Thread 1*/ | 0x01/*ExitList */))
        {
            LIBCLOG_MSG("Terminating thread, exit conditions detected. pib_flstatus=%#lx\n",  pPib->pib_flstatus);
            fInternalTerm = 1;
            continue;
        }

        if (rc == ERROR_WAIT_NO_CHILDREN)
        {
            DosWaitEventSem(ghevBirth, 5*1000);
            ULONG cIgnore;
            if (DosResetEventSem(ghevBirth, &cIgnore) == ERROR_INVALID_HANDLE)
                fInternalTerm = 1;
        }
        else
            LIBC_ASSERTM_FAILED("waitChild -> rc=%d expected ERROR_WAIT_NO_CHILDREN!\n", rc);
    }

    /*
     * Cleanup some stuff
     */
    gtidThread = 0;
    HEV hev = ghevWait;
    ghevWait = NULLHANDLE;
    DosPostEventSem(hev);
    LIBCLOG_RETURN_VOID();
}


/**
 * Wait for process(es) using the OS/2 API.
 * @returns 0 if child was added
 * @returns OS/2 error code (positive).
 * @returns Negative errno on semaphore failure. Child was reaped, but failed to insert anything.
 */
static int waitChild(PWAITINFO pWait, int fNoWait, pid_t pidWait)
{
    RESULTCODES resc;
    PID         pid;
    int rc = DosWaitChild(DCWA_PROCESS, fNoWait ? DCWW_NOWAIT : DCWW_WAIT, &resc, &pid, pidWait);
    if (rc)
        return rc;

    /*
     * Cool, we're got something on the hook. Let's see what
     * kind of fish it is.... dead. (doh)
     */
    LIBCLOG_MSG2("OS/2: Child %#lx (%ld) for reason %ld and with code %ld (%#lx).\n",
                 pid, pid, resc.codeTerminate, resc.codeResult, resc.codeResult);

    /* Fill in death certificate based on OS/2 info. */
    pWait->pid      = pid;
    switch (resc.codeTerminate)
    {
        case TC_EXIT:
            pWait->uCode    = CLD_EXITED;
            pWait->uStatus  = resc.codeResult;
            break;

        case TC_HARDERROR:
            pWait->uCode    = CLD_KILLED;
            pWait->uStatus  = SIGBUS;
            break;

        case TC_TRAP:
        case TC_EXCEPTION:
            pWait->uCode    = CLD_DUMPED;
            pWait->uStatus  = SIGSEGV;
            break;

        case TC_KILLPROCESS:
            pWait->uCode    = CLD_KILLED;
            pWait->uStatus  = SIGKILL;
            break;

        default:
            pWait->uCode    = CLD_KILLED;
            pWait->uStatus  = SIGSEGV;
            break;
    }

    /*
     * Reap it as a LIBC process, that might give more detail
     * (and besides it's deadly important to do it!)
     */
    __LIBC_SPMCHILDNOTIFY Notify = {NULL, sizeof(Notify), 0};
    if (!__libc_spmQueryChildNotification(pid, &Notify))
    {
        /*
         * LIBC fish, it (usually) contains more extensive details about
         * why it terminated.
         */
        LIBCLOG_MSG2("SPM Child %#lx (%ld) for reason %d and with code %d (%#x).\n",
                     pid, pid, Notify.enmDeathReason, Notify.iExitCode, Notify.iExitCode);
        pWait->pgrp = Notify.pgrp;

        switch (Notify.enmDeathReason)
        {
            case __LIBC_EXIT_REASON_EXIT:
                pWait->uCode    = CLD_EXITED;
                pWait->uStatus  = Notify.iExitCode;
                break;

            case __LIBC_EXIT_REASON_HARDERROR:
                pWait->uCode    = CLD_KILLED;
                pWait->uStatus  = SIGBUS;
                break;

            case __LIBC_EXIT_REASON_TRAP:
            case __LIBC_EXIT_REASON_XCPT:
                pWait->uCode    = CLD_DUMPED;
                pWait->uStatus  = SIGSEGV;
                break;

            case __LIBC_EXIT_REASON_KILL:
                pWait->uCode    = CLD_KILLED;
                pWait->uStatus  = SIGKILL;
                break;

            default:
                if (    Notify.enmDeathReason >= __LIBC_EXIT_REASON_SIGNAL_BASE
                    &&  Notify.enmDeathReason <= __LIBC_EXIT_REASON_SIGNAL_MAX)
                {
                    pWait->uCode    = CLD_KILLED;
                    pWait->uStatus  = Notify.enmDeathReason - __LIBC_EXIT_REASON_SIGNAL_BASE;
                    break;
                }
                /* fall thru */
            case __LIBC_EXIT_REASON_NONE:
                LIBCLOG_MSG2("Unknown death reason %d\n", Notify.enmDeathReason);
                break;
        }
    }

    /*
     * Raise signal.
     */
    siginfo_t SigInfo = {0};
    waitInfoToSigInfo(pWait, &SigInfo);
    SigInfo.si_flags = __LIBC_SI_QUEUED | __LIBC_SI_INTERNAL | __LIBC_SI_NO_NOTIFY_CHILD;

    rc = __libc_back_signalSemRequest();
    if (!rc)
    {
        rc = __libc_back_signalRaiseInternal(__libc_threadCurrent(), SIGCHLD, &SigInfo, NULL, __LIBC_BSRF_QUEUED | __LIBC_BSRF_EXTERNAL);
        LIBC_ASSERTM(rc >= 0, "failed raising SIGCHLD. rc=%d\n", rc);
        __libc_back_signalSemRelease();
    }

    return 0;
}


/**
 * Allocate and insert a wait node.
 */
static int waitAllocInsert(const PWAITINFO pWaitInsert)
{
    /*
     * Allocate info node.
     * This is ugly, because we don't wanna do too much inside the semaphore.
     */
    PWAITINFO pWait = NULL;
    if (gcWaitFree < sizeof(gaWaitPreAlloced) / sizeof(gaWaitPreAlloced[0]))
    {
        waitSemRelease();
        pWait = _hmalloc(sizeof(*pWait));
        int rc = waitSemRequest(1);
        if (rc)
        {
            free(pWait);
            return rc;
        }
    }
    if (!pWait)
    {
        pWait = gpWaitFree;
        if (pWait)
        {
            gcWaitFree--;
            gpWaitFree = pWait->pNext;
        }
        else
        {
            if (giWaitPreAlloced < sizeof(gaWaitPreAlloced) / sizeof(gaWaitPreAlloced[0]))
                pWait = &gaWaitPreAlloced[giWaitPreAlloced++];
        }
    }

    /*
     * Fill the node and then insert it.
     */
    if (pWait)
    {
        *pWait = *pWaitInsert;
        waitInsertFIFO(pWait);
        return 0;
    }
    return -ENOMEM;
}


/**
 * Checks whether the service has already been started.
 *
 * @return 1 if started, 0 if not.
 */
int __libc_back_processWaitNotifyAlreadyStarted(void)
{
    return gtidThread != 0;
}


/**
 * Notify the process wait facilities that
 * it's time to start collect child status codes.
 */
void __libc_back_processWaitNotifyExec(pid_t pid)
{
    /*
     * Allocate and enter semaphore protection.
     */
    PWAITCHILD pChild = NULL;
    if (gpChildrenFree)
    {
        waitSemRequest(0);
        pChild = gpChildrenFree;
        if (pChild)
            gpChildrenFree = pChild->pNext;
        else
            waitSemRelease();
    }
    if (!pChild)
    {
        pChild = _hmalloc(sizeof(*pChild));
        waitSemRequest(0);
    }

    /*
     * Tell wait thread that there is a child waiting.
     */
    if (pChild)
    {
        pChild->pid = pid;
        pChild->pNext = gpChildrenHead;
        gpChildrenHead = pChild;
    }
    __atomic_increment(&gcChildren);
    __atomic_increment(&gcBirths);
    int rc = DosPostEventSem(ghevBirth);
    LIBC_ASSERTM(!rc || rc == ERROR_ALREADY_POSTED, "DosPostEventSem(%#lx (birth)) -> %d\n", ghevBirth, rc);
    rc = rc;

    /*
     * No wait thread? Create one.
     */
    if (!gtidThread)
    {
        gfTerminate = 0;

        /*
         * Create the internal thread for dealing with waiting.
         */
        int tid = __libc_back_threadCreate(waitThread, 128*1024, NULL, 1);
        if (tid > 0)
        {
            gtidThread = tid;
            LIBCLOG_MSG2("Created waitThread! tid=%#x (%d)\n", tid, tid);
        }
        else
        {
            LIBC_ASSERTM_FAILED("Failed to create waitThread! rc=%d\n", tid);
            waitSemRelease();
            abort();
        }
    }

    waitSemRelease();
}


/**
 * Informs the wait facilities that a SIGCHLD signal action change have
 * action have modified the properties of the wait*() calls.
 *
 * If SIGCHLD action is either set to SIG_IGN or have the SA_NOCLDWAIT flag set
 * zombies will no longer be kept around. What happens for normal child exits are
 * simply that the wait selection get's empty and ECHILD is the correct reply.
 * In the case of stopped and continued children is a bit of a mystery. ECHILD
 * isn't the right one I think, it's also damn difficult to emulate. So, for now
 * those situations will not be handled properly and may cause deadlocks.
 *
 * @param   fNoWaitStatus       New child wait mode.
 */
void __libc_back_processWaitNotifyNoWait(int fNoWaitStatus)
{
    __atomic_xchg(&gfNoWaitStatus, fNoWaitStatus);
}


/**
 * Notify the process wait facilities that a child
 * process have posted a SIGCHLD signal to the parent.
 *
 * @param   pSigInfo    The signal info of the SIGCHLD signal.
 */
void __libc_back_processWaitNotifyChild(siginfo_t *pSigInfo)
{
    LIBCLOG_ENTER("pSigInfo=%p{si_code=%d si_status=%d si_pid=%#x si_pgrp=%#x}\n", (void *)pSigInfo,
                  pSigInfo->si_code, pSigInfo->si_status, pSigInfo->si_pid, pSigInfo->si_pgrp);

    int rc = waitSemRequest(1);
    if (!rc)
    {
        /*
         * Allocate node for it.
         */
        PWAITINFO pWait = gpWaitFree;
        if (pWait)
        {
            gcWaitFree--;
            gpWaitFree = pWait->pNext;
        }
        else
        {
            if (giWaitPreAlloced < sizeof(gaWaitPreAlloced) / sizeof(gaWaitPreAlloced[0]))
                pWait = &gaWaitPreAlloced[giWaitPreAlloced++];
        }
        if (pWait)
        {
            /* fill node */
            pWait->pid          = pSigInfo->si_pid;
            pWait->pgrp         = pSigInfo->si_pgrp;
            pWait->uCode        = pSigInfo->si_code;
            pWait->uStatus      = pSigInfo->si_status;
            pWait->uTimestamp   = pSigInfo->si_timestamp;
            pWait->uid          = pSigInfo->si_uid;

            /* insert it */
            waitInsertFIFO(pWait);
            DosPostEventSem(ghevWait);
        }
        else
            LIBCLOG_MSG("Failed to register child %d %#x %#x because of wait node shortage!\n",
                        pSigInfo->si_pid, pSigInfo->si_code, pSigInfo->si_status);
        waitSemRelease();
    }
    else
        LIBCLOG_MSG("Failed to register child %d %#x %#x because of semaphore rc=%d.\n",
                     pSigInfo->si_pid, pSigInfo->si_code, pSigInfo->si_status, rc);
    LIBCLOG_RETURN_VOID();
}


/**
 * Converts a wait node to a signal info structure.
 */
static inline void waitInfoToSigInfo(const PWAITINFO pWait, siginfo_t *pSigInfo)
{
    pSigInfo->si_signo      = SIGCHLD;
    pSigInfo->si_pid        = pWait->pid;
    pSigInfo->si_pgrp       = pWait->pgrp;
    pSigInfo->si_code       = pWait->uCode;
    pSigInfo->si_status     = pWait->uStatus;
    pSigInfo->si_timestamp  = pWait->uTimestamp;
    pSigInfo->si_uid        = pWait->uid;
    pSigInfo->si_flags      = __LIBC_SI_NO_NOTIFY_CHILD;
}


/**
 * Waits/polls for on one or more processes to change it's running status.
 *
 * @returns 0 on success, pSigInfo containing status info.
 * @returns Negated error code (errno.h) on failure.
 * @param   enmIdType   What kind of process specification Id contains.
 * @param   Id          Process specification of the enmIdType sort.
 * @param   pSigInfo    Where to store the result.
 * @param   fOptions    The WEXITED, WUNTRACED, WSTOPPED and WCONTINUED flags are used to
 *                      select the events to report. WNOHANG is used for preventing the api
 *                      from blocking. And WNOWAIT is used for peeking.
 * @param   pResUsage   Where to store the reported resources usage for the child.
 *                      Optional and not implemented on OS/2.
 */
int __libc_Back_processWait(idtype_t enmIdType, id_t Id, siginfo_t *pSigInfo, unsigned fOptions, struct rusage *pUsage)
{
    LIBCLOG_ENTER("enmIdType=%d Id=%#llx (%lld) pSigInfo=%p fOptions=%#x\n", enmIdType, Id, Id, (void *)pSigInfo, fOptions);

    /*
     * Validate options.
     */
    if (!fOptions & (WEXITED | WUNTRACED | WSTOPPED | WCONTINUED))
        LIBCLOG_ERROR_RETURN(-EINVAL, "ret -EINVAL - No event was selected! fOptions=%#x\n", fOptions);
    if (fOptions & ~(WEXITED | WUNTRACED | WSTOPPED | WCONTINUED | WNOHANG | WNOWAIT))
        LIBCLOG_ERROR_RETURN(-EINVAL, "ret -EINVAL - Unknown options %#x. (fOptions=%#x)\n",
                                 fOptions & ~(WEXITED | WUNTRACED | WSTOPPED | WCONTINUED | WNOHANG | WNOWAIT), fOptions);
    if (!pSigInfo)
        LIBCLOG_ERROR_RETURN(-EINVAL, "ret -EINVAL - pSigInfo is NULL.\n");
    switch (enmIdType)
    {
        case P_ALL:
            break;
        case P_PID:
            if (Id <= 0 || Id >= 0x7fffffff)
                LIBCLOG_ERROR_RETURN(-EINVAL, "ret -EINVAL - Invalid P_PID Id %lld\n", Id);
            break;
        case P_PGID:
            if (Id < 0 || Id >= 0x7fffffff)
                LIBCLOG_ERROR_RETURN(-EINVAL, "ret -EINVAL - Invalid P_PGID Id %lld\n", Id);
            break;
        default:
            LIBCLOG_ERROR_RETURN(-EINVAL, "ret -EINVAL - Invalid id type %d\n", enmIdType);
    }

    /*
     * Touch and zero the return structures.
     * (Better to crash soon than late.)
     */
    bzero(pSigInfo, sizeof(*pSigInfo));
    if (pUsage)
        bzero(pUsage, sizeof(*pUsage));

    /*
     * Wait loop.
     *
     * Inside the loop we own the wait semaphore most of the time.
     */
    FS_VAR();
    FS_SAVE_LOAD();
    int rc = waitSemRequest(0);
    if (rc)
        LIBCLOG_ERROR_RETURN_MSG(rc, "%d mutex\n", rc);
    unsigned    cIterations = 0;
    int         fInterrupted = 0;
    for (;; cIterations++)
    {
        /*
         * Examin the status list for anything which satisfies our specification.
         */
        WAITINFO    Wait;
        pid_t       pid = (pid_t)Id;
        if (!pid && enmIdType == P_PGID)
            pid = (pid_t)__libc_spmGetId(__LIBC_SPMID_PGRP);
        PWAITINFO   pInfo = gpWaitHead;
        while (pInfo)
        {
            /*
             * Check if it match the fOptions.
             */
            int fFlag;
            switch (pInfo->uCode)
            {
                case CLD_EXITED:
                case CLD_KILLED:
                case CLD_DUMPED:
                    fFlag = WEXITED;
                    break;
                #if 0 /* not implemented! */
                case CLD_TRAPPED:
                    break;
                #endif
                case CLD_STOPPED:
                    fFlag = WSTOPPED | WUNTRACED;
                    break;
                case CLD_CONTINUED:
                    fFlag = WCONTINUED;
                    break;
                default:
                    fFlag = 0;
                    break;
            }
            if (fFlag && (fFlag & fOptions))
            {
                if (enmIdType == P_ALL)
                    break;
                else if (enmIdType == P_PID)
                {
                    if (pInfo->pid == pid)
                        break;
                }
                else if (enmIdType == P_PGID && pInfo->pgrp == pid)
                    break;
            }

            /* next */
            pInfo = pInfo->pNext;
        }
        /* found something? */
        if (pInfo)
        {
            /*
             * We've found something! Prepare for return.
             */
            /* copy it */
            Wait = *pInfo;

            /* unlink it? */
            if (!(fOptions & WNOWAIT))
            {
                if (pInfo->pNext)
                    pInfo->pNext->pPrev = pInfo->pPrev;
                else
                    gpWaitTail = pInfo->pPrev;
                if (pInfo->pPrev)
                    pInfo->pPrev->pNext = pInfo->pNext;
                else
                    gpWaitHead = pInfo->pNext;
                /* free it */
                gcWaitFree++;
                pInfo->pNext = gpWaitFree;
                gpWaitFree = pInfo;
            }

            /* no need for semaphore any longer. */
            waitSemRelease();

            /* convert to siginfo */
            if (pSigInfo)
                waitInfoToSigInfo(&Wait, pSigInfo);
            /* return */
            rc = 0;
            break;
        }

        /*
         * Verify the wait id (again).
         *
         * Because of the design we cannot use DosWaitChild to check if
         * the process actually have any children. Which means we'll
         * have to use some child counting and thread states to check this.
         */
        if (enmIdType == P_ALL)
        {
            /*
             * It's enough to know that there are children around.
             */
            if (!gcChildren && !gpChildrenHead)
            {
                /** @todo Later we'll add support for waiting on processes spawned by direct calls to DosExecPgm.
                 * Should call DosWaitChild to be 99% sure. */
                waitSemRelease();
                rc = -ECHILD;
                break;
            }
        }
        else if (enmIdType == P_PID)
        {
            /*
             * In this case we'll have to make sure this PID exists
             * AND that it's a child of ours.
             */
            PWAITCHILD pChild = gpChildrenHead;
            for (;pChild; pChild = pChild->pNext)
                if (pChild->pid == (pid_t)Id)
                    break;
            if (!pChild)
            {
                /** @todo Later we'll add support for waiting on processes spawned by direct calls to DosExecPgm.
                 * Should call DosVerifyPidTid and DosGetPPid(). */
                waitSemRelease();
                rc = -ECHILD; /* See discussion in this function description and in waitid(). */
                break;
            }
        }
        else
        {
            rc = __libc_spmValidPGrp((pid_t)Id, 1 /* only children */);
            if (rc)
            {
                rc = __libc_spmValidPGrp((pid_t)Id, 0 /* check for group existance */);
                waitSemRelease();
                rc = -ECHILD; /* See discussion in this function description and in waitid(). */
                break;
            }
        }
        waitSemRelease();

        /*
         * If non-blocking we must return now.
         */
        if (fOptions & WNOHANG)
        {
            rc = 0;
            break;
        }

        /*
         * Delayed interrupted condition.
         */
        if (fInterrupted)
        {
            rc = -EINTR;
            break;
        }

        /*
         * Ok, we'll do some waiting.
         *
         * But first we'll check if this process is trying to terminate, because
         * if it is we'll not gonna hang along here anylonger. Only 2nd and
         * subsequent iterations => 30 second timeout before deadlock is fixed.
         */
        if (    cIterations >= 1
            &&  fibIsInExit())
            LIBCLOG_ERROR_RETURN_MSG(-EDEADLK, "%d (-EDEADLK) pib_flstatus=%#x\n", -EDEADLK, (unsigned)fibGetProcessStatus());
        rc = DosWaitEventSem(ghevWait, 30*1000);
        if (rc == ERROR_INTERRUPT)
            fInterrupted = 1;
        else if (rc && rc != ERROR_TIMEOUT && rc != ERROR_SEM_TIMEOUT)
            LIBCLOG_ERROR_RETURN_MSG(-EDEADLK, "%d (-DEADLK) waitsem -> rc=%d\n", -EDEADLK, rc);

        /*
         * Regain the semaphore and reset the semaphore if it was posted.
         */
        int rc2 = waitSemRequest(0);
        if (rc2)
            LIBCLOG_ERROR_RETURN_MSG(rc2, "%d mutex\n", rc2);
        if (!rc)
        {
            ULONG cIgnore;
            DosResetEventSem(ghevWait, &cIgnore);
        }
    } /* wait loop */

    /*
     * Done.
     */
    if (rc >= 0)
        LIBCLOG_RETURN_MSG(rc, "ret %d SigInfo.si_pid=%#x SigInfo.si_code=%#x SigInfo.si_status=%#x\n",
                           rc, pSigInfo->si_pid, pSigInfo->si_code, pSigInfo->si_status);
    LIBCLOG_ERROR_RETURN_MSG(rc, "ret %d SigInfo.si_pid=%#x SigInfo.si_code=%#x SigInfo.si_status=%#x\n",
                             rc, pSigInfo->si_pid, pSigInfo->si_code, pSigInfo->si_status);
}


_FORK_CHILD1(0xf0010000, processWaitForkChildHook)

/**
 * Callback function for registration using _FORK_PARENT1()
 * or _FORK_CHILD1().
 *
 * @returns 0 on success.
 * @returns positive errno on warning.
 * @returns negative errno on failure. Fork will be aborted.
 * @param   pForkHandle     Pointer to fork handle.
 * @param   enmOperation    Callback operation.
 */
static int processWaitForkChildHook(__LIBC_PFORKHANDLE pForkHandle, __LIBC_FORKOP enmOperation)
{
    LIBCLOG_ENTER("pForkHandle=%p enmOperation=%d\n", (void *)pForkHandle, enmOperation);

    switch (enmOperation)
    {
	/*
	 * Have to reset the globals to indicate no waiter thread.
	 */
	case __LIBC_FORK_OP_FORK_CHILD:
	{
            PWAITINFO pInfo = gpWaitHead;
            while (pInfo)
            {
                PWAITINFO p = pInfo;
                pInfo = pInfo->pNext;
                p->pNext = gpWaitFree;
                gpWaitFree = pInfo;
            }
            gpWaitHead      = NULL;
            gpWaitTail      = NULL;

            PWAITCHILD pChild = gpChildrenHead;
            if (pChild)
            {
                gpChildrenHead = NULL;
                pChild->pNext = gpChildrenFree;
                gpChildrenFree = pChild;
            }
            gcChildren      = 0;
            gcBirths        = 0;
            gcDeaths        = 0;
            //gfNoWaitStatus ???
            gtidThread      = 0;
            gfTerminate     = 0;
            break;
	}

	default:
	    break;
    }
    LIBCLOG_RETURN_INT(0);
}

