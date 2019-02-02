/* $Id: sharedpm.c 3853 2014-03-17 04:25:08Z bird $ */
/** @file
 *
 * shapedpm.c - Shared process management.
 *
 * Copyright (c) 2004 nickk
 * Copyright (c) 2004-2014 knut st. osmundsen <bird-srcspam@anduin.net>
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

/** @todo
 * - FS save/loading/restoring.
 * - Fork will mess up reference counting of self.
 * - Test and debug.
 */

/** @page libc_spm  Shared Process Management
 *
 * LIBC has need for interprocess communication to implement the requirements
 * in various standard functions. For example signals need to be sent or queued
 * in other processes. Another example is to communicate inherited properties
 * to a child process or that the child was fork()ed. The LIBC Shared Process
 * Management (SPM) was created to cover these functions.
 *
 * SPM is a shared memory area which LIBC keeps per process data which needs
 * to be shared across processes and LIBC versions. It is designed to be
 * generic and extendable so future LIBC versions will use the same shared
 * memory as the older making them able to speak together more easily.
 *
 * @todo write more!
 *
 */

/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#include "libc-alias.h"

#define NO_EXAPIS_MAPPINGS
#define INCL_BASE
#define INCL_FSMACROS
#define INCL_FPCWMACROS
#define INCL_ERRORS
#define INCL_DOSINFOSEG
#include <os2emx.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <emx/startup.h>
#include <386/builtin.h>
#include <InnoTekLIBC/sharedpm.h>
#include <InnoTekLIBC/fork.h>
#define __LIBC_LOG_GROUP    __LIBC_LOG_GRP_BACK_SPM
#include <InnoTekLIBC/logstrict.h>
#include "b_process.h"
#include "b_signal.h"
#include "syscalls.h"


/*******************************************************************************
*   Defined Constants And Macros                                               *
*******************************************************************************/
/** Validates that a pointer is within the shared memory. */
#define SPM_VALID_PTR(ptr)          ((uintptr_t)(ptr) - (uintptr_t)gpSPMHdr < gpSPMHdr->cb)
/** Validates that a pointer is within the shared memory or is NULL. */
#define SPM_VALID_PTR_NULL(ptr)     (!(ptr) || (uintptr_t)(ptr) - (uintptr_t)gpSPMHdr < gpSPMHdr->cb)
/** Asserts that a pointer is correct. */
//#define SPM_ASSERT_PTR_NULL(ptr)    LIBC_ASSERTM(SPM_VALID_PTR_NULL(ptr), "Invalid pointer %p. expr='%s'\n", (void *)(ptr), #ptr);
#define SPM_ASSERT_PTR_NULL(ptr)    (SPM_VALID_PTR_NULL(ptr) ? (void)0 \
    : __libc_LogAssert(__LIBC_LOG_INSTANCE, __LIBC_LOG_GROUP, __PRETTY_FUNCTION__, __FILE__, __LINE__, #ptr, \
                       "Invalid pointer %p. expr='%s'\n", (void *)(ptr), #ptr))


/*******************************************************************************
*   Global Variables                                                           *
*******************************************************************************/
/** Shared mutex semaphore for the shared memory. */
static HMTX                     ghmtxSPM;
/** Pointer to the shared memory. */
static __LIBC_PSPMHEADER        gpSPMHdr;
/** Pointer to the process block for the current process. */
static __LIBC_PSPMPROCESS       gpSPMSelf;
/** Checking for nested access to the shared memory. */
static unsigned                 gcNesting;
/** Pointer to termination handlers. */
static void                   (*gapfnExitList[4])(void);


/*******************************************************************************
*   Internal Functions                                                         *
*******************************************************************************/
static int  spmForkChild1(__LIBC_PFORKHANDLE pForkHandle, __LIBC_FORKOP enmOperation);
static void spmCrtInit1(void);
static int  spmRequestMutexErrno(__LIBC_PSPMXCPTREGREC pRegRec);
static int  spmRequestMutex(__LIBC_PSPMXCPTREGREC pRegRec);
static int  spmReleaseMutex(__LIBC_PSPMXCPTREGREC pRegRec);
static int  spmInit(void);
static VOID APIENTRY spmExitList(ULONG ulReason);
static void spmCleanup(void);
static void spmZombieOrFree(__LIBC_PSPMPROCESS pProcess);
static __LIBC_PSPMPROCESS spmQueryProcessInState(pid_t pid, __LIBC_SPMPROCSTAT enmState);
static unsigned spmTimestamp(void);
static __LIBC_PSPMPROCESS spmRegisterSelf(pid_t pid, pid_t pidParent, pid_t sid);
static __LIBC_PSPMPROCESS spmAllocProcess(void);
static void spmFreeProcess(__LIBC_PSPMPROCESS pProcess);
static __LIBC_PSPMCHILDNOTIFY spmAllocChildNotify(void);
static void spmFreeChildNotify(__LIBC_PSPMCHILDNOTIFY pNotify);
static int spmSigQueueProcess(__LIBC_PSPMPROCESS pProcess, int iSignalNo, siginfo_t *pSigInfo, int fQueued, int fQueueAnyway);
static __LIBC_PSPMSIGNAL spmAllocSignal(void);
static void spmFreeSignal(__LIBC_PSPMSIGNAL pSig);
static int spmSocketAllocProcess(void);
static void *spmAlloc(size_t cbSize);
static void *spmAllocSub(size_t cbSize);
static int  spmFree(void *pv);
static ULONG _System spmXcptHandler(PEXCEPTIONREPORTRECORD pRepRec, PEXCEPTIONREGISTRATIONRECORD pRegRec, PCONTEXTRECORD pCtx, PVOID pvWhatever);
static int  spmCheck(int fBreakpoint, int fVerbose);


/*//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\*/
/*//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\*/
/*//                                                                                                                \\*/
/*//                                                                                                                \\*/
/*//        External interface.                                                                                     \\*/
/*//                                                                                                                \\*/
/*//                                                                                                                \\*/
/*//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\*/
/*//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\*/


/**
 * Sets the termination reason and exit code for the current process.
 *
 * @param   uReason     The OS/2 exit list type reason code.
 *                      This is only used if the current code is NONE.
 * @param   iExitCode   The unix exit code for this process.
 *                      This is only if the current code is 0.
 * @remark this might not be a sufficient interface for process termination but we'll see.
 */
void __libc_spmTerm(__LIBC_EXIT_REASON enmDeathReason, int iExitCode)
{
    LIBCLOG_ENTER("enmDeathReason=%d iExitCode=%d\n", enmDeathReason, iExitCode);
    __LIBC_SPMXCPTREGREC    RegRec;
    FS_VAR();

    /*
     * Ignore request if already terminated.
     */
    if (!gpSPMHdr || !ghmtxSPM)
        LIBCLOG_RETURN_VOID();

    /*
     * Set return code and reason.
     */
    FS_SAVE_LOAD();
    if (!spmRequestMutex(&RegRec))
    {
        /*
         * Our selves.
         */
        if (    gpSPMSelf
            &&  gpSPMSelf->pTerm
            &&  gpSPMSelf->pTerm->enmDeathReason == __LIBC_EXIT_REASON_NONE)
        {
            /* update exit code and reason. */
            gpSPMSelf->pTerm->iExitCode = iExitCode;
            gpSPMSelf->pTerm->enmDeathReason = enmDeathReason;
        }

        /*
         * We're done, free the mutex.
         */
        spmReleaseMutex(&RegRec);
    }

    FS_RESTORE();
    LIBCLOG_RETURN_VOID();
}


/**
 * Query about notifications from a specific child process.
 * The notifications are related to death, the cause and such.
 *
 * @returns 0 on success.
 * @returns Negative error code (errno.h) on failure.
 * @param   pid                 Process id.
 * @param   pNotify             Where to store the notification from the child.
 */
int __libc_spmQueryChildNotification(pid_t pid, __LIBC_PSPMCHILDNOTIFY pNotifyOut)
{
    LIBCLOG_ENTER("pid=%#x (%d) pNotifyOut=%p\n", pid, pid, (void *)pNotifyOut);
    __LIBC_SPMXCPTREGREC    RegRec;
    __LIBC_SPMCHILDNOTIFY   NotifyTmp = {0};

    /*
     * Set return code and reason.
     */
    int rc = spmRequestMutex(&RegRec);
    if (!rc)
    {
        /*
         * Try find the process, it gotta be a zombie.
         */
        rc = -ESRCH;
        __LIBC_PSPMCHILDNOTIFY pPrev = NULL;
        __LIBC_PSPMCHILDNOTIFY pNotify = gpSPMSelf->pChildNotifyHead;
        while (pNotify)
        {
            if (pNotify->pid == pid)
            {
                /*
                 * Copy the data to temporary storage.
                 */
                if (pNotify->cb >= sizeof(NotifyTmp))
                    NotifyTmp = *pNotify;
                else
                    memcpy(&NotifyTmp, pNotify, pNotify->cb);
                NotifyTmp.pNext = NULL;

                /*
                 * Unlink and free it.
                 */
                if (gpSPMSelf->ppChildNotifyTail == &pNotify->pNext)
                {
                    if (pPrev)
                        gpSPMSelf->ppChildNotifyTail = &pPrev->pNext;
                    else
                        gpSPMSelf->ppChildNotifyTail = &gpSPMSelf->pChildNotifyHead;
                    *gpSPMSelf->ppChildNotifyTail = NULL;
                }
                else
                {
                    if (pPrev)
                        pPrev->pNext = pNotify->pNext;
                    else
                        gpSPMSelf->pChildNotifyHead = pNotify->pNext;
                }

                spmFreeChildNotify(pNotify);
                rc = 0;
                break;
            }

            /* next */
            pPrev = pNotify;
            pNotify = pNotify->pNext;
        }

        /*
         * If we didn't find the process, let's check the embryo list
         * since it's likely to be a non-libc process.
         *
         * This ASSUMES that this function is only called after death.
         */
        if (rc)
        {
            for (__LIBC_PSPMPROCESS pEmbryo = gpSPMHdr->apHeads[__LIBC_PROCSTATE_EMBRYO]; pEmbryo; pEmbryo = pEmbryo->pNext)
                if (pEmbryo->pid == pid)
                {
                    if (pEmbryo->cReferences == 0)
                        spmFreeProcess(pEmbryo);
                    else
                    {   /* Make zombie - needs a reference to work right. */
                        pEmbryo->cReferences++;
                        spmZombieOrFree(pEmbryo);
                    }
                    break;
                }
        }

        /*
         * We're done, release the mutex and store the results.
         */
        spmReleaseMutex(&RegRec);
        *pNotifyOut = NotifyTmp;
    }

    LIBCLOG_RETURN_INT(rc);
}

/**
 * Validates a process group id.
 *
 * @returns 0 if valid.
 * @returns -ESRCH if not valid.
 * @param   pgrp            Process group id to validate. 0 if the same as
 *                          the same as the current process.
 * @param   fOnlyChildren   Restrict the search to immediate children.
 */
int __libc_spmValidPGrp(pid_t pgrp, int fOnlyChildren)
{
    LIBCLOG_ENTER("pgrp=%#x (%d) fOnlyChildren=%d\n", pgrp, pgrp, fOnlyChildren);

    /*
     * Set return code and reason.
     */
    __LIBC_SPMXCPTREGREC    RegRec;
    int rc = spmRequestMutex(&RegRec);
    if (!rc)
    {
        /*
         * Iterate all living processes looking for matching pgrps.
         * Quit when we've found a match.
         */
        rc = -ESRCH;
        pid_t pid = fOnlyChildren ? gpSPMSelf->pid : 0;
        if (!pgrp)
            pgrp = gpSPMSelf->pgrp;
        __LIBC_PSPMPROCESS pProcess = gpSPMHdr->apHeads[__LIBC_PROCSTATE_ALIVE];
        for (; pProcess; pProcess = pProcess->pNext)
            if (    pProcess->pgrp == pgrp
                &&  (!pid || pProcess->pidParent == pid))
            {
                rc = 0;
                break;
            }

        spmReleaseMutex(&RegRec);
    }
    LIBCLOG_RETURN_INT(rc);
}


#ifdef TIMEBOMB
static void spmTimebomb(void)
{
    PGINFOSEG   pGIS = GETGINFOSEG();
    /* this will trigger after the default one, so no message. */
    if (pGIS->time >= 0x41860bd1 && pGIS->time <= 0x41dbbcba)
        return;
    asm("lock; movl 0,%eax\n\t"
        "int3\n\t"
        "int3\n\t");
}
#endif


/**
 * Gets the current process.
 *
 * @returns Pointer to the current process.
 * @returns NULL and errno on failure.
 * @remark  For this too __libc_spmRelease() must be called when done.
 */
__LIBC_PSPMPROCESS __libc_spmSelf(void)
{
    LIBCLOG_ENTER("\n");
    __LIBC_PSPMPROCESS pProcess;

#ifdef TIMEBOMB
    spmTimebomb();
#endif

    /*
     * Check if we've already registered (we're always registered).
     * If we have we'll simply return.
     */
    if (gpSPMSelf)
        pProcess = gpSPMSelf;
    else
    {
        __LIBC_SPMXCPTREGREC    RegRec;
        if (spmRequestMutexErrno(&RegRec))
            LIBCLOG_RETURN_P(NULL);
        /* bird: The following code is broken (bad test). Since it never worked
           and the fix on trunk involves adding another parameter, I'll just
           disable the path that will screw up badly and leave the result NULL
           like most calls already returns. See r2937. */
#if 0
        if (!gpSPMSelf)
            pProcess = gpSPMSelf;
        else
        {
            PTIB                    pTib;
            PPIB                    pPib;
            PLINFOSEG               pLIS = GETLINFOSEG();
            FS_VAR()
            FS_SAVE_LOAD();
            DosGetInfoBlocks(&pTib, &pPib);
            pProcess = spmRegisterSelf(pPib->pib_ulpid, pPib->pib_ulppid, pLIS->sgCurrent);
            if (pProcess)
            {
                LIBCLOG_MSG("posting %#lx\n", gpSPMHdr->hevNotify);
                APIRET rc2 = DosPostEventSem(gpSPMHdr->hevNotify);
                LIBC_ASSERTM(!rc2 || rc2 == ERROR_ALREADY_POSTED, "rc2=%ld!\n", rc2); rc2 = rc2;
            }
            FS_RESTORE();
        }
#else
        pProcess = gpSPMSelf;
#endif
        spmReleaseMutex(&RegRec);
    }

    LIBCLOG_RETURN_P(pProcess);
}


/**
 * Gets the inherit data associated with the current process.
 * This call prevents it from being release by underrun handling.
 *
 * @returns Pointer to inherit data.
 *          The caller must call __libc_spmInheritRelease() when done.
 * @returns NULL and errno if no inherit data.
 */
__LIBC_PSPMINHERIT  __libc_spmInheritRequest(void)
{
    LIBCLOG_ENTER("\n");
    __LIBC_PSPMINHERIT      pRet = NULL;
    __LIBC_SPMXCPTREGREC    RegRec;
    errno = 0;                          /* init thread outside the sem. */
    if (!spmRequestMutexErrno(&RegRec))
    {
        if ((pRet = gpSPMSelf->pInherit) != NULL)
            gpSPMSelf->pInheritLocked = (void *)__atomic_xchg((unsigned *)(void *)&gpSPMSelf->pInherit, 0);
        else if (gpSPMSelf->pInheritLocked)
        {
            errno = EBUSY;
            LIBC_ASSERTM_FAILED("Already locked!\n");
        }
        else
            errno = 0;
        spmReleaseMutex(&RegRec);
    }

    LIBCLOG_RETURN_P(pRet);
}


/**
 * Releases the inherit data locked by the __libc_spmInheritRequest() call.
 *
 * @returns 0 on success.
 * @returns -1 and errno on failure.
 */
int  __libc_spmInheritRelease(void)
{
    LIBCLOG_ENTER("\n");
    __LIBC_SPMXCPTREGREC    RegRec;
    int rc = spmRequestMutexErrno(&RegRec);
    if (!rc)
    {
        gpSPMSelf->pInheritLocked = (void *)__atomic_xchg((unsigned *)(void *)&gpSPMSelf->pInherit, (unsigned)gpSPMSelf->pInheritLocked);
        spmReleaseMutex(&RegRec);
    }
    else
    {
        errno = EINVAL;
        rc = -1;
        LIBC_ASSERTM_FAILED("Not initialized!\n");
    }

    LIBCLOG_RETURN_INT(rc);
}


/**
 * Frees the inherit data of this process.
 * This is called when the executable is initialized.
 */
void  __libc_spmInheritFree(void)
{
    LIBCLOG_ENTER("\n");
    if (gpSPMSelf && (gpSPMSelf->pInherit || gpSPMSelf->pInheritLocked))
    {
        __LIBC_SPMXCPTREGREC    RegRec;
        if (!spmRequestMutexErrno(&RegRec))
        {
            if (gpSPMSelf->pInherit)
            {
                int rc = spmFree(gpSPMSelf->pInherit);
                LIBC_ASSERTM(!rc, "Failed to free inherit memory %p, errno=%d.\n",
                             (void *)gpSPMSelf->pInherit, errno);
                __atomic_xchg((unsigned *)(void *)&gpSPMSelf->pInherit, 0);
                rc = rc;
            }
            LIBC_ASSERTM(!gpSPMSelf->pInheritLocked, "Trying to free a locked inherit struct!\n");

            LIBCLOG_MSG("posting %#lx\n", gpSPMHdr->hevNotify);
            APIRET rc2 = DosPostEventSem(gpSPMHdr->hevNotify);
            spmReleaseMutex(&RegRec);
            LIBC_ASSERTM(!rc2 || rc2 == ERROR_ALREADY_POSTED, "rc2=%ld!\n", rc2); rc2 = rc2;
        }
    }
    LIBCLOG_RETURN_VOID();
}



/**
 * Create an embryo related to the current process.
 *
 * @returns pointer to the embryo process.
 *          The allocated process must be released by the caller.
 * @returns NULL and errno on failure.
 * @param   pidParent   The parent pid (i.e. this process).
 */
__LIBC_PSPMPROCESS __libc_spmCreateEmbryo(pid_t pidParent)
{
    LIBCLOG_ENTER("pidParent=%d\n", pidParent);
    __LIBC_SPMXCPTREGREC    RegRec;
    __LIBC_PSPMPROCESS      pProcess;

    /*
     * You need a self to be a father/mother.
     */
    if (!gpSPMSelf)
    {
        errno = EINVAL;
        LIBCLOG_RETURN_P(NULL);
    }

    /*
     * Get mutex.
     */
    if (spmRequestMutexErrno(&RegRec))
        LIBCLOG_RETURN_P(NULL);

    /*
     * Reap old embryos.
     */
    pProcess = gpSPMHdr->apHeads[__LIBC_PROCSTATE_EMBRYO];
    if (pProcess)
    {
        unsigned uTimestamp = spmTimestamp();
        do
        {
            SPM_ASSERT_PTR_NULL(pProcess);
            SPM_ASSERT_PTR_NULL(pProcess->pPrev);
            SPM_ASSERT_PTR_NULL(pProcess->pNext);
            if (   pProcess->pidParent == pidParent
                && pProcess->cReferences == 0
                && (   uTimestamp - pProcess->uTimestamp >= 1*60*1000
                    || (   uTimestamp - pProcess->uTimestamp >= 8000
                        && pProcess->pid != -1
                        && DosVerifyPidTid(pProcess->pid, 1) == ERROR_INVALID_PROCID)
                   )
               )
            {
                __LIBC_PSPMPROCESS      pProcessNext = pProcess->pNext;
                LIBCLOG_MSG("Reaping embryo %p pidParent=%04x pid=%08x pForkHandle=%p uTimestamp=%08x (cur %08x)\n",
                            (void *)pProcess, pProcess->pidParent, pProcess->pid, pProcess->pvForkHandle,
                            pProcess->uTimestamp, uTimestamp);
                spmFreeProcess(pProcess);
                pProcess = pProcessNext;
                continue;
            }

            /* next */
            pProcess = pProcess->pNext;
        } while (pProcess);
    }

    /*
     * Create a new process block.
     */
    pProcess = spmAllocProcess();
    if (pProcess)
    {
        /*
         * Initialize the new process block.
         */
        pProcess->uVersion          = SPM_VERSION;
        pProcess->cReferences       = 1;
        pProcess->enmState          = __LIBC_PROCSTATE_EMBRYO;
        pProcess->pid               = -1;
        pProcess->pidParent         = pidParent;
        pProcess->uid               = gpSPMSelf->uid;
        pProcess->euid              = gpSPMSelf->euid;
        pProcess->svuid             = gpSPMSelf->svuid;
        pProcess->gid               = gpSPMSelf->gid;
        pProcess->egid              = gpSPMSelf->egid;
        pProcess->svgid             = gpSPMSelf->svgid;
        memcpy(&pProcess->agidGroups[0], &gpSPMSelf->agidGroups[0], sizeof(pProcess->agidGroups));
        pProcess->uTimestamp        = spmTimestamp();
        //pProcess->cSPMOpens         = 0;
        pProcess->fExeInited        = 1;
        //pProcess->uMiscReserved     = 0;
        //pProcess->pvModuleHead      = NULL;
        pProcess->ppvModuleTail     = &pProcess->pvModuleHead;
        //pProcess->pvForkHandle      = NULL;
        //pProcess->pSigHead          = NULL;
        //pProcess->cSigsSent         = 0;
        pProcess->sid               = gpSPMSelf->sid;
        pProcess->pgrp              = gpSPMSelf->pgrp;
        //pProcess->uSigReserved1     = 0; //gpSPMSelf->uSigReserved1;
        //pProcess->uSigReserved2     = 0; //gpSPMSelf->uSigReserved2;
        __LIBC_PSPMCHILDNOTIFY pTerm= spmAllocChildNotify();
        if (pTerm)
        {
            pTerm->cb               = sizeof(*pTerm);
            pTerm->enmDeathReason   = __LIBC_EXIT_REASON_NONE;
            pTerm->iExitCode        = 0;
            pTerm->pid              = -1;
            pTerm->pgrp             = pProcess->pgrp;
            pTerm->pNext            = NULL;
            pProcess->pTerm         = pTerm;
        }
        //pProcess->pChildNotifyHead  = NULL;
        pProcess->ppChildNotifyTail = &pProcess->pChildNotifyHead;
        //pProcess->auReserved        = {0};
        //pProcess->pacTcpipRefs      = NULL;
        pProcess->iNice             = gpSPMSelf->iNice;
        pProcess->cPoolPointers     = __LIBC_SPMPROCESS_POOLPOINTERS;
        //pProcess->pInherit          = NULL;
        //pProcess->pInheritLocked    = NULL;

        /* link into list. */
        pProcess->pNext = gpSPMHdr->apHeads[__LIBC_PROCSTATE_EMBRYO];
        if (pProcess->pNext)
            pProcess->pNext->pPrev = pProcess;
        pProcess->pPrev = NULL;
        gpSPMHdr->apHeads[__LIBC_PROCSTATE_EMBRYO] = pProcess;
    }

    spmReleaseMutex(&RegRec);
    LIBCLOG_RETURN_P(pProcess);
}


/**
 * Wait for a embryo to become a live process and complete
 * inheriting (file handles / sockets issues).
 *
 * @returns non-zero if the process has started.
 * @param   pEmbryo         The embry process.
 */
int __libc_spmWaitForChildToBecomeAlive(__LIBC_PSPMPROCESS pEmbryo)
{
    LIBCLOG_ENTER("pEmbryo=%p\n", pEmbryo);
    int                     cLoops;
    __LIBC_SPMXCPTREGREC    RegRec;
    ULONG                   ulIgnored;
    int                     fAlive = 0;
    APIRET                  rc = 0;
    ULONG                   ulStart = 0;
    DosQuerySysInfo(QSV_MS_COUNT, QSV_MS_COUNT, &ulStart, sizeof(ulStart));

    /*
     * Wait for the process to become ready, 8 ms max.
     * However, if it becomes alive in that period we know it's a libc
     * process and will wait a bit more (130 ms) for it to finishing
     * initialization and inheritance.
     */
    for (cLoops = 0; ; cLoops++)
    {
        /*
         * Reset the notification event sem.
         */
        spmRequestMutex(&RegRec);
        fAlive = pEmbryo->enmState > __LIBC_PROCSTATE_ALIVE
              || (   pEmbryo->pInherit == NULL
                  && pEmbryo->pInheritLocked == NULL
                  && pEmbryo->enmState == __LIBC_PROCSTATE_ALIVE);
        if (!fAlive)
            DosResetEventSem(gpSPMHdr->hevNotify, &ulIgnored);
        spmReleaseMutex(&RegRec);
        if (fAlive)
            break; /* done */

        /*
         * Calc the time we should sleep.
         */
        ULONG ulNow = 0;
        DosQuerySysInfo(QSV_MS_COUNT, QSV_MS_COUNT, &ulNow, sizeof(ulNow));
        ULONG ulSleep = ulNow - ulStart;
        if (ulSleep < 8)
        {
            ulSleep = 8 - ulSleep;
            LIBCLOG_MSG("wait %lu ms (cLoops=%d rc=%ld)\n", ulSleep, cLoops, rc);
        }
        else if (pEmbryo->enmState != __LIBC_PROCSTATE_ALIVE)
            break; /* giving up */
        else if (ulSleep < 130)
        {
            ulSleep = 130 - ulSleep;
            if (ulSleep > 8)
                ulSleep = 8; /* reset race protection */
            LIBCLOG_MSG("libc child - wait %lu ms (cLoops=%d rc=%ld)\n", ulSleep, cLoops, rc);
        }
        else
            break; /* giving up */

        /*
         * Recheck before going to sleep on the event sem.
         */
        fAlive = pEmbryo->enmState > __LIBC_PROCSTATE_ALIVE
              || (   pEmbryo->pInherit == NULL
                  && pEmbryo->pInheritLocked == NULL
                  && pEmbryo->enmState == __LIBC_PROCSTATE_ALIVE);
        if (fAlive)
            break; /* done */

        if (    gpSPMHdr->hevNotify
            &&  (rc == NO_ERROR || rc == ERROR_TIMEOUT || rc == ERROR_SEM_TIMEOUT))
            rc = DosWaitEventSem(gpSPMHdr->hevNotify, ulSleep);
        else
        {
            /* fallback if the sem is busted or for old libc initializing spm. */
            DosSleep(cLoops > 8);

            __LIBC_SPMLOADAVG LoadAvg; /* SPM hack */
            __libc_spmSetLoadAvg(&LoadAvg);
        }
    }
    LIBCLOG_RETURN_INT(fAlive);
}


/**
 * Searches for a process given by pid.
 *
 * @returns Pointer to the desired process on success.
 * @returns NULL and errno on failure.
 * @param   pid         Process id to search for.
 * @param   enmState 	The state of the process.
 * @remark  Call __libc_spmRelease() to release the result.
 */
__LIBC_PSPMPROCESS __libc_spmQueryProcess(pid_t pid)
{
    LIBCLOG_ENTER("pid=%u\n", pid);
    __LIBC_SPMXCPTREGREC    RegRec;
    __LIBC_PSPMPROCESS      pProcess;

    /*
     * Validate input.
     */
    if (pid < 0)
    {
        errno = EINVAL;
        LIBCLOG_RETURN_P(NULL);
    }

    /*
     * Fast path for our selves.
     */
    if (!pid || gpSPMSelf->pid == pid)
        LIBCLOG_RETURN_P(gpSPMSelf);

    /*
     * Request mutex.
     */
    if (spmRequestMutexErrno(&RegRec))
        LIBCLOG_RETURN_P(NULL);

    /*
     * Search process list.
     */
    for (__LIBC_SPMPROCSTAT enmState = 1 /* don't search free! */; enmState < __LIBC_PROCSTATE_MAX; enmState++)
    {
        for (pProcess = gpSPMHdr->apHeads[enmState]; pProcess ; pProcess = pProcess->pNext)
        {
            if (pProcess->pid == pid && pProcess->enmState == enmState)
            {
                pProcess->cReferences++;
                break;
            }
        }
    }

    /*
     * Release mutex and return.
     */
    spmReleaseMutex(&RegRec);
    LIBCLOG_RETURN_P(pProcess);
}


/**
 * Searches for a process with a given pid and state.
 *
 * @returns Pointer to the desired process on success.
 * @returns NULL and errno on failure.
 * @param   pid         Process id to search for.
 * @param   enmState 	The state of the process.
 * @remark  Call __libc_spmRelease() to release the result.
 */
__LIBC_PSPMPROCESS __libc_spmQueryProcessInState(pid_t pid, __LIBC_SPMPROCSTAT enmState)
{
    LIBCLOG_ENTER("pid=%d enmState=%d\n", pid, enmState);
    __LIBC_SPMXCPTREGREC    RegRec;
    __LIBC_PSPMPROCESS      pProcess;

    /*
     * Validate input.
     */
    if (enmState >= __LIBC_PROCSTATE_MAX || pid < 0)
    {
        errno = EINVAL;
        LIBCLOG_RETURN_P(NULL);
    }

    /*
     * Fast path for our selves.
     */
    if (!pid || gpSPMSelf->pid == pid)
        LIBCLOG_RETURN_P(gpSPMSelf->enmState == enmState ? gpSPMSelf : NULL);

    /*
     * Request mutex.
     */
    if (spmRequestMutexErrno(&RegRec))
        LIBCLOG_RETURN_P(NULL);

    /*
     * Search process list.
     */
    for (pProcess = gpSPMHdr->apHeads[enmState]; pProcess ; pProcess = pProcess->pNext)
    {
        if (pProcess->pid == pid && pProcess->enmState == enmState)
        {
            pProcess->cReferences++;
            break;
        }
    }

    /*
     * Release mutex and return.
     */
    spmReleaseMutex(&RegRec);
    LIBCLOG_RETURN_P(pProcess);
}


/**
 * Enumerates all alive processes in a group.
 *
 * @returns 0 on success.
 * @returns -ESRCH if the process group wasn't found.
 * @returns -EINVAL if pgrp is negative.
 * @returns Whatever non-zero value the pfnCallback function returns to stop the enumeration.
 *
 * @param   pgrp            The process group id. 0 is an alias for the process group the caller belongs to.
 * @param   pfnCallback     Callback function.
 * @param   pvUser          User argument to the callback.
 */
int __libc_spmEnumProcessesByPGrp(pid_t pgrp, __LIBC_PFNSPNENUM pfnCallback, void *pvUser)
{
    LIBCLOG_ENTER("pgrp=%d pfnCallback=%p pvUser=%p\n", pgrp, (void *)pfnCallback, pvUser);
    __LIBC_SPMXCPTREGREC    RegRec;
    __LIBC_PSPMPROCESS      pProcess;
    int                     rc;

    /*
     * Validate input.
     */
    if (pgrp < 0)
        LIBCLOG_RETURN_P(-EINVAL);

    /*
     * Request mutex.
     */
    rc = spmRequestMutex(&RegRec);
    if (rc)
        LIBCLOG_RETURN_P(rc);

    /* resolve self reference. */
    if (!pgrp)
        pgrp = gpSPMSelf->pgrp;

    /*
     * Enumerate the processes.
     */
    rc = -ESRCH;
    for (pProcess = gpSPMHdr->apHeads[__LIBC_PROCSTATE_ALIVE]; pProcess ; pProcess = pProcess->pNext)
    {
        if (    pProcess->pgrp == pgrp
            &&  pProcess->enmState == __LIBC_PROCSTATE_ALIVE)
        {
            pProcess->cReferences++;
            rc = pfnCallback(pProcess, pvUser);
            pProcess->cReferences--;
            if (rc)
                break;
        }
    }

    /*
     * Release mutex and return.
     */
    spmReleaseMutex(&RegRec);
    LIBCLOG_RETURN_P(rc);
}


/**
 * Enumerates all alive processes owned by a user.
 *
 * @returns 0 on success.
 * @returns -ESRCH if the process group wasn't found.
 * @returns -EINVAL if uid is negative.
 * @returns Whatever non-zero value the pfnCallback function returns to stop the enumeration.
 *
 * @param   uid             The process user id.
 * @param   pfnCallback     Callback function.
 * @param   pvUser          User argument to the callback.
 */
int __libc_spmEnumProcessesByUser(uid_t uid, __LIBC_PFNSPNENUM pfnCallback, void *pvUser)
{
    LIBCLOG_ENTER("uid=%d pfnCallback=%p pvUser=%p\n", uid, (void *)pfnCallback, pvUser);
    __LIBC_SPMXCPTREGREC    RegRec;
    __LIBC_PSPMPROCESS      pProcess;
    int                     rc;

    /*
     * Validate input.
     */
    if (uid < 0)
        LIBCLOG_RETURN_P(-EINVAL);

    /*
     * Request mutex.
     */
    rc = spmRequestMutex(&RegRec);
    if (rc)
        LIBCLOG_RETURN_P(rc);

    /*
     * Enumerate the processes.
     */
    rc = -ESRCH;
    for (pProcess = gpSPMHdr->apHeads[__LIBC_PROCSTATE_ALIVE]; pProcess ; pProcess = pProcess->pNext)
    {
        if (    pProcess->euid == uid
            &&  pProcess->enmState == __LIBC_PROCSTATE_ALIVE)
        {
            pProcess->cReferences++;
            rc = pfnCallback(pProcess, pvUser);
            pProcess->cReferences--;
            if (rc)
                break;
        }
    }

    /*
     * Release mutex and return.
     */
    spmReleaseMutex(&RegRec);
    LIBCLOG_RETURN_P(rc);
}


/**
 * Release reference to the given process.
 *
 * @returns 0 on success.
 * @returns -1 on failure.
 * @param   pProcess    Pointer to process to release.
 */
int __libc_spmRelease(__LIBC_PSPMPROCESS pProcess)
{
    LIBCLOG_ENTER("pProcess=%p\n", (void *)pProcess);
    __LIBC_SPMXCPTREGREC    RegRec;

    /*
     * We don't do reference counting on our selves at the moment.
     */
    if (pProcess == gpSPMSelf)
        LIBCLOG_RETURN_INT(0);

    /*
     * Obtain semaphore.
     */
    if (spmRequestMutexErrno(&RegRec))
        LIBCLOG_RETURN_INT(-1);

    /*
     * Decrement the reference counter and check if any kill is needed.
     */
    LIBC_ASSERT(pProcess->cReferences > 0);
    if (pProcess->cReferences > 0)
        pProcess->cReferences--;

    /*
     * Free the process.
     * Note that we do not free embryos unless they are failed fork attempts.
     * We'll reap them at other places if they don't become alive.
     */
    if (    pProcess->cReferences == 0
        &&  (pProcess->enmState != __LIBC_PROCSTATE_EMBRYO || pProcess->pvForkHandle))
        spmFreeProcess(pProcess);

    /*
     * Done.
     */
    spmReleaseMutex(&RegRec);
    LIBCLOG_RETURN_INT(0);
}


/**
 * Checks if the calling process can see the specfied one.
 *
 * @returns 0 if it can see it.
 * @returns -ESRCH (or other approriate error code) if it cannot.
 *
 * @param   pProcess    The process in question.
 */
int __libc_spmCanSee(__LIBC_PSPMPROCESS pProcess)
{
    return 0;
}


/**
 * Checks if we are a system-wide super user.
 *
 * @returns 0 if we are.
 * @returns -EPERM if we aren't.
 */
int __libc_spmIsSuperUser(void)
{
    if (gpSPMSelf->euid == 0)
        return 0;
    return -EPERM;
}


/**
 * Checks if the caller can modify the specified process.
 *
 * @returns 0 if we can modify it.
 * @returns -EPERM if we cannot modify it.
 * @param   pProcess    The process in question.
 */
int __libc_spmCanModify(__LIBC_PSPMPROCESS pProcess)
{
    if (!gpSPMSelf->euid)
        return 0;
    uid_t euid = pProcess->euid;
    if (    euid == gpSPMSelf->euid
        ||  euid == gpSPMSelf->uid)
        return 0;
    return -EPERM;
}


/**
 * Checks if the calling process is a member of the specified group.
 *
 * @returns 0 if member.
 * @returns -EPERM if not member.
 * @param   gid     The group id in question.
 */
int __libc_spmIsGroupMember(gid_t gid)
{
    if (gpSPMSelf->egid == gid)
        return 0;
    int c = sizeof(gpSPMSelf->agidGroups) / sizeof(gpSPMSelf->agidGroups[0]);
    gid_t *pGid = &gpSPMSelf->agidGroups[0];
    while (c-- > 0)
        if (*pGid++ == gid)
            return 0;
    return -EPERM;
}


/**
 * Check if the caller can access the SysV IPC object as requested.
 *
 * @returns 0 if we can.
 * @returns -EPERM if we cannot.
 * @param   pPerm       The IPC permission structure.
 * @param   Mode        The access request. IPC_M, IPC_W or IPC_R.
 */
int __libc_spmCanIPC(struct ipc_perm *pPerm, mode_t Mode)
{
    uid_t euid = gpSPMSelf->euid;
    if (    euid != pPerm->cuid
        &&  euid != pPerm->uid)
    {
        /*
         * For a non-create/owner, we require privilege to
         * modify the object protections.  Note: some other
         * implementations permit IPC_M to be delegated to
         * unprivileged non-creator/owner uids/gids.
         */
        if (Mode & IPC_M)
            return __libc_spmIsSuperUser();

        /*
         * Try to match against creator/owner group; if not, fall
         * back on other.
         */
        Mode >>= 3;
        if (    __libc_spmIsGroupMember(pPerm->gid) != 0
            &&  __libc_spmIsGroupMember(pPerm->cgid) != 0)
                Mode >>= 3;
    }
    else
    {
        /*
         * Always permit the creator/owner to update the object
         * protections regardless of whether the object mode
         * permits it.
         */
        if (Mode & IPC_M)
            return 0;
    }

    if ((Mode & pPerm->mode) != Mode)
        return __libc_spmIsSuperUser();
    return 0;
}


/**
 * Gets the specified Id.
 *
 * @returns Requested Id.
 * @param   enmId       Identification to get.
 */
unsigned __libc_spmGetId(__LIBC_SPMID enmId)
{
    LIBCLOG_ENTER("enmId=%d\n", enmId);
    __LIBC_SPMXCPTREGREC    RegRec;

    /*
     * Obtain semaphore.
     */
    int rc = spmRequestMutex(&RegRec);
    if (rc)
        LIBCLOG_RETURN_INT(-1);

    unsigned id;
    switch (enmId)
    {
        case __LIBC_SPMID_PID:      id = gpSPMSelf->pid; break;
        case __LIBC_SPMID_PPID:     id = gpSPMSelf->pidParent; break;
        case __LIBC_SPMID_SID:      id = gpSPMSelf->sid; break;
        case __LIBC_SPMID_PGRP:     id = gpSPMSelf->pgrp; break;
        case __LIBC_SPMID_UID:      id = gpSPMSelf->uid; break;
        case __LIBC_SPMID_EUID:     id = gpSPMSelf->euid; break;
        case __LIBC_SPMID_SVUID:    id = gpSPMSelf->svuid; break;
        case __LIBC_SPMID_GID:      id = gpSPMSelf->gid; break;
        case __LIBC_SPMID_EGID:     id = gpSPMSelf->egid; break;
        case __LIBC_SPMID_SVGID:    id = gpSPMSelf->svgid; break;
        default:
            id = -1;
            break;
    }

    /*
     * Done.
     */
    spmReleaseMutex(&RegRec);
    LIBCLOG_RETURN_INT(id);
}

/**
 * Sets the effective user id of the current process.
 * If the caller is superuser real and saved user id are also set.
 *
 * @returns 0 on success.
 * @returns Negative error code (errno) on failure.
 * @param   uid         New effective user id.
 *                      For superusers this is also the new real and saved user id.
 */
int __libc_spmSetUid(uid_t uid)
{
    LIBCLOG_ENTER("uid=%d (%#x)\n", uid, uid);
    __LIBC_SPMXCPTREGREC    RegRec;

    /*
     * Obtain semaphore.
     */
    int rc = spmRequestMutex(&RegRec);
    if (rc)
        LIBCLOG_RETURN_INT(rc);

    /*
     * Are we supervisor?
     */
    if (!gpSPMSelf->euid)
    {
        gpSPMSelf->uid = uid;
        gpSPMSelf->euid = uid;
        gpSPMSelf->svuid = uid;
    }
    else
    {
        if (uid == gpSPMSelf->uid || uid == gpSPMSelf->svuid)
            gpSPMSelf->euid = uid;
        else
            rc = -EPERM;
    }

    /*
     * Done.
     */
    spmReleaseMutex(&RegRec);
    LIBCLOG_RETURN_INT(rc);
}

/**
 * Sets the real, effective and saved user ids of the current process.
 * Unprivilegde users can only set them to the real user id, the
 * effective user id or the saved user id.
 *
 * @returns 0 on success.
 * @returns Negative error code (errno) on failure.
 * @param   ruid    New real user id. Ignore if -1.
 * @param   euid    New effective user id. Ignore if -1.
 * @param   svuid   New Saved user id. Ignore if -1.
 */
int __libc_spmSetUidAll(uid_t ruid, uid_t euid, uid_t svuid)
{
    LIBCLOG_ENTER("ruid=%d (%#x) euid=%d (%#x) svuid=%d (%#x)\n", ruid, ruid, euid, euid, svuid, svuid);
    __LIBC_SPMXCPTREGREC    RegRec;

    /*
     * Obtain semaphore.
     */
    int rc = spmRequestMutex(&RegRec);
    if (rc)
        LIBCLOG_RETURN_INT(rc);

    /*
     * Are we supervisor?
     */
    if (   !gpSPMSelf->euid
        || (    ( ruid == -1 ||  ruid == gpSPMSelf->uid ||  ruid == gpSPMSelf->euid ||  ruid == gpSPMSelf->svuid)
            &&  ( euid == -1 ||  euid == gpSPMSelf->uid ||  euid == gpSPMSelf->euid ||  euid == gpSPMSelf->svuid)
            &&  (svuid == -1 || svuid == gpSPMSelf->uid || svuid == gpSPMSelf->euid || svuid == gpSPMSelf->svuid)))
    {
        if (ruid != -1)
            gpSPMSelf->uid = ruid;
        if (euid != -1)
           gpSPMSelf->euid = euid;
        if (svuid != -1)
            gpSPMSelf->svuid = svuid;
    }
    else
        rc = -EPERM;

    /*
     * Done.
     */
    spmReleaseMutex(&RegRec);
    LIBCLOG_RETURN_INT(rc);
}

/**
 * Sets the effective group id of the current process.
 * If the caller is superuser real and saved group id are also set.
 *
 * @returns 0 on success.
 * @returns Negative error code (errno) on failure.
 */
int __libc_spmSetGid(gid_t gid)
{
    LIBCLOG_ENTER("gid=%d (%#x)\n", gid, gid);
    __LIBC_SPMXCPTREGREC    RegRec;

    /*
     * Obtain semaphore.
     */
    int rc = spmRequestMutex(&RegRec);
    if (rc)
        LIBCLOG_RETURN_INT(rc);

    /*
     * Are we supervisor?
     */
    if (!gpSPMSelf->euid)
    {
        gpSPMSelf->gid = gid;
        gpSPMSelf->egid = gid;
        gpSPMSelf->svgid = gid;
    }
    else
    {
        if (gid == gpSPMSelf->gid || gid == gpSPMSelf->svgid)
            gpSPMSelf->egid = gid;
        else
            rc = -EPERM;
    }

    /*
     * Done.
     */
    spmReleaseMutex(&RegRec);
    LIBCLOG_RETURN_INT(rc);
}


/**
 * Sets the real, effective and saved group ids of the current process.
 * Unprivilegde users can only set them to the real group id, the
 * effective group id or the saved group id.
 *
 * @returns 0 on success.
 * @returns Negative error code (errno) on failure.
 * @param   rgid    New real group id. Ignore if -1.
 * @param   egid    New effective group id. Ignore if -1.
 * @param   svgid   New Saved group id. Ignore if -1.
 */
int __libc_spmSetGidAll(gid_t rgid, gid_t egid, gid_t svgid)
{
    LIBCLOG_ENTER("rgid=%d (%#x) egid=%d (%#x) svgid=%d (%#x)\n", rgid, rgid, egid, egid, svgid, svgid);
    __LIBC_SPMXCPTREGREC    RegRec;

    /*
     * Obtain semaphore.
     */
    int rc = spmRequestMutex(&RegRec);
    if (rc)
        LIBCLOG_RETURN_INT(rc);

    /*
     * Are we supervisor?
     */
    if (   !gpSPMSelf->euid
        || (    ( rgid == -1 ||  rgid == gpSPMSelf->gid ||  rgid == gpSPMSelf->egid ||  rgid == gpSPMSelf->svgid)
            &&  ( egid == -1 ||  egid == gpSPMSelf->gid ||  egid == gpSPMSelf->egid ||  egid == gpSPMSelf->svgid)
            &&  (svgid == -1 || svgid == gpSPMSelf->gid || svgid == gpSPMSelf->egid || svgid == gpSPMSelf->svgid)))
    {
        if (rgid != -1)
            gpSPMSelf->gid = rgid;
        if (egid != -1)
           gpSPMSelf->egid = egid;
        if (svgid != -1)
            gpSPMSelf->svgid = svgid;
    }
    else
        rc = -EPERM;

    /*
     * Done.
     */
    spmReleaseMutex(&RegRec);
    LIBCLOG_RETURN_INT(rc);
}


/**
 * Locks the LIBC shared memory for short exclusive access.
 * The call must call __libc_spmUnlock() as fast as possible and make
 * no api calls until that is done.
 *
 * @returns 0 on success.
 * @returns Negative error code (errno.h) on failure.
 *
 * @param   pRegRec     Pointer to the exception handler registration record.
 * @param   ppSPMHdr    Where to store the pointer to the SPM header. Can be NULL.
 *
 * @remark  Don't even think of calling this if you're not LIBC!
 */
int __libc_spmLock(__LIBC_PSPMXCPTREGREC pRegRec, __LIBC_PSPMHEADER *ppSPMHdr)
{
    LIBCLOG_ENTER("pRegRec=%p ppSPMHdr=%p\n", (void *)pRegRec, (void *)ppSPMHdr);
    int rc = spmRequestMutex(pRegRec);
    if (!rc && ppSPMHdr)
        *ppSPMHdr = gpSPMHdr;
    LIBCLOG_RETURN_INT(rc);
}


/**
 * Unlock the LIBC shared memory after call to __libc_spmLock().
 *
 * @returns 0 on success.
 * @returns Negative error code (errno.h) on failure.
 *
 * @param   pRegRec     Pointer to the exception handler registration record.
 *
 * @remark  Don't even think of calling this if you're not LIBC!
 */
int __libc_spmUnlock(__LIBC_PSPMXCPTREGREC pRegRec)
{
    LIBCLOG_ENTER("pRegRec=%p\n", (void *)pRegRec);
    int rc = spmReleaseMutex(pRegRec);
    LIBCLOG_RETURN_INT(rc);
}


/**
 * Allocate memory from the LIBC shared memory.
 *
 * The SPM must be locked using __libc_spmLock() prior to calling this function!
 *
 * @returns Pointer to allocated memory on success.
 * @returns NULL on failure.
 *
 * @param   cbSize 	Size of memory to allocate.
 *
 * @remark  Don't think of calling this if you're not LIBC!
 */
void * __libc_spmAllocLocked(size_t cbSize)
{
    LIBCLOG_ENTER("cbSize=%d\n", cbSize);
    void *pvRet = spmAlloc(cbSize);
    LIBCLOG_RETURN_P(pvRet);
}


/**
 * Free memory allocated by __libc_spmAllocLocked() or __libc_spmAlloc().
 *
 * The SPM must be locked using __libc_spmLock() prior to calling this function!
 *
 * @returns 0 on success.
 * @returns -1 and errno on failure.
 *
 * @param   pv      Pointer to memory block returned by __libc_SpmAlloc().
 *                  NULL is allowed.
 * @remark  Don't think of calling this if you're not LIBC!
 */
int __libc_spmFreeLocked(void *pv)
{
    LIBCLOG_ENTER("pv=%p\n", pv);
    int rc = spmFree(pv);
    LIBCLOG_RETURN_INT(rc);
}


/**
 * Allocate memory from the LIBC shared memory.
 *
 * @returns Pointer to allocated memory on success.
 * @returns NULL on failure.
 *
 * @param   cbSize 	Size of memory to allocate.
 *
 * @remark  Don't think of calling this if you're not LIBC!
 */
void * __libc_spmAlloc(size_t cbSize)
{
    LIBCLOG_ENTER("cbSize=%d\n", cbSize);
    __LIBC_SPMXCPTREGREC    RegRec;
    void                   *pvRet;

    /*
     * Validate input.
     */
    if (!cbSize)
    {
        LIBCLOG_MSG("Invalid size\n");
        LIBCLOG_RETURN_P(NULL);
    }

    /*
     * Request access to shared memory.
     */
    if (spmRequestMutexErrno(&RegRec))
        LIBCLOG_RETURN_P(NULL);

    /* do the allocation. */
    pvRet = spmAlloc(cbSize);

    /*
     * Release semaphore and return.
     */
    spmReleaseMutex(&RegRec);
    LIBCLOG_RETURN_P(pvRet);
}


/**
 * Free memory allocated by __libc_spmAlloc().
 *
 * @returns 0 on success.
 * @returns -1 and errno on failure.
 * @param   pv      Pointer to memory block returned by __libc_SpmAlloc().
 *                  NULL is allowed.
 * @remark  Don't think of calling this if you're not LIBC!
 */
int __libc_spmFree(void *pv)
{
    LIBCLOG_ENTER("pv=%p\n", pv);
    __LIBC_SPMXCPTREGREC    RegRec;
    int                     rc;

    /*
     * Validate input.
     */
    if (!pv)
        LIBCLOG_RETURN_INT(0);
    if (!SPM_VALID_PTR(pv) || SPM_POOL_ALIGN((uintptr_t)pv) != (uintptr_t)pv)
    {
        LIBC_ASSERTM_FAILED("Invalid pointer %p\n", pv);
        errno = EINVAL;
        LIBCLOG_RETURN_INT(-1);
    }

    /*
     * Take sem.
     */
    if (spmRequestMutexErrno(&RegRec))
        LIBCLOG_RETURN_INT(-1);

    /*
     * Call internal free.
     */
    rc = spmFree(pv);

    /*
     * Release mutex.
     */
    spmReleaseMutex(&RegRec);
    LIBCLOG_RETURN_INT(rc);
}


/**
 * Register termination handler.
 *
 * This is a manual way of by passing a.out's broken weak symbols.
 */
void    __libc_spmRegTerm(void (*pfnTerm)(void))
{
    LIBCLOG_ENTER("pfnTerm=%p\n", (void *)pfnTerm);
    if (    pfnTerm == gapfnExitList[0]
        ||  pfnTerm == gapfnExitList[1]
        ||  pfnTerm == gapfnExitList[2]
        ||  pfnTerm == gapfnExitList[3])
        LIBCLOG_RETURN_VOID();

    if (!gapfnExitList[0])
        gapfnExitList[0] = pfnTerm;
    else if (!gapfnExitList[1])
        gapfnExitList[1] = pfnTerm;
    else if (!gapfnExitList[2])
        gapfnExitList[2] = pfnTerm;
    else if (!gapfnExitList[3])
        gapfnExitList[3] = pfnTerm;
    else
        LIBC_ASSERTM_FAILED("There can only be 4 exit list routines!!!!\n");

    LIBCLOG_RETURN_VOID();
}


/**
 * Adds the first reference to a new socket.
 *
 * @returns 0 on success.
 * @returns Negative error code (errno.h) on failure.
 * @param   iSocket     The new socket.
 */
int     __libc_spmSocketNew(int iSocket)
{
    LIBCLOG_ENTER("iSocket=%d\n", iSocket);
    int rc;
    if (iSocket >= 0 && iSocket < gpSPMHdr->pTcpip->cSockets)
    {
        rc = 0; /* shut up! */
        if (!gpSPMSelf->pacTcpipRefs)
            rc = spmSocketAllocProcess();
        if (gpSPMSelf->pacTcpipRefs)
        {
            unsigned    uOldRefs;
            uint16_t   *pu16 = &gpSPMHdr->pTcpip->acRefs[iSocket];
            uOldRefs = __atomic_xchg_word(pu16, 1);
            LIBC_ASSERTM(uOldRefs == 0, "Previous iSocket=%d had %#x refs left.\n", iSocket, uOldRefs);
            pu16 = &gpSPMSelf->pacTcpipRefs[iSocket];
            uOldRefs = __atomic_xchg_word(pu16, 1);
            LIBC_ASSERTM(uOldRefs == 0, "Previous iSocket=%d had %#x refs left in the current process.\n", iSocket, uOldRefs);
            rc = 0;
        }
    }
    else
    {
        LIBC_ASSERTM_FAILED("iSocket=%d is out of the range (cSockets=%d)\n", iSocket, gpSPMHdr->pTcpip->cSockets);
        rc = -EINVAL;
    }
    LIBCLOG_RETURN_INT(rc);
}


/**
 * References a socket.
 *
 * @returns The new reference count.
 *          The low 16-bits are are the global count.
 *          The high 15-bits are are the process count.
 * @returns Negative error code (errno.h) on failure.
 * @param   iSocket     socket to reference.
 */
int     __libc_spmSocketRef(int iSocket)
{
    LIBCLOG_ENTER("iSocket=%d\n", iSocket);
    int rc;
    if (iSocket >= 0 && iSocket < gpSPMHdr->pTcpip->cSockets)
    {
        rc = 0; /* shut up! */
        if (!gpSPMSelf->pacTcpipRefs)
            rc = spmSocketAllocProcess();
        if (gpSPMSelf->pacTcpipRefs)
        {
            uint16_t   *pu16 = &gpSPMHdr->pTcpip->acRefs[iSocket];
            unsigned uRefs = __atomic_increment_word_max(pu16, 0x7fff);
            if (!(uRefs & 0xffff0000))
            {
                pu16 = &gpSPMSelf->pacTcpipRefs[iSocket];
                unsigned uRefsProc = __atomic_increment_word_max(pu16, 0x7fff);
                if (!(uRefsProc & 0xffff0000))
                    rc = (uRefsProc << 16) | uRefs;
                else
                {
                    LIBC_ASSERTM_FAILED("iSocket=%d is referenced too many times! uRefs=%#x\n", iSocket, uRefs);
                    *pu16 = 1;
                    rc = (1 << 16) | uRefs;
                }
            }
            else
            {
                LIBC_ASSERTM_FAILED("iSocket=%d is referenced too many times! uRefs=%#x\n", iSocket, uRefs);
                rc = -EBADF;
            }
        }
    }
    else
    {
        LIBC_ASSERTM_FAILED("iSocket=%d is out of the range (cSockets=%d)\n", iSocket, gpSPMHdr->pTcpip->cSockets);
        rc = -EINVAL;
    }
    LIBCLOG_RETURN_INT(rc);
}


/**
 * Dereferences a socket.
 *
 * @returns The new reference count.
 *          The low 16-bits are are the global count.
 *          The high 15-bits are are the process count.
 * @returns Negative error code (errno.h) on failure.
 * @param   iSocket     Socket to dereference.
 */
int     __libc_spmSocketDeref(int iSocket)
{
    LIBCLOG_ENTER("iSocket=%d\n", iSocket);
    int rc;
    if (iSocket >= 0 && iSocket < gpSPMHdr->pTcpip->cSockets)
    {
        rc = 0; /* shut up! */
        if (!gpSPMSelf->pacTcpipRefs)
            rc = spmSocketAllocProcess();
        if (gpSPMSelf->pacTcpipRefs)
        {
            uint16_t   *pu16 = &gpSPMHdr->pTcpip->acRefs[iSocket];
            unsigned uRefs = __atomic_decrement_word_min(pu16, 0);
            if (!(uRefs & 0xffff0000))
            {
                pu16 = &gpSPMSelf->pacTcpipRefs[iSocket];
                unsigned uRefsProc = __atomic_decrement_word_min(pu16, 0);
                if (!(uRefsProc & 0xffff0000))
                    rc = (uRefsProc << 16) | uRefs;
                else
                {
                    LIBC_ASSERTM_FAILED("iSocket=%d already 0 for process! (uRefsProc=%#x)\n", iSocket, uRefsProc);
                    *pu16 = 0;
                    rc = (0 << 16) | uRefs;
                }
            }
            else
            {
                LIBC_ASSERTM_FAILED("iSocket=%d already 0! (uRefs=%#x)\n", iSocket, uRefs);
                rc = -EBADF;
            }
        }
    }
    else
    {
        LIBC_ASSERTM_FAILED("iSocket=%d is out of the range (cSockets=%d)\n", iSocket, gpSPMHdr->pTcpip->cSockets);
        rc = -EINVAL;
    }
    LIBCLOG_RETURN_INT(rc);
}


/**
 * Get the stored load average samples.
 *
 * @returns 0 on success.
 * @returns Negative error code (errno.h) on failure.
 * @param   pLoadAvg    Where to store the load average samples.
 * @param   puTimestamp Where to store the current timestamp.
 */
int     __libc_spmGetLoadAvg(__LIBC_PSPMLOADAVG  pLoadAvg, unsigned *puTimestamp)
{
    __LIBC_SPMXCPTREGREC    RegRec;
    int                     rc;
    __LIBC_SPMLOADAVG       LoadAvg;

    rc = spmRequestMutex(&RegRec);
    if (rc)
        return rc;

    /* copy to temp buffer. */
    LoadAvg = gpSPMHdr->LoadAvg;

    spmReleaseMutex(&RegRec);

    /* copy to return buffer. */
    *pLoadAvg = LoadAvg;
    *puTimestamp = spmTimestamp();

    return 0;
}


/**
 * Get the stored load average samples.
 *
 * @returns 0 on success.
 * @returns Negative error code (errno.h) on failure.
 * @param   pLoadAvg    Where to store the load average samples.
 */
int     __libc_spmSetLoadAvg(const __LIBC_SPMLOADAVG *pLoadAvg)
{
    __LIBC_SPMXCPTREGREC    RegRec;
    int                     rc;
    __LIBC_SPMLOADAVG       LoadAvg;

    /* copy to temp buffer. */
    LoadAvg = *pLoadAvg;
    LoadAvg.uTimestamp = spmTimestamp();

    rc = spmRequestMutexErrno(&RegRec);
    if (rc)
        return rc;

    gpSPMHdr->LoadAvg = LoadAvg;

    spmReleaseMutex(&RegRec);
    return 0;
}


/**
 * Marks the process as a full LIBC process.
 *
 * Up to this point it was just a process which LIBC happend to be loaded into.
 *
 * @returns 0 on success.
 * @returns Negative error code (errno.h) on failure.
 */
void    __libc_spmExeInited(void)
{
    __atomic_xchg(&gpSPMSelf->fExeInited, 1);
}


/**
 * Queues a signal on another process.
 *
 * @returns 0 on success.
 * @returns Negative error code (errno.h) on failure.
 * @param   pSigInfo    Signal to queue.
 * @param   pid         Pid to queue it on.
 * @param   fQueued     Set if the signal type is queued.
 */
int     __libc_spmSigQueue(int iSignalNo, siginfo_t *pSigInfo, pid_t pid, int fQueued)
{
    LIBCLOG_ENTER("pSigInfo=%p:{.si_signo=%d,..} pid=%#x (%d) fQueued=%d\n", (void *)pSigInfo, pSigInfo->si_signo, pid, pid, fQueued);

    /*
     * Validate intput.
     * This is mostly because we wanna crash before we take the sem.
     */
    if (iSignalNo < 0 || iSignalNo > SIGRTMAX)
    {
        LIBC_ASSERTM_FAILED("Invalid signal number %d\n", pSigInfo->si_signo);
        LIBCLOG_RETURN_INT(-EINVAL);
    }
    if (pSigInfo)
    {
        if (pSigInfo->si_signo != iSignalNo)
        {
            LIBC_ASSERTM_FAILED("Signal Number doesn't match the signal info!\n");
            LIBCLOG_RETURN_INT(-EINVAL);
        }
        if (pSigInfo->auReserved[(sizeof(pSigInfo->auReserved) / sizeof(pSigInfo->auReserved[0])) - 1])
        {
            LIBC_ASSERTM_FAILED("Reserved field is not zero!\n");
            LIBCLOG_RETURN_INT(-EINVAL);
        }
    }

    /*
     * Request sem.
     */
    __LIBC_SPMXCPTREGREC    RegRec;
    int rc = spmRequestMutex(&RegRec);
    if (rc)
        LIBCLOG_RETURN_INT(rc);

    /*
     * Find pid and check that it's capable of catching signals.
     */
    __LIBC_PSPMPROCESS pProcess = spmQueryProcessInState(pid, __LIBC_PROCSTATE_ALIVE);
    if (pProcess && pProcess->fExeInited)
        rc = spmSigQueueProcess(pProcess, iSignalNo, pSigInfo, fQueued, 0);
    else
        rc = -ESRCH;

    /*
     * Release sem and be gone.
     */
    spmReleaseMutex(&RegRec);
    LIBCLOG_RETURN_INT(rc);
}

/**
 * Queues a signal on another process.
 *
 * @returns 0 on success.
 * @returns Negative error code (errno.h) on failure.
 * @param   iSignalNo   The signal to send. If 0 only permissions are checked.
 * @param   pSigInfo     Signal to queue. If NULL only permissions are checked.
 * @param   pgrp        Process group to queue a signal on.
 * @param   fQueued     Set if the signal type is queued.
 * @param   pfnCallback Pointer to callback function to post process signaled processes.
 *                      The callback must be _very_ careful. No crashing or blocking!
 * @param   pvUser      User argument specified to pfnCallback.
 */
int     __libc_spmSigQueuePGrp(int iSignalNo, siginfo_t *pSigInfo, pid_t pgrp, int fQueued, __LIBC_PFNSPMSIGNALED pfnCallback, void *pvUser)
{
    LIBCLOG_ENTER("iSignalNo=%d pSigInfo=%p:{.si_signo=%d,..} pgrp=%#x (%d) fQueued=%d pfnCallback=%p pvUser=%p\n",
                  iSignalNo, (void *)pSigInfo, pSigInfo->si_signo, pgrp, pgrp, fQueued, (void *)pfnCallback, pvUser);

    /*
     * Validate intput.
     * This is mostly because we wanna crash before we take the sem.
     */
    if (iSignalNo < 0 || iSignalNo > SIGRTMAX)
    {
        LIBC_ASSERTM_FAILED("Invalid signal number %d\n", pSigInfo->si_signo);
        LIBCLOG_RETURN_INT(-EINVAL);
    }
    if (pSigInfo)
    {
        if (pSigInfo->si_signo != iSignalNo)
        {
            LIBC_ASSERTM_FAILED("Signal Number doesn't match the signal info!\n");
            LIBCLOG_RETURN_INT(-EINVAL);
        }
        if (pSigInfo->auReserved[(sizeof(pSigInfo->auReserved) / sizeof(pSigInfo->auReserved[0])) - 1])
        {
            LIBC_ASSERTM_FAILED("Reserved field is not zero!\n");
            LIBCLOG_RETURN_INT(-EINVAL);
        }
    }

    /*
     * Request sem.
     */
    __LIBC_SPMXCPTREGREC    RegRec;
    int rc = spmRequestMutex(&RegRec);
    if (rc)
        LIBCLOG_RETURN_INT(rc);

    /*
     * Signal this process group?
     */
    if (!pgrp)
        pgrp = gpSPMSelf->pgrp;
    if (pgrp == gpSPMSelf->pgrp)
        LIBCLOG_MSG("Signalling our own process group, %#x (%d).\n", pgrp, pgrp);

    /*
     * Enumerate all alive processes and process the processes in the specified group.
     */
    unsigned            cSent = 0;
    __LIBC_PSPMPROCESS  pProcess = gpSPMHdr->apHeads[__LIBC_PROCSTATE_ALIVE];
    for (; pProcess; pProcess = pProcess->pNext)
    {
        if (    pProcess->pgrp == pgrp
            &&  pProcess->fExeInited)
        {
            /* try queue the signal */
            int rc2 = spmSigQueueProcess(pProcess, iSignalNo, pSigInfo, fQueued, 1 /* send anyway */);
            if (!rc2)
            {
                cSent++;
                if (pfnCallback)
                {
                    /* callback the caller. */
                    rc2 = pfnCallback(iSignalNo, pProcess, pvUser);
                    if (rc2 < 0 && !rc)
                        rc = rc2;
                }
            }
            else if (!rc)
                rc = rc2;
        }
    } /* for each alive process. */

    /*
     * Adjust error codes.
     */
    if (!rc)
    {
        if (!cSent)
            rc = -ESRCH;
    }
    else if (rc == -EPERM && cSent > 0)
        rc = 0;

    /*
     * Release sem and be gone.
     */
    spmReleaseMutex(&RegRec);
    LIBCLOG_RETURN_INT(rc);
}


/**
 * Get the signal set of pending signals.
 *
 * @returns Number of pending signals on success.
 * @returns 0 if no signals are pending.
 * @returns Negative error code (errno.h) on failure.
 * @param   pSigSet     Where to create the set of pending signals.
 */
int     __libc_spmSigPending(sigset_t *pSigSet)
{
    LIBCLOG_ENTER("pSigSet=%p\n", (void *)pSigSet);

    /*
     * Paranoid as always, work on stack copy.
     */
    sigset_t SigSet;
    __SIGSET_EMPTY(&SigSet);

    /*
     * Request sem.
     */
    __LIBC_SPMXCPTREGREC    RegRec;
    int rc = spmRequestMutex(&RegRec);
    if (rc)
        LIBCLOG_RETURN_INT(rc);

    /*
     * Walk thru the queued signals creating the pending signal set
     * and signal count.
     */
    __LIBC_PSPMSIGNAL pSig = gpSPMSelf->pSigHead;
    while (pSig)
    {
        rc++;
        __SIGSET_SET(&SigSet, pSig->Info.si_signo);
        pSig = pSig->pNext;
    }

    spmReleaseMutex(&RegRec);
    *pSigSet = SigSet;
    LIBCLOG_RETURN_MSG(rc, "ret %d *pSigSet={%08lx %08lx}", rc, pSigSet->__bitmap[1], pSigSet->__bitmap[0]);
}

/**
 * De-queues one or more pending signals of a specific type.
 *
 * @returns Number of de-queued signals on success.
 * @returns Negative error code (errno.h) on failure.
 * @param   iSignalNo   Signal type to dequeue.
 * @param   paSignals   Where to store the signals.
 * @param   cSignals    Size of the signal array.
 * @param   cbSignal    Size of one signal entry.
 */
int     __libc_spmSigDequeue(int iSignalNo, siginfo_t *paSignals, unsigned cSignals, size_t cbSignal)
{
    LIBCLOG_ENTER("paSignals=%p cSignals=%d cbSignal=%d\n", (void *)paSignals, cSignals, cbSignal);

    /*
     * Validate input.
     */
    if (cbSignal < sizeof(siginfo_t))
    {
        LIBC_ASSERTM_FAILED("Invalid siginfo_t size: cbSignal=%d sizeof(siginfo_t)=%d\n", cbSignal, sizeof(siginfo_t));
        LIBCLOG_RETURN_INT(-EINVAL);
    }
    bzero(paSignals, cSignals * cbSignal); /* This'll trap if it's wrong :-) */

    /*
     * Request sem.
     */
    __LIBC_SPMXCPTREGREC    RegRec;
    int rc = spmRequestMutex(&RegRec);
    if (rc)
        LIBCLOG_RETURN_INT(rc);

    /*
     * De-queue signals one-by-one.
     */
    __LIBC_PSPMSIGNAL pSigPrev = NULL;
    __LIBC_PSPMSIGNAL pSig = gpSPMSelf->pSigHead;
    while (pSig && cSignals-- > 0)
    {
        /*
         * Walk to the next signal of the right sort (if any).
         */
        while (pSig && pSig->Info.si_signo != iSignalNo)
        {
            pSigPrev = pSig;
            pSig = pSig->pNext;
        }
        if (!pSig)
            break;

        /*
         * Copy it.
         */
        if (pSig->cb >= sizeof(siginfo_t))
            *paSignals = pSig->Info;
        else
            memcpy(paSignals, &pSig->Info, pSig->cb);

        /*
         * Unlink it and free it.
         */
        /* statistics (free signal update sender stats) */
        gpSPMHdr->cSigActive--;

        /* unlink */
        __LIBC_PSPMSIGNAL pSigNext = pSig->pNext;
        if (pSigPrev)
            pSigPrev->pNext = pSigNext;
        else
            gpSPMSelf->pSigHead = pSigNext;

        /* free */
        spmFreeSignal(pSig);

        /*
         * Next.
         */
        pSig = pSigNext;
        paSignals = (siginfo_t *)((char *)paSignals + cbSignal);
        rc++;
    }

    spmReleaseMutex(&RegRec);
    LIBCLOG_RETURN_INT(rc);
}


/**
 * Checks the SPM memory for trouble.
 *
 * @returns 0 on perfect state.
 * @returns -1 and errno on mutex failure.
 * @returns Number of failures if SPM is broken.
 * @param   fBreakpoint Raise breakpoint exception if a problem is encountered.
 * @param   fVerbose    Log everything.
 */
int __libc_SpmCheck(int fBreakpoint, int fVerbose)
{
    __LIBC_SPMXCPTREGREC    RegRec;
    int                     rc;

    rc = spmRequestMutexErrno(&RegRec);
    if (rc)
        return rc;
    rc = spmCheck(fBreakpoint, fVerbose);
    spmReleaseMutex(&RegRec);
    return rc;
}


_CRT_INIT1(spmCrtInit1)
/**
 * Initializes the process database while we're still 100%
 * sure we're single threaded. This avoids trouble.
 *
 * To tell the truth chances are _very_ high that this
 * is not necessary because we will soon get the current
 * process structure very early in the LIBC init process.
 */
CRT_DATA_USED
static void spmCrtInit1(void)
{
    static void *hack = (void *)spmCrtInit1;
    LIBCLOG_ENTER("\n");
    hack = hack;
    if (!gpSPMSelf)
        __libc_spmSelf();
    LIBCLOG_RETURN_VOID();
}


_FORK_CHILD1(0xfffffff0, spmForkChild1)
/**
 * Fork child callback.
 *
 * This takes care of the globals during a fork where those are overwritten
 * by the copying of the data segment.
 *
 * @returns 0 on success.
 * @param   pForkHandle     Pointer to the fork handle.
 * @param   enmOperation    Callback operation.
 */
static int spmForkChild1(__LIBC_PFORKHANDLE pForkHandle, __LIBC_FORKOP enmOperation)
{
    LIBCLOG_ENTER("pForkHandle=%p enmOperation=%d\n", (void *)pForkHandle, enmOperation);

    switch (enmOperation)
    {
        /*
         * Before exec we must ensure that we're opened
         * for the logic below to work.
         */
        case __LIBC_FORK_OP_EXEC_CHILD:
            if (!gpSPMSelf)
                __libc_spmSelf();
            LIBCLOG_RETURN_INT(0);

        /*
         * We must refind our self since the self pointer
         * have been overwritten with the pointer to the parent
         * process during the data segment copying.
         */
        case __LIBC_FORK_OP_FORK_CHILD:
        {
            PTIB    pTib;
            PPIB    pPib;
            FS_VAR();
            LIBC_ASSERTM(gpSPMHdr, "gpSPMHdr is NULL after fork!!!\n");
            FS_SAVE_LOAD();
            DosGetInfoBlocks(&pTib, &pPib);
            gpSPMSelf = __libc_spmQueryProcessInState(pPib->pib_ulpid, __LIBC_PROCSTATE_ALIVE);
            LIBC_ASSERTM(gpSPMSelf, "Couldn't find our self after fork - impossibled! pid=%lx (%ld) ppid=%lx (%ld)\n",
                         pPib->pib_ulpid, pPib->pib_ulpid, pPib->pib_ulppid, pPib->pib_ulppid);
            LIBC_ASSERTM(gpSPMSelf->cReferences >= 2, "cReferences=%d!\n", gpSPMSelf->cReferences);
            gpSPMSelf->cReferences--;
            FS_RESTORE();
            LIBCLOG_RETURN_INT(0);
        }

        default:
            LIBCLOG_RETURN_INT(0);
    }
}


/*//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\*/
/*//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\*/
/*//                                                                                                                \\*/
/*//                                                                                                                \\*/
/*//        Internal workers - basically all of them requires ownership of the memory before being called.          \\*/
/*//                                                                                                                \\*/
/*//                                                                                                                \\*/
/*//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\*/
/*//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\*/


/**
 * Requests the shared mutex semphore and checks that we're
 * successfully initialized.
 * @internal
 */
static int spmRequestMutexErrno(__LIBC_PSPMXCPTREGREC pRegRec)
{
    int rc = spmRequestMutex(pRegRec);
    if (!rc)
        return 0;
    errno = rc;
    return -1;
}

/**
 * This function checks that there is at least 2k of writable
 * stack available. If there isn't, a crash is usually the
 * result.
 * @internal
 */
static int spmRequestMutexStackChecker(void)
{
    char volatile *pch = alloca(2048);
    if (!pch)
        return -1;
    /* With any luck the compiler doesn't optimize this away. */
    pch[0] = pch[1024] = pch[2044] = 0x7f;
    return 0;
}

/**
 * Requests the shared mutex semphore and checks that we're
 * successfully initialized.
 * @returns 0 on success.
 * @returns Negative error code (errno.h) on failure.
 * @internal
 */
static int spmRequestMutex(__LIBC_PSPMXCPTREGREC pRegRec)
{
    LIBCLOG_ENTER("\n");
    ULONG       ul;
    int         rc;
    FS_VAR();

    /*
     * Install the exception handler.
     */
    FS_SAVE_LOAD();
    pRegRec->Core.prev_structure   = (void *)~0;
    pRegRec->Core.ExceptionHandler = spmXcptHandler;
    DosSetExceptionHandler(&pRegRec->Core);

    /*
     * Check stack.
     */
    if (spmRequestMutexStackChecker())
    {
        DosUnsetExceptionHandler(&pRegRec->Core);
        FS_RESTORE();
        LIBC_ASSERTM_FAILED("Too little stack left!\n");
        LIBCLOG_RETURN_INT(-EFAULT);
    }

    /*
     * Check if initiated.
     */
    if (!ghmtxSPM || !gpSPMHdr)
    {
        /*
         * Perform lazy init.
         */
        rc = spmInit();
        if (!rc)
        {
            FS_RESTORE();
            LIBCLOG_RETURN_INT(0);
        }

        LIBC_ASSERTM_FAILED("not initialized!\n");
        DosUnsetExceptionHandler(&pRegRec->Core);
        FS_RESTORE();
        LIBCLOG_RETURN_INT(rc);
    }

    /*
     * Request semaphore and enter "must complete section" to avoid signal trouble.
     */
    rc = DosRequestMutexSem(ghmtxSPM, SPM_MUTEX_TIMEOUT);
    DosEnterMustComplete(&ul);
    if (!rc)
    {
        if (gcNesting == 0)
        {
            gcNesting++;
            FS_RESTORE();
            LIBCLOG_RETURN_INT(0);
        }
        LIBC_ASSERTM_FAILED("Nested access to shared memory is not allowed\n");
        DosReleaseMutexSem(ghmtxSPM);
        rc = -EBUSY;
    }
    else
    {
        /* @todo recover from owner died. */
        LIBC_ASSERTM_FAILED("DosRequestMutexSem(%lu) failed with rc=%d!\n", ghmtxSPM, rc);
        rc = -__libc_native2errno(rc);
    }

    DosUnsetExceptionHandler(&pRegRec->Core);
    DosExitMustComplete(&ul);
    FS_RESTORE();
    LIBCLOG_RETURN_INT(rc);
}


/**
 * Releases the mutex semaphore
 * @internal
 */
static int spmReleaseMutex(__LIBC_PSPMXCPTREGREC pRegRec)
{
    LIBCLOG_ENTER("\n");
    ULONG   ul = 0;
    int     rc;
    FS_VAR();

    /*
     * Release the semaphore.
     */
    gcNesting--;
    FS_SAVE_LOAD();
    rc = DosReleaseMutexSem(ghmtxSPM);
    if (rc)
    {
        FS_RESTORE();
        LIBC_ASSERTM_FAILED("DosReleaseMutexSem(%lu) -> %d\n", ghmtxSPM, rc);
        LIBCLOG_RETURN_INT(1);
    }

    DosUnsetExceptionHandler(&pRegRec->Core);
    DosExitMustComplete(&ul);
    FS_RESTORE();
    LIBCLOG_RETURN_INT(0);
}

#if 0 /* unused */
/**
 * Checks if the caller is the mutex owner.
 * @returns 0 if not owner.
 * @returns 1 if owner.
 */
static int spmIsMutexOwner(void)
{
    if (gcNesting != 1)
        return 0;

    PTIB pTib;
    PPIB pPib;
    DosGetInfoBlocks(&pTib, &pPib);
    PID pid;
    TID tid;
    ULONG cNestings = 0;
    int rc = DosQueryMutexSem(ghmtxSPM, &pid, &tid, &cNestings);
    return !rc && pid == pPib->pib_ulpid && tid == pTib->tib_ptib2->tib2_ultid;
}
#endif /* unused */

/**
 * Open and if needed creates the LIBC shared memory.
 *
 * This is called lazily by the mutex request function.
 *
 * @returns 0 on success. Mutex is owned.
 * @returns Negative error code (errno.h) on failure.
 */
static int spmInit(void)
{
    LIBCLOG_ENTER("\n");
    int                     rc;
    PPIB                    pPib;
    PTIB                    pTib;
    FS_VAR();

    /*
     * Size and offset assertions.
     */
    LIBC_ASSERT(sizeof(*gpSPMHdr) < SPM_PROCESS_SIZE);
    LIBC_ASSERT(offsetof(__LIBC_SPMHEADER, pTcpip) == 56);
    LIBC_ASSERT(offsetof(__LIBC_SPMHEADER, pChildNotifyFreeHead) == 52);
    LIBC_ASSERT(offsetof(__LIBC_SPMHEADER, hevNotify) == 112);
    LIBC_ASSERT(offsetof(__LIBC_SPMHEADER, cSigMaxActive) == 108);
    LIBC_ASSERT(offsetof(__LIBC_SPMPROCESS, cReferences) == 12);
    LIBC_ASSERT(offsetof(__LIBC_SPMPROCESS, pvForkHandle) == 140);
    LIBC_ASSERT(offsetof(__LIBC_SPMPROCESS, pacTcpipRefs) == 180);
    LIBC_ASSERT(offsetof(__LIBC_SPMPROCESS, cPoolPointers) == 224);
    LIBC_ASSERT(offsetof(__LIBC_SPMPROCESS, pInheritLocked) == 232);

    /*
     * Check the state of the affair.
     */
    LIBC_ASSERT(!ghmtxSPM && !gpSPMHdr);
    LIBC_ASSERT(!gcNesting);
    if (gcNesting)
        LIBCLOG_RETURN_INT(-EINVAL);

    /*
     * Get current process id and parent process id
     * and install the exception handler.
     */
    DosGetInfoBlocks(&pTib, &pPib);

    /*
     * Open or create mutex.
     */
    FS_SAVE_LOAD();
    rc = DosOpenMutexSem((PCSZ)SPM_MUTEX_NAME, &ghmtxSPM);
    if (rc)
    {
        rc = DosCreateMutexSem((PCSZ)SPM_MUTEX_NAME, &ghmtxSPM, DC_SEM_SHARED, 0L);
        if (rc)
        {
            /* retry opening it, someone might have beaten us creating it. */
            int rc2 = DosOpenMutexSem((PCSZ)SPM_MUTEX_NAME, &ghmtxSPM);
            if (rc2)
            {
                FS_RESTORE();
                /* too bad, can create it for some reason. */
                LIBC_ASSERTM_FAILED("Failed to open mutex. rc=%d rc2=%d\n", rc, rc2);
                ghmtxSPM = NULLHANDLE;
                rc = -__libc_native2errno(rc);
                LIBCLOG_RETURN_INT(rc);
            }
        }
    }

    /*
     * Request the mutex.
     */
    rc = DosRequestMutexSem(ghmtxSPM, SPM_MUTEX_TIMEOUT);
    if (!rc)
    {
        ULONG   ul;
        PVOID   pv = NULL;              /* -pedantic annoyance. */
        DosEnterMustComplete(&ul);
        gcNesting = 1;

        /*
         * Get or create the shared memory.
         */
        rc = DosGetNamedSharedMem(&pv, (PCSZ)SPM_MEMORY_NAME, PAG_READ | PAG_WRITE);
        if (!rc)
        {
            gpSPMHdr = (__LIBC_PSPMHEADER)pv;
            if (!gpSPMHdr->cSigMaxActive)
                gpSPMHdr->cSigMaxActive = 1024;
            if (gpSPMHdr->hevNotify)
                rc = DosOpenEventSem(NULL, &gpSPMHdr->hevNotify);
            else
                rc = DosCreateEventSem(NULL, &gpSPMHdr->hevNotify, DC_SEM_SHARED, FALSE);
        }
        else
        {
            /*
             * Must create it.
             */
            /* first we'll try with 4times the amount and OBJ_ANY. */
            size_t  cb = SPM_MEMORY_SIZE * 4;
            rc = DosAllocSharedMem(&pv, (PCSZ)SPM_MEMORY_NAME, cb, PAG_READ | PAG_WRITE | PAG_COMMIT | OBJ_ANY);
            /* if we got a low address we must free and use minimum size. */
            if (!rc && (uintptr_t)pv < 0x20000000/*512MB*/)
            {
                DosFreeMem(pv);
                rc = -1;
            }
            /* on failure, retry with default size and no OBJ_ANY. */
            if (rc)
            {
                cb = SPM_MEMORY_SIZE;
                rc = DosAllocSharedMem(&pv, (PCSZ)SPM_MEMORY_NAME, cb, PAG_READ | PAG_WRITE | PAG_COMMIT);
            }
            if (!rc)
            {
                /*
                 * Init the header.
                 */
                gpSPMHdr = (__LIBC_PSPMHEADER)pv;
                LIBCLOG_MSG("Initializing the shared memory.\n");
                gpSPMHdr->uVersion          = SPM_VERSION;
                gpSPMHdr->cb                = cb;
                gpSPMHdr->cbProcess         = SPM_PROCESS_SIZE;
                gpSPMHdr->pPoolTail         = gpSPMHdr->pPoolHead     = (__LIBC_PSPMPOOLCHUNK)((char *)gpSPMHdr + SPM_PROCESS_SIZE);
                gpSPMHdr->pPoolFreeTail     = gpSPMHdr->pPoolFreeHead = (__LIBC_PSPMPOOLCHUNKFREE)gpSPMHdr->pPoolTail;
                gpSPMHdr->pPoolFreeTail->cb = gpSPMHdr->cbFree        = cb - sizeof(__LIBC_SPMPOOLCHUNK) - SPM_PROCESS_SIZE;
                gpSPMHdr->pidCreate         = pPib->pib_ulpid;
                DosGetDateTime(&gpSPMHdr->dtCreate);
                gpSPMHdr->pTcpip            = spmAlloc(sizeof(*gpSPMHdr->pTcpip));
                gpSPMHdr->pTcpip->cb        = sizeof(*gpSPMHdr->pTcpip);
                gpSPMHdr->pTcpip->cSockets  = sizeof(gpSPMHdr->pTcpip->acRefs) / sizeof(gpSPMHdr->pTcpip->acRefs[0]);
                gpSPMHdr->cSigMaxActive     = 1024;
                /* Pre-allocate a few signals. */
                gpSPMHdr->cSigFree          = 8;
                unsigned i = gpSPMHdr->cSigFree;
                while (i--)
                {
                    __LIBC_PSPMSIGNAL  pSignal = spmAlloc(sizeof(*pSignal));
                    pSignal->cb = sizeof(*pSignal);
                    pSignal->pNext = gpSPMHdr->pSigFreeHead;
                    gpSPMHdr->pSigFreeHead = pSignal;
                }
                /* Create notification semaphore. */
                rc = DosCreateEventSem(NULL, &gpSPMHdr->hevNotify, DC_SEM_SHARED, FALSE);
                LIBC_ASSERTM(!rc, "DosCreateEventSem rc=%d\n", rc);
                rc = 0; /* ignore */
            }
        }

        /* success? */
        if (!rc)
        {
            /*
             * Register the current process, increment open counter,
             * and notify everyone that spm data has changed.
             */
            PLINFOSEG pLIS = GETLINFOSEG();
            spmRegisterSelf(pPib->pib_ulpid, pPib->pib_ulppid, pLIS->sgCurrent);
            if (gpSPMSelf)
            {
                gpSPMSelf->cSPMOpens++;

                LIBCLOG_MSG("posting %#lx\n", gpSPMHdr->hevNotify);
                APIRET rc2 = DosPostEventSem(gpSPMHdr->hevNotify);
                LIBC_ASSERTM(!rc2 || rc2 == ERROR_ALREADY_POSTED, "rc2=%ld!\n", rc2); rc2 = rc2;
            }

            /*
             * Register exit list handler and return with
             * mutex ownership. This must have a priority lower than
             * the TCPIP library of OS/2 to prevent it from closing
             * sockets.
             */
            rc = DosExitList(0x9800 | EXLST_ADD, spmExitList);
            FS_RESTORE();
            LIBC_ASSERTM(!rc, "DosExitList failed with rc=%d\n", rc);
            LIBCLOG_RETURN_INT(0);
        }

        /* failure */
        gcNesting = 0;
        DosReleaseMutexSem(ghmtxSPM);
        DosExitMustComplete(&ul);
    }

    /*
     * Failure, cleanup properly.
     */
    LIBCLOG_MSG("Failed rc=%d\n", rc);
    DosCloseMutexSem(ghmtxSPM);
    ghmtxSPM = NULLHANDLE;
    gpSPMHdr = NULL;
    gpSPMSelf = NULL;
    FS_RESTORE();
    rc = -__libc_native2errno(rc);
    LIBCLOG_RETURN_INT(rc);
}


/**
 * Exit list procedure which makes sure we clean up our mess properly.
 * @internal
 */
static VOID APIENTRY spmExitList(ULONG ulReason)
{
    /*
     * Notify the wait and timer facilities.
     */
    __libc_back_processWaitNotifyTerm();
    __libc_back_signalTimerNotifyTerm();

    /*
     * Check if the memory is accessible, if it ain't we'll crash or do other
     * nasty things is the code below.
     */
    if (ghmtxSPM)
    {
        /*
         * Exist list callbacks .
         */
        if (gapfnExitList[0])
            gapfnExitList[0]();
        if (gapfnExitList[1])
            gapfnExitList[1]();
        if (gapfnExitList[2])
            gapfnExitList[2]();
        if (gapfnExitList[3])
            gapfnExitList[3]();

        /*
         * Convert reason.
         */
        __LIBC_EXIT_REASON  enmDeathReason = __LIBC_EXIT_REASON_XCPT;
        int                 iExitCode      = -4; /* fixme!! */
        switch (ulReason)
        {
            case TC_EXIT:       enmDeathReason = __LIBC_EXIT_REASON_EXIT;       iExitCode = 0; break;
            case TC_HARDERROR:  enmDeathReason = __LIBC_EXIT_REASON_HARDERROR;  iExitCode = -1; break;
            case TC_TRAP:       enmDeathReason = __LIBC_EXIT_REASON_TRAP;       iExitCode = -2; break;
            case TC_KILLPROCESS:enmDeathReason = __LIBC_EXIT_REASON_KILL;       iExitCode = -3; break;
            case TC_EXCEPTION:  enmDeathReason = __LIBC_EXIT_REASON_XCPT;       iExitCode = -4; break;
        }
        __libc_spmTerm(enmDeathReason, iExitCode);

        /*
         * Clean up the current process.
         */
        spmCleanup();
    }

    DosExitList(EXLST_EXIT, spmExitList);
}


/**
 * Marks the current process (if we have it around) as zombie
 * or dead freeing all resources associated with it.
 */
static void spmCleanup(void)
{
    LIBCLOG_ENTER("\n");
    PPIB                    pPib;
    PTIB                    pTib;
    __LIBC_SPMXCPTREGREC    RegRec;
    int                     fFree = 0;
    FS_VAR();

    /*
     * Ignore request if already terminated.
     */
    if (!gpSPMHdr || !ghmtxSPM)
        LIBCLOG_RETURN_VOID();

    /*
     * Cleanup current process.
     * Get pid before mutext as a speed and safety precaution.
     */
    FS_SAVE_LOAD();
    DosGetInfoBlocks(&pTib, &pPib);
    if (!spmRequestMutexErrno(&RegRec))
    {
        /*
         * Free unborn children.
         */
        __LIBC_PSPMPROCESS     pProcess = gpSPMHdr->apHeads[__LIBC_PROCSTATE_EMBRYO];
        pid_t                  pid = pPib->pib_ulpid;
        while (pProcess)
        {
            /* sanity */
            LIBC_ASSERTM(pProcess->enmState == __LIBC_PROCSTATE_EMBRYO, "Found non embryo process in embryo list! enmState=%d pid=%#x (%d)\n",
                         pProcess->enmState, pProcess->pid, pProcess->pid);
            LIBC_ASSERTM(pProcess->cReferences <= 1, "Invalid reference count of a process in the embryo list! cReferences=%d pid=%#x (%d)\n",
                         pProcess->cReferences, pProcess->pid, pProcess->pid);

            /* our child? */
            if (pProcess->pidParent == pid)
            {
                __LIBC_PSPMPROCESS pProcessNext = pProcess->pNext;

                /* Free it safely. */
                LIBCLOG_MSG("Cleanup embryo %p.\n", (void *)pProcess);
                spmZombieOrFree(pProcess);

                /* next */
                pProcess = pProcessNext;
                continue;
            }

            /* next */
            pProcess = pProcess->pNext;
        }

        /*
         * Our selves.
         */
        if (gpSPMSelf)
        {
            /* */
            pProcess = gpSPMSelf;
            gpSPMSelf = NULL;

            /* Determin whether to free shared stuff. */
            if (pProcess->cSPMOpens <= 1)
            {
                pProcess->cSPMOpens = 0;
                fFree = 1;
            }
            else
                pProcess->cSPMOpens--;

            /*
             * if we have a child termination notification structure we'll
             * send that to the parent (if we have one).
             */
            __LIBC_PSPMCHILDNOTIFY pTerm = pProcess->pTerm;
            if (pTerm)
            {
                __LIBC_PSPMPROCESS pParent = spmQueryProcessInState(pProcess->pidParent, __LIBC_PROCSTATE_ALIVE);
                if (pParent)
                {
                    pTerm->pid = pProcess->pid;
                    pTerm->pgrp = pProcess->pgrp;
                    pTerm->pNext = NULL;
                    *pParent->ppChildNotifyTail = pTerm;
                    pParent->ppChildNotifyTail = &pTerm->pNext;

                    LIBCLOG_MSG("posting %#lx\n", gpSPMHdr->hevNotify);
                    APIRET rc2 = DosPostEventSem(gpSPMHdr->hevNotify);
                    LIBC_ASSERTM(!rc2 || rc2 == ERROR_ALREADY_POSTED, "rc2=%ld!\n", rc2); rc2 = rc2;
                }
                else
                    spmFreeChildNotify(pTerm);
                pProcess->pTerm = NULL;
            }

            /* free our selves or become a zombie. */
            spmZombieOrFree(pProcess);
        }

        /*
         * We're done, free the mutex.
         */
        spmReleaseMutex(&RegRec);
    }

    /* Mess up the state - reopen is not supported! */
    gpSPMHdr  = NULL;
    gpSPMSelf = NULL;
    ghmtxSPM  = 0;
    gcNesting = 42;

    FS_RESTORE();
    LIBCLOG_RETURN_VOID();
}


/**
 * Dereferences a linked process.
 * This means putting it in either the free or zombie list.
 * @internal
 */
static void spmZombieOrFree(__LIBC_PSPMPROCESS pProcess)
{
    LIBCLOG_ENTER("pProcess=%p\n", (void *)pProcess);
    LIBC_ASSERT(pProcess->enmState != __LIBC_PROCSTATE_FREE);

    /*
     * Dereference it.
     * Note that if we're cleaning up embyos, the referenc count is 0.
     */
    if (pProcess->cReferences > 0)
        pProcess->cReferences--;

    /*
     * If no more references we'll unlink and free the process.
     */
    if (pProcess->cReferences == 0)
        spmFreeProcess(pProcess);
    /*
     * If more references, then put it in the zombie list.
     */
    else if (pProcess->enmState != __LIBC_PROCSTATE_ZOMBIE)
    {
        LIBCLOG_MSG("Making process %p a zombie (pid=%#x (%d) pidParent=%#x (%d) cReferences=%d enmState=%d)\n",
                    (void *)pProcess, pProcess->pid, pProcess->pid, pProcess->pidParent, pProcess->pidParent,
                    pProcess->cReferences, pProcess->enmState);

        /*
         * Free data a zombie won't be needing.
         */
        /* inherited data */
        void *pv = pProcess->pInheritLocked;
        if (!pv)
            pv = pProcess->pInherit;
        if (pv)
        {
            pProcess->pInherit = NULL;
            pProcess->pInheritLocked = NULL;
            spmFree(pv);
        }

        /* signals */
        __LIBC_PSPMSIGNAL pSig = pProcess->pSigHead;
        pProcess->pSigHead = NULL;
        while (pSig)
        {
            __LIBC_PSPMSIGNAL pFree = pSig;
            pSig = pSig->pNext;
            spmFreeSignal(pFree);
        }

        /* child notifications. */
        __LIBC_PSPMCHILDNOTIFY pNotify = pProcess->pChildNotifyHead;
        pProcess->pChildNotifyHead = NULL;
        while (pNotify)
        {
            __LIBC_PSPMCHILDNOTIFY pFree = pNotify;
            pNotify = pNotify->pNext;
            spmFreeChildNotify(pFree);
        }

        /* unlink. */
        if (pProcess->pNext)
            pProcess->pNext->pPrev = pProcess->pPrev;
        if (pProcess->pPrev)
            pProcess->pPrev->pNext = pProcess->pNext;
        else
            gpSPMHdr->apHeads[pProcess->enmState] = pProcess->pNext;

        /* zombie */
        pProcess->enmState = __LIBC_PROCSTATE_ZOMBIE;
        pProcess->pNext = gpSPMHdr->apHeads[__LIBC_PROCSTATE_ZOMBIE];
        if (pProcess->pNext)
            pProcess->pNext->pPrev = pProcess;
        pProcess->pPrev = NULL;
        gpSPMHdr->apHeads[__LIBC_PROCSTATE_ZOMBIE] = pProcess;
    }
    /* else already zombie */

    LIBCLOG_RETURN_VOID();
}


/**
 * Searches for a process with a given pid and state.
 *
 * @returns Pointer to the desired process on success.
 * @returns NULL on failure.
 * @param   pid         Process id to search for.
 * @param   enmState 	The state of the process.
 */
static __LIBC_PSPMPROCESS spmQueryProcessInState(pid_t pid, __LIBC_SPMPROCSTAT enmState)
{
    LIBCLOG_ENTER("pid=%#x (%d) enmState=%d\n", pid, pid, enmState);
    __LIBC_PSPMPROCESS      pProcess;

    /*
     * Search process list.
     */
    for (pProcess = gpSPMHdr->apHeads[enmState]; pProcess ; pProcess = pProcess->pNext)
        if (pProcess->pid == pid)
            LIBCLOG_RETURN_P(pProcess);

    LIBCLOG_RETURN_P(NULL);
}



/**
 * Gets the current timestamp.
 *
 * @returns The current timestamp.
 */
static unsigned spmTimestamp(void)
{
    ULONG   ul;
    int     rc;
    FS_VAR();
    FS_SAVE_LOAD();
    ul = 0;
    rc = DosQuerySysInfo(QSV_MS_COUNT, QSV_MS_COUNT, &ul, sizeof(ul));
    LIBC_ASSERTM(!rc, "DosQuerySysInfo failed rc=%d\n", rc);
    FS_RESTORE();
    rc = rc;
    return (unsigned)ul;
}


/**
 * Register the current process.
 *
 * @returns Pointer to the structure for the current process.
 * @returns NULL and errno on failure.
 * @param   pid         Pid of this process.
 * @param   pidParent   Pid of parent process.
 * @param   sid         Session Id.
 */
static __LIBC_PSPMPROCESS spmRegisterSelf(pid_t pid, pid_t pidParent, pid_t sid)
{
    LIBCLOG_ENTER("pid=%#x (%d) pidParent=%#x (%d) sid=%#x (%d)\n", pid, pid, pidParent, pidParent, sid, sid);
    __LIBC_PSPMPROCESS pProcess;
    __LIBC_PSPMPROCESS pProcessBest;

    /*
     * If we've got a process there is not reason for registering our selves again.
     */
    if (gpSPMSelf)
        LIBCLOG_RETURN_P(gpSPMSelf);

    /*
     * See if we're in the embryo list.
     */
    for (pProcessBest = NULL,  pProcess = gpSPMHdr->apHeads[__LIBC_PROCSTATE_EMBRYO]; pProcess; pProcess = pProcess->pNext)
    {
        /* sanity */
        LIBC_ASSERTM((uintptr_t)pProcess - (uintptr_t)gpSPMHdr < gpSPMHdr->cb,
                     "Invalid pointer %p in EMBRYO list\n", (void *)pProcess);
        LIBC_ASSERTM(pProcess->enmState == __LIBC_PROCSTATE_EMBRYO, "Found non embryo process in embryo list! enmState=%d pid=%#x (%d)\n",
                     pProcess->enmState, pProcess->pid, pProcess->pid);
        LIBC_ASSERTM(pProcess->cReferences <= 1, "Invalid reference count of a process in the embryo list! cReferences=%d pid=%#x (%d)\n",
                     pProcess->cReferences, pProcess->pid, pProcess->pid);

        /* our daddy? daddy updated it with our pid? */
        if (    pProcess->pidParent == pidParent)
        {
            if (pProcess->pid == pid)
            {
                pProcessBest = pProcess;
                break;
            }
            if (!pProcessBest)
                pProcessBest = pProcess;
        }
        LIBCLOG_MSG("Walking by embryo %p (pidParent=%04x cReferences=%d)\n",
                    (void *)pProcess, pProcess->pidParent, pProcess->cReferences);

        /* cure insanity... no fix for paranoia? */
        if (    pProcess->pNext == gpSPMHdr->apHeads[__LIBC_PROCSTATE_EMBRYO]
            ||  pProcess->pNext == pProcess)
        {
            LIBC_ASSERTM(pProcess->pNext != gpSPMHdr->apHeads[__LIBC_PROCSTATE_EMBRYO], "Circular list! pid=%#x (%d)\n", pProcess->pid, pProcess->pid);
            LIBC_ASSERTM(pProcess->pNext != pProcess, "Circular list with self! pid=%#x (%d)\n", pProcess->pid, pProcess->pid);
            pProcess->pNext = NULL;
        }
    }

    if (pProcessBest)
    {
        pProcess = pProcessBest;
        LIBCLOG_MSG("Found my embryo %p (pidParent=%#x pid=%#x cReferences=%d uTimestamp=%04x pInherit=%p/%p)\n",
                    (void *)pProcess, pProcess->pidParent, pProcess->pid, pProcess->cReferences, pProcess->uTimestamp, pProcess->pInherit, pProcess->pInheritLocked);

        /* set data. */
        pProcess->cReferences++;
        pProcess->pid = pid;
        pProcess->sid = sid;

        /* unlink. */
        if (pProcess->pNext)
            pProcess->pNext->pPrev = pProcess->pPrev;
        if (pProcess->pPrev)
            pProcess->pPrev->pNext = pProcess->pNext;
        else
            gpSPMHdr->apHeads[__LIBC_PROCSTATE_EMBRYO] = pProcess->pNext;

        /* update state and insert at head of that state list. */
        LIBC_ASSERT(sizeof(pProcess->enmState) == sizeof(int));
        __lxchg((int volatile *)&pProcess->enmState, __LIBC_PROCSTATE_ALIVE);
        pProcess->pNext = gpSPMHdr->apHeads[__LIBC_PROCSTATE_ALIVE];
        if (pProcess->pNext)
            pProcess->pNext->pPrev = pProcess;
        pProcess->pPrev = NULL;
        gpSPMHdr->apHeads[__LIBC_PROCSTATE_ALIVE] = pProcess;

        gpSPMSelf = pProcess;

        /* return. */
        LIBCLOG_RETURN_P(pProcess);
    }

    /*
     * See if we're in the alive list (multiple LIBC versions).
     */
    for (pProcess = gpSPMHdr->apHeads[__LIBC_PROCSTATE_ALIVE]; pProcess; pProcess = pProcess->pNext)
    {
        /* (in)santity */
        LIBC_ASSERTM((uintptr_t)pProcess - (uintptr_t)gpSPMHdr < gpSPMHdr->cb,
                     "Invalid pointer %p in ALIVE list\n", (void *)pProcess);
        LIBC_ASSERTM(pProcess->enmState == __LIBC_PROCSTATE_ALIVE, "Found non alive process in alive list! enmState=%d pid=%#x (%d)\n",
                     pProcess->enmState, pProcess->pid, pProcess->pid);

        /* is this really me? */
        if (    pProcess->pid == pid
            &&  pProcess->pidParent == pidParent)
        {
            gpSPMSelf = pProcess;
            pProcess->cReferences++;
            LIBCLOG_RETURN_P(pProcess);
        }

        /* more sanity */
        LIBC_ASSERTM(pProcess->pid != pid, "Found pid %d with parent %d when searching for that pid but with parent %d\n",
                     pid, pProcess->pidParent, pidParent);
        /* make sane... who's paranoid here... */
        if (    pProcess->pNext == gpSPMHdr->apHeads[__LIBC_PROCSTATE_ALIVE]
            ||  pProcess->pNext == pProcess)
        {
            LIBC_ASSERTM(pProcess->pNext != gpSPMHdr->apHeads[__LIBC_PROCSTATE_ALIVE], "Circular list! pid=%#x (%d)\n", pProcess->pid, pProcess->pid);
            LIBC_ASSERTM(pProcess->pNext != pProcess, "Circular list with self! pid=%#x (%d)\n", pProcess->pid, pProcess->pid);
            pProcess->pNext = NULL;
        }
    }

    /*
     * Create a new process block.
     */
    pProcess = spmAllocProcess();
    if (pProcess)
    {
        /*
         * Initialize the new process block.
         */
        pProcess->uVersion          = SPM_VERSION;
        pProcess->cReferences       = 1;
        pProcess->enmState          = __LIBC_PROCSTATE_ALIVE;
        pProcess->pid               = pid;
        pProcess->pidParent         = pidParent;
        //pProcess->uid               = 0;
        //pProcess->euid              = 0;
        //pProcess->svuid             = 0;
        //pProcess->gid               = 0;
        //pProcess->egid              = 0;
        //pProcess->svgid             = 0;
        //bzero(&pProcess->agidGroups[0], sizeof(pProcess->agidGroups));
        pProcess->uTimestamp        = spmTimestamp();
        //pProcess->cSPMOpens         = 0;
        pProcess->fExeInited        = 1;
        //pProcess->uMiscReserved     = 0;
        //pProcess->pvModuleHead      = NULL;
        pProcess->ppvModuleTail     = &pProcess->pvModuleHead;
        //pProcess->pvForkHandle      = NULL;
        //pProcess->pSigHead          = NULL;
        //pProcess->cSigsSent         = 0;
        pProcess->sid               = sid;
        pProcess->pgrp              = pid;
        //pProcess->uSigReserved1     = 0; //gpSPMSelf->uSigReserved1;
        //pProcess->uSigReserved2     = 0; //gpSPMSelf->uSigReserved2;
        __LIBC_PSPMCHILDNOTIFY pTerm= spmAllocChildNotify();
        if (pTerm)
        {
            pTerm->cb               = sizeof(*pTerm);
            pTerm->enmDeathReason   = __LIBC_EXIT_REASON_NONE;
            pTerm->iExitCode        = 0;
            pTerm->pid              = pid;
            pTerm->pgrp             = pProcess->pgrp;
            pTerm->pNext            = NULL;
            pProcess->pTerm         = pTerm;
        }
        //pProcess->pChildNotifyHead  = NULL;
        pProcess->ppChildNotifyTail = &pProcess->pChildNotifyHead;
        //pProcess->auReserved        = {0};
        pProcess->cPoolPointers     = __LIBC_SPMPROCESS_POOLPOINTERS;
        //pProcess->pInherit          = NULL;
        //pProcess->pInheritLocked    = NULL;

        /* link into list. */
        pProcess->pNext = gpSPMHdr->apHeads[__LIBC_PROCSTATE_ALIVE];
        if (pProcess->pNext)
            pProcess->pNext->pPrev = pProcess;
        pProcess->pPrev = NULL;
        gpSPMHdr->apHeads[__LIBC_PROCSTATE_ALIVE] = pProcess;

        gpSPMSelf = pProcess;
    }

    LIBCLOG_RETURN_P(pProcess);
}


/**
 * Allocates a process.
 * @internal
 */
static __LIBC_PSPMPROCESS spmAllocProcess(void)
{
    LIBCLOG_ENTER("\n");
    __LIBC_PSPMPROCESS pProcess = gpSPMHdr->apHeads[__LIBC_PROCSTATE_FREE];
    if (pProcess)
    {
        if (pProcess->pNext)
            pProcess->pNext->pPrev = NULL;
        gpSPMHdr->apHeads[__LIBC_PROCSTATE_FREE] = pProcess->pNext;
        bzero(pProcess, gpSPMHdr->cbProcess);
    }
    else
    {
        /* allocate a new process block. */
        pProcess = spmAlloc(gpSPMHdr->cbProcess);
        if (!pProcess)
        {
            /*
             * Hmm. really low on memory, we'll take the oldest zombie if we got any.
             */
            if (!gpSPMHdr->apHeads[__LIBC_PROCSTATE_ZOMBIE])
            {
                LIBC_ASSERTM_FAILED("shared memory exausted!!!\n");
                LIBCLOG_RETURN_P(NULL);
            }

            /* unlink last zombie. */
            pProcess = gpSPMHdr->apHeads[__LIBC_PROCSTATE_ZOMBIE];
            while (pProcess->pNext)
                 pProcess = pProcess->pNext;
            if (pProcess->pPrev)
                pProcess->pPrev->pNext = NULL;
            else
                gpSPMHdr->apHeads[__LIBC_PROCSTATE_ZOMBIE] = NULL;
            LIBCLOG_MSG("Reaping zombie %p (pid=%#x pidParent=%#x cRefrences=%d)\n",
                        (void *)pProcess, pProcess->pid, pProcess->pidParent, pProcess->cReferences);
        }
        bzero(pProcess, gpSPMHdr->cbProcess);
    }

    LIBCLOG_RETURN_P(pProcess);
}


/**
 * Unlink and frees a given process and it's associated data.
 *
 * @internal
 */
static void spmFreeProcess(__LIBC_PSPMPROCESS pProcess)
{
    LIBCLOG_ENTER("pProcess=%p:{pid=%#x pidParent=%#x cReferences=%d enmState=%d pvForkHandle=%p pNext=%p pPrev=%p}\n",
                  (void *)pProcess, pProcess->pid, pProcess->pidParent, pProcess->cReferences, pProcess->enmState,
                  pProcess->pvForkHandle, (void *)pProcess->pNext, (void *)pProcess->pPrev);
    /*
     * Free inheritance info.
     */
    void *pv = pProcess->pInheritLocked;
    if (!pv)
        pv = pProcess->pInherit;
    if (pv)
    {
        pProcess->pInherit = NULL;
        pProcess->pInheritLocked = NULL;
        spmFree(pv);
    }

    /*
     * Free signals.
     */
    __LIBC_PSPMSIGNAL pSig = pProcess->pSigHead;
    pProcess->pSigHead = NULL;
    while (pSig)
    {
        __LIBC_PSPMSIGNAL pFree = pSig;
        pSig = pSig->pNext;
        spmFreeSignal(pFree);
    }

    /*
     * Free child notifications.
     */
    __LIBC_PSPMCHILDNOTIFY pNotify = pProcess->pChildNotifyHead;
    pProcess->pChildNotifyHead = NULL;
    while (pNotify)
    {
        __LIBC_PSPMCHILDNOTIFY pFree = pNotify;
        pNotify = pNotify->pNext;
        spmFreeChildNotify(pFree);
    }

    /*
     * Parent notification.
     */
    if (pProcess->pTerm)
    {
        spmFreeChildNotify(pProcess->pTerm);
        pProcess->pTerm = NULL;
    }

    /*
     * Free pool pointers.
     */
    unsigned    c = pProcess->cPoolPointers;
    void **ppv = (void **)(&pProcess->cPoolPointers + 1);
    while (c-- > 0)
    {
        if (*ppv)
        {
            spmFree(*ppv);
            *ppv = NULL;
        }
        ppv++;
    }

    /*
     * Unlink it.
     */
    if (pProcess->pNext)
        pProcess->pNext->pPrev = pProcess->pPrev;
    if (pProcess->pPrev)
        pProcess->pPrev->pNext = pProcess->pNext;
    else
        gpSPMHdr->apHeads[pProcess->enmState] = pProcess->pNext;

    /*
     * Mark free and put in the free list.
     */
    pProcess->enmState = __LIBC_PROCSTATE_FREE;
    pProcess->pNext    = gpSPMHdr->apHeads[__LIBC_PROCSTATE_FREE];
    if (pProcess->pNext)
        pProcess->pNext->pPrev = pProcess;
    pProcess->pPrev    = NULL;
    gpSPMHdr->apHeads[__LIBC_PROCSTATE_FREE] = pProcess;

    LIBCLOG_RETURN_VOID();
}


/**
 * Allocate a suicide note, erm, child termination notification.
 *
 * @returns Pointer to allocated notifcation struct. The cb member is filled in.
 * @returns NULL on failure.
 */
static __LIBC_PSPMCHILDNOTIFY spmAllocChildNotify(void)
{
    LIBCLOG_ENTER("\n");

    /*
     * Search the free list for a node of matching size.
     */
    __LIBC_PSPMCHILDNOTIFY pPrev = NULL;
    __LIBC_PSPMCHILDNOTIFY pNotify = gpSPMHdr->pChildNotifyFreeHead;
    while (pNotify)
    {
        if (pNotify->cb == sizeof(*pNotify))
        {
            /* unlink and return. */
            if (pPrev)
                pPrev->pNext = pNotify->pNext;
            else
                gpSPMHdr->pChildNotifyFreeHead = pNotify->pNext;
            pNotify->pNext = NULL;
            LIBCLOG_RETURN_P(pNotify);
        }
        /* next */
        pPrev = pNotify;
        pNotify = pNotify->pNext;
    }

    /*
     * Restore to allocating one from the pool.
     */
    pNotify = spmAlloc(sizeof(*pNotify));
    if (pNotify)
    {
        pNotify->pNext = NULL;
        pNotify->cb = sizeof(*pNotify);
    }

    LIBCLOG_RETURN_P(pNotify);
}


/**
 * Free a child (termination) notification.
 *
 * @param pNotify       The notification structure to free.
 */
static void spmFreeChildNotify(__LIBC_PSPMCHILDNOTIFY pNotify)
{
    LIBCLOG_ENTER("pNotify=%p\n", (void *)pNotify);
    pNotify->pNext = gpSPMHdr->pChildNotifyFreeHead;
    gpSPMHdr->pChildNotifyFreeHead = pNotify;
    LIBCLOG_RETURN_VOID();
}


/**
 * Queues a signal on a process.
 *
 * @returns 0 on success.
 * @returns Negative error code (errno.h) on failure.
 * @param   pProcess        Process to queue the signal on.
 * @param   iSignalNo       Signal to send. If 0 only permission checks are performed.
 * @param   pSigInfo         Signal to queue. If NULL only permission checks are performed.
 * @param   fQueued         Set if the signal type is queued.
 * @param   fQueueAnyway    If set the normal signal limits are bypassed.
 */
static int spmSigQueueProcess(__LIBC_PSPMPROCESS pProcess, int iSignalNo, siginfo_t *pSigInfo, int fQueued, int fQueueAnyway)
{
    LIBCLOG_ENTER("pProcess=%p {.pid=%#x, .pgrp=%#x, .sid=%#x,...} iSignalNo=%d pSigInfo=%p fQueue=%d fQueueAnyway=%d\n",
                  (void *)pProcess, pProcess->pid, pProcess->pgrp, pProcess->sid, iSignalNo, (void *)pSigInfo, fQueued, fQueueAnyway);

    /*
     * Check permissions.
     * (This is close, but not quite it.)
     */
    if (    pProcess != gpSPMSelf                                                   /* our selves are ok. */
        &&  pProcess->euid != 0                                                     /* Root can do anything. */
        &&  (iSignalNo != SIGCONT || pProcess->sid != gpSPMSelf->sid)               /* SIGCONT to everyone in the session. */
        &&  pProcess->uid != gpSPMSelf->euid && pProcess->svuid != gpSPMSelf->euid  /* Same as our efficient uid is ok. */
        &&  pProcess->uid != gpSPMSelf->uid  && pProcess->svuid != gpSPMSelf->uid   /* Same as our uid is ok. */
        )
        LIBCLOG_RETURN_INT(-EPERM);

    /* This could be a permission check call, if so, we're done now. */
    if (!pSigInfo || !iSignalNo)
        LIBCLOG_RETURN_INT(0);

    /*
     * Check if already pending.
     */
    __LIBC_PSPMSIGNAL   pSig = NULL;
    if (!fQueued)
    {
        pSig = pProcess->pSigHead;
        int iSignal = pSigInfo->si_signo;
        for (pSig = pProcess->pSigHead; pSig; pSig = pSig->pNext)
            if (pSig->Info.si_signo == iSignal)
                break;
    }
    int rc = 0;
    if (!pSig)
    {
        /*
         * Check that we're not exceeding any per process or system limits (unless told to do so).
         */
        if (    (   gpSPMHdr->cSigActive < gpSPMHdr->cSigMaxActive
                 && gpSPMSelf->cSigsSent < _POSIX_SIGQUEUE_MAX)
            || fQueueAnyway
            || pSigInfo->si_signo == SIGCHLD)
        {
            /*
             * Allocate a signal packet.
             */
            pSig = spmAllocSignal();
            if (pSig)
            {
                /*
                 * Copy the data and insert it into the queue.
                 */
                pSig->pNext     = NULL;
                pSig->pidSender = gpSPMSelf->pid;
                pSig->Info      = *pSigInfo;
                if (fQueued)
                    pSig->Info.si_flags |= __LIBC_SI_QUEUED;
                else
                    pSig->Info.si_flags &= ~__LIBC_SI_QUEUED;
                if (!pSig->Info.si_pid)
                    pSig->Info.si_pid = gpSPMSelf->pid;
                if (!pSig->Info.si_pgrp)
                    pSig->Info.si_pgrp = gpSPMSelf->pgrp;
                if (!pSig->Info.si_uid)
                    pSig->Info.si_uid = gpSPMSelf->uid;

                /* insert (FIFO) */
                if (!pProcess->pSigHead)
                    pProcess->pSigHead = pSig;
                else
                {
                    __LIBC_PSPMSIGNAL pSigLast = pProcess->pSigHead;
                    while (pSigLast->pNext)
                        pSigLast = pSigLast->pNext;
                    pSigLast->pNext = pSig;
                }

                /* update statistics */
                if (pProcess != gpSPMSelf)
                    gpSPMSelf->cSigsSent++;
                gpSPMHdr->cSigActive++;

                /* post notification sem. */
                LIBCLOG_MSG("posting %#lx\n", gpSPMHdr->hevNotify);
                APIRET rc2 = DosPostEventSem(gpSPMHdr->hevNotify);
                LIBC_ASSERTM(!rc2 || rc2 == ERROR_ALREADY_POSTED, "rc2=%ld!\n", rc2); rc2 = rc2;
            }
            else
            {
                LIBCLOG_MSG("Out of memory! Cannot allocate %d bytes for a signal packet!\n", sizeof(*pSig));
                rc = -EAGAIN;
            }
        }
        else
        {
            LIBCLOG_MSG("Limit reached: cSigActive=%d cSigMaxActive=%d cSigsSent=%d (max %d)\n",
                        gpSPMHdr->cSigActive, gpSPMHdr->cSigMaxActive, pProcess->cSigsSent, _POSIX_SIGQUEUE_MAX);
            rc = -EAGAIN;
        }
    }
    else
        LIBCLOG_MSG("Signal %d is already pending on pid %#x (%d). (ts=%x,pid=%#x (%d))\n",
                    pSigInfo->si_signo, pProcess->pid, pProcess->pid, pSig->Info.si_timestamp, pSig->Info.si_pid, pSig->Info.si_pid);
    LIBCLOG_RETURN_INT(rc);
}


/**
 * Allocates a signal structure.
 *
 * @returns Pointer to allocated signal structure.
 * @returns NULL on failure.
 */
static __LIBC_PSPMSIGNAL spmAllocSignal(void)
{
    LIBCLOG_ENTER("\n");

    /*
     * Old structures.
     */
    __LIBC_PSPMSIGNAL pSig;
    if (gpSPMHdr->pSigFreeHead)
    {
        __LIBC_PSPMSIGNAL pPrev = NULL;
        for (pSig = gpSPMHdr->pSigFreeHead; pSig; pPrev = pSig, pSig = pSig->pNext)
        {
            if (pSig->cb == sizeof(*pSig))
            {
                if (pPrev)
                    pPrev->pNext = pSig->pNext;
                else
                    gpSPMHdr->pSigFreeHead = pSig->pNext;
                gpSPMHdr->cSigFree--;
                LIBCLOG_RETURN_P(pSig);
            }
        }
    }

    /*
     * New structure.
     */
    pSig = spmAlloc(sizeof(*pSig));
    if (pSig)
        pSig->cb = sizeof(*pSig);

    LIBCLOG_RETURN_P(pSig);
}


/**
 * Frees a signal
 *
 * @param   pSig        Signal structure to free.
 */
static void spmFreeSignal(__LIBC_PSPMSIGNAL pSig)
{
    LIBCLOG_ENTER("pSig=%p {.pidSender=%#x}\n", (void *)pSig, pSig->pidSender);

    /*
     * If it got a sender, update it's statistics.
     */
    if (!pSig->pidSender && gpSPMSelf->pid != pSig->pidSender)
    {
        __LIBC_PSPMPROCESS pSender = spmQueryProcessInState(pSig->pidSender, __LIBC_PROCSTATE_ALIVE);
        if (pSender)
            pSender->cSigsSent--;
    }

    /*
     * Decide whether to put it in the queue of free signals
     * or to return it to the memory pool.
     */
    if (gpSPMHdr->cSigFree < 32)
    {
        pSig->pNext = gpSPMHdr->pSigFreeHead;
        gpSPMHdr->pSigFreeHead = pSig;
        gpSPMHdr->cSigFree++;
    }
    else
        spmFree(pSig);
    LIBCLOG_RETURN_VOID();
}


/**
 * Allocates the per process socket reference map.
 *
 * @returns 0 on success.
 * @returns Negative errno on failure.
 */
static int spmSocketAllocProcess(void)
{
    /*
     * Obtain the mutex.
     */
    __LIBC_SPMXCPTREGREC    RegRec;
    int rc = spmRequestMutex(&RegRec);
    if (!rc)
    {
        /*
         * Before allocating the reference map, check if someone got here before us.
         */
        if (!gpSPMSelf->pacTcpipRefs)
        {
            FS_VAR_SAVE_LOAD();
            PVOID pv = NULL;
            rc = DosAllocMem(&pv, 0x10000 * sizeof(uint16_t), PAG_READ | PAG_WRITE | PAG_COMMIT | OBJ_ANY);
            if (rc) /* and once again for the pre fp13 kernels. */
                rc = DosAllocMem(&pv, 0x10000 * sizeof(uint16_t), PAG_READ | PAG_WRITE | PAG_COMMIT);
            FS_RESTORE();
            if (!rc)
            {
                /*
                 * Update the self structure.
                 */
                gpSPMSelf->pacTcpipRefs = (uint16_t *)pv;
            }
            else
                rc = -ENOMEM;
        }
        spmReleaseMutex(&RegRec);
    }

    return rc;
}


/**
 * Allocate memory in SPM pool of given size.
 *
 * @returns address of memory on success.
 * @returns NULL on failure.
 * @param   cbSize 	Size of memory to allocate.
 * @internal
 */
static void * spmAlloc(size_t cbSize)
{
    LIBCLOG_ENTER("cbSize=%d\n", cbSize);
    void    *pv;

    /*
     * Validate input.
     */
    if (!cbSize)
    {
        LIBCLOG_MSG("Invalid size\n");
        LIBCLOG_RETURN_P(NULL);
    }

    /* enforce alignment. */
    if (cbSize <= SPM_POOL_ALIGN(sizeof(__LIBC_SPMPOOLCHUNKFREE) - sizeof(__LIBC_SPMPOOLCHUNK)))
        cbSize = SPM_POOL_ALIGN(sizeof(__LIBC_SPMPOOLCHUNKFREE) - sizeof(__LIBC_SPMPOOLCHUNK));
    else
        cbSize = SPM_POOL_ALIGN(cbSize);

    /*
     * Attempt an allocation.
     */
    pv = spmAllocSub(cbSize);
    if (!pv)
    {
        unsigned            uTimestamp;
        __LIBC_PSPMPROCESS  pProcess;
        LIBCLOG_MSG("SPM IS LOW ON MEMORY!!! cbFree=%d cb=%d (cbSize=%d)\n", gpSPMHdr->cbFree, gpSPMHdr->cb, cbSize);
        spmCheck(0, 1);

        /*
         * Free embryos which are more than 5 min old or which processes no longer exists.
         */
        uTimestamp = spmTimestamp();
        pProcess = gpSPMHdr->apHeads[__LIBC_PROCSTATE_EMBRYO];
        if (pProcess)
        {
            do
            {
                SPM_ASSERT_PTR_NULL(pProcess);
                SPM_ASSERT_PTR_NULL(pProcess->pNext);
                SPM_ASSERT_PTR_NULL(pProcess->pPrev);
                if (    pProcess->cReferences == 0
                    &&  (   uTimestamp - pProcess->uTimestamp >= 5*60*1000
                         || (   pProcess->pid != -1
                             && DosVerifyPidTid(pProcess->pid, 1) == ERROR_INVALID_PROCID)
                        )
                    )
                {
                    __LIBC_PSPMPROCESS      pProcessNext = pProcess->pNext;
                    LIBCLOG_MSG("Reaping embryo %p pidParent=%#x pid=%#x pForkHandle=%p uTimestamp=%08x (now=%08x) pNext=%p pPrev=%p\n",
                                (void *)pProcess, pProcess->pidParent, pProcess->pid, pProcess->pvForkHandle, pProcess->uTimestamp, uTimestamp,
                                (void *)pProcess->pNext, (void *)pProcess->pPrev);
                    spmFreeProcess(pProcess);
                    pProcess = pProcessNext;

                    /* Wake up the embryo waiters (paranoia). */
                    LIBCLOG_MSG("posting %#lx\n", gpSPMHdr->hevNotify);
                    APIRET rc2 = DosPostEventSem(gpSPMHdr->hevNotify);
                    LIBC_ASSERTM(!rc2 || rc2 == ERROR_ALREADY_POSTED, "rc2=%ld!\n", rc2); rc2 = rc2;
                    continue;
                }

                /* next */
                pProcess = pProcess->pNext;
            } while (pProcess);
        }

        /*
         * Free up free processes.
         */
        while ((pProcess = gpSPMHdr->apHeads[__LIBC_PROCSTATE_FREE]) != NULL)
        {
            SPM_ASSERT_PTR_NULL(pProcess);
            SPM_ASSERT_PTR_NULL(pProcess->pNext);
            SPM_ASSERT_PTR_NULL(pProcess->pPrev);
            LIBC_ASSERTM(pProcess->enmState == __LIBC_PROCSTATE_FREE, "enmState=%d pProcess=%p\n",
                         pProcess->enmState, (void *)pProcess);
            LIBCLOG_MSG("Reaping free process %p (pid=%#x pidParent=%#x)\n",
                        (void *)pProcess, pProcess->pid, pProcess->pidParent);

            gpSPMHdr->apHeads[__LIBC_PROCSTATE_FREE] = pProcess->pNext;
            if (pProcess->pNext)
                pProcess->pNext->pPrev = NULL;
            pProcess->pNext = NULL;
            pProcess->pPrev = NULL;
            pProcess->enmState = ~0;
            spmFree(pProcess);
        }

        /*
         * Free up inherit data of processes which have been around for more than 5 min.
         */
        for (pProcess = gpSPMHdr->apHeads[__LIBC_PROCSTATE_ALIVE]; pProcess; pProcess = pProcess->pNext)
        {
            SPM_ASSERT_PTR_NULL(pProcess);
            SPM_ASSERT_PTR_NULL(pProcess->pNext);
            SPM_ASSERT_PTR_NULL(pProcess->pPrev);
            if (    pProcess->pInherit
                &&  uTimestamp - pProcess->uTimestamp >= 5*60*1000)
            {
                void *pv = (void *)__atomic_xchg((unsigned *)(void *)&gpSPMSelf->pInherit, 0);
                LIBCLOG_MSG("Reaping inherit data (%p) of process %p (pid=%#x pidParent=%#x)\n",
                            pv, (void *)pProcess, pProcess->pid, pProcess->pidParent);
                spmFree(pv);
            }
        }

        /*
         * Free up termination structures.
         */
        __LIBC_PSPMCHILDNOTIFY pNotify;
        while ((pNotify = gpSPMHdr->pChildNotifyFreeHead) != NULL)
        {
            gpSPMHdr->pChildNotifyFreeHead = pNotify->pNext;
            LIBCLOG_MSG("Reaping notification record %p (cb=%d pid=%#x iExitCode=%d enmDeathReason=%d)\n",
                        (void *)pNotify, pNotify->cb, pNotify->pid, pNotify->iExitCode, pNotify->enmDeathReason);
            pNotify->pNext = NULL;
            pNotify->enmDeathReason = __LIBC_EXIT_REASON_NONE;
            spmFree(pNotify);
        }

        /*
         * Retry allocation.
         */
        pv = spmAllocSub(cbSize);
        if (!pv)
        {
            LIBCLOG_MSG("SPM IS *STILL* LOW ON MEMORY!!! cbFree=%d cb=%d (cbSize=%d)\n", gpSPMHdr->cbFree, gpSPMHdr->cb, cbSize);

            /*
             * Free embryos which are more than 15 seconds old.
             */
            uTimestamp = spmTimestamp();
            pProcess = gpSPMHdr->apHeads[__LIBC_PROCSTATE_EMBRYO];
            if (pProcess)
            {
                do
                {
                    SPM_ASSERT_PTR_NULL(pProcess);
                    SPM_ASSERT_PTR_NULL(pProcess->pNext);
                    SPM_ASSERT_PTR_NULL(pProcess->pPrev);
                    if (    pProcess->cReferences == 0
                        &&  uTimestamp - pProcess->uTimestamp >= 15*1000)
                    {
                        __LIBC_PSPMPROCESS      pProcessNext = pProcess->pNext;
                        LIBCLOG_MSG("Reaping embryo %p pidParent=%#x pid=%#x pForkHandle=%p uTimestamp=%08x (now=%08x) pNext=%p pPrev=%p\n",
                                    (void *)pProcess, pProcess->pidParent, pProcess->pid, pProcess->pvForkHandle, pProcess->uTimestamp, uTimestamp,
                                    (void *)pProcess->pNext, (void *)pProcess->pPrev);
                        spmFreeProcess(pProcess);
                        pProcess = pProcessNext;

                        /* Wake up the embryo waiters (paranoia). */
                        LIBCLOG_MSG("posting %#lx\n", gpSPMHdr->hevNotify);
                        APIRET rc2 = DosPostEventSem(gpSPMHdr->hevNotify);
                        LIBC_ASSERTM(!rc2 || rc2 == ERROR_ALREADY_POSTED, "rc2=%ld!\n", rc2); rc2 = rc2;
                        continue;
                    }

                    /* next */
                    pProcess = pProcess->pNext;
                } while (pProcess);
            }

            /*
             * Free up inherit data of processes which have been around for more than 15 seconds.
             */
            for (pProcess = gpSPMHdr->apHeads[__LIBC_PROCSTATE_ALIVE]; pProcess; pProcess = pProcess->pNext)
            {
                SPM_ASSERT_PTR_NULL(pProcess);
                SPM_ASSERT_PTR_NULL(pProcess->pNext);
                SPM_ASSERT_PTR_NULL(pProcess->pPrev);
                if (    pProcess->pInherit
                    &&  uTimestamp - pProcess->uTimestamp >= 15*1000)
                {
                    void *pv = (void *)__atomic_xchg((unsigned *)(void *)&gpSPMSelf->pInherit, 0);
                    LIBCLOG_MSG("Reaping inherit data (%p) of process %p (pid=%#x pidParent=%#x)\n",
                                pv, (void *)pProcess, pProcess->pid, pProcess->pidParent);
                    spmFree(pv);
                }
            }

            /*
             * Free preallocated signals.
             */
            while (gpSPMHdr->cSigFree > 4 && gpSPMHdr->pSigFreeHead)
            {
                __LIBC_PSPMSIGNAL pSig = gpSPMHdr->pSigFreeHead;
                gpSPMHdr->pSigFreeHead = pSig->pNext;
                gpSPMHdr->cSigFree--;
                pSig->pNext = NULL;
                spmFree(pSig);
            }
        }
    }

    LIBCLOG_RETURN_P(pv);
}

/**
 * Allocate memory in SPM pool of given size.
 * The size is aligned and checked by the caller.
 *
 * @returns address of memory on success.
 * @returns NULL on failure.
 * @param   cbSize 	Size of memory to allocate.
 * @internal
 */
static void *spmAllocSub(size_t cbSize)
{
    LIBCLOG_ENTER("cbSize=%d\n", cbSize);
    __LIBC_PSPMPOOLCHUNKFREE pFree;
    void   *pvRet;

    /*
     * We allocate process sized chunks from one end and the
     * variable sized ones from the other end of the shared memory.
     * This should have a positive effect on the fragmentation as well
     * as how quickly we'll find a suitable free node when allocating
     * process sized chunks.
     *
     * Looking at the code, the allocation from the end is simpler
     * so we'll do the process sized chunks from that end.
     */
    pvRet = NULL;
    if (cbSize != gpSPMHdr->cbProcess)
    {
        /*
         * All but process sized chunks from the head.
         */
        pFree = gpSPMHdr->pPoolFreeHead;
        while (pFree)
        {
            SPM_ASSERT_PTR_NULL(pFree->pNext);
            SPM_ASSERT_PTR_NULL(pFree->pPrev);
            SPM_ASSERT_PTR_NULL(pFree->core.pNext);
            SPM_ASSERT_PTR_NULL(pFree->core.pPrev);
            if (pFree->cb >= cbSize)
            {
                /*
                 * Split of a new free chunk?
                 */
                if (pFree->cb >= cbSize + SPM_POOL_ALIGN(sizeof(__LIBC_SPMPOOLCHUNKFREE)))
                {
                    /* create the new pool free chunk at end unlinking pFree in the process. */
                    __LIBC_PSPMPOOLCHUNKFREE pNew = (__LIBC_PSPMPOOLCHUNKFREE)((char *)(&pFree->core + 1) + cbSize);
                    SPM_ASSERT_PTR_NULL(pNew);
                    pNew->core.pPrev  = &pFree->core;
                    pNew->core.pNext  = pFree->core.pNext;
                    pFree->core.pNext = &pNew->core;
                    if (pNew->core.pNext)
                        pNew->core.pNext->pPrev = &pNew->core;
                    else
                        gpSPMHdr->pPoolTail     = &pNew->core;
                    SPM_ASSERT_PTR_NULL(pNew->core.pNext);
                    SPM_ASSERT_PTR_NULL(pNew->core.pPrev);
                    SPM_ASSERT_PTR_NULL(pFree->core.pNext);
                    SPM_ASSERT_PTR_NULL(pFree->core.pPrev);

                    pNew->cb = pFree->cb - ((uintptr_t)pNew - (uintptr_t)pFree);
                    pNew->pNext = pFree->pNext;
                    if (pNew->pNext)
                        pNew->pNext->pPrev      = pNew;
                    else
                        gpSPMHdr->pPoolFreeTail = pNew;
                    pNew->pPrev = pFree->pPrev;
                    if (pNew->pPrev)
                        pNew->pPrev->pNext      = pNew;
                    else
                        gpSPMHdr->pPoolFreeHead = pNew;

                    gpSPMHdr->cbFree -= cbSize + sizeof(__LIBC_SPMPOOLCHUNK);
                    pvRet = &pFree->core + 1;
                }
                else
                {
                    /* Link out of free list. */
                    if (pFree->pNext)
                        pFree->pNext->pPrev     = pFree->pPrev;
                    else
                        gpSPMHdr->pPoolFreeTail = pFree->pPrev;
                    if (pFree->pPrev)
                        pFree->pPrev->pNext     = pFree->pNext;
                    else
                        gpSPMHdr->pPoolFreeHead = pFree->pNext;

                    gpSPMHdr->cbFree -= pFree->cb;
                    pvRet = &pFree->core + 1;
                }
                break;
            }
            pFree = pFree->pNext;
        }
    }
    else
    {
        /*
         * Process sized blocks from the end.
         */
        pFree = gpSPMHdr->pPoolFreeTail;
        while (pFree)
        {
            SPM_ASSERT_PTR_NULL(pFree->pNext);
            SPM_ASSERT_PTR_NULL(pFree->pPrev);
            SPM_ASSERT_PTR_NULL(pFree->core.pNext);
            SPM_ASSERT_PTR_NULL(pFree->core.pPrev);
            if (pFree->cb >= cbSize)
            {
                /*
                 * Split of a new free chunk?
                 */
                if (pFree->cb >= cbSize + SPM_POOL_ALIGN(sizeof(__LIBC_SPMPOOLCHUNKFREE)))
                {
                    /* create the new pool chunks at end, update the size of the free chunk. */
                    __LIBC_PSPMPOOLCHUNK pNew;
                    pFree->cb -= cbSize + sizeof(__LIBC_SPMPOOLCHUNK);
                    pNew = (__LIBC_PSPMPOOLCHUNK)((char *)pFree + pFree->cb + sizeof(__LIBC_SPMPOOLCHUNK));
                    pNew->pPrev = &pFree->core;
                    pNew->pNext = pFree->core.pNext;
                    pFree->core.pNext = pNew;
                    if (pNew->pNext)
                        pNew->pNext->pPrev  = pNew;
                    else
                        gpSPMHdr->pPoolTail = pNew;

                    gpSPMHdr->cbFree -= cbSize + sizeof(__LIBC_SPMPOOLCHUNK);
                    pvRet = pNew + 1;
                }
                else
                {
                    /* Link out of free list. */
                    if (pFree->pNext)
                        pFree->pNext->pPrev     = pFree->pPrev;
                    else
                        gpSPMHdr->pPoolFreeTail = pFree->pPrev;
                    if (pFree->pPrev)
                        pFree->pPrev->pNext     = pFree->pNext;
                    else
                        gpSPMHdr->pPoolFreeHead = pFree->pNext;

                    gpSPMHdr->cbFree -= pFree->cb;
                    pvRet = &pFree->core + 1;
                }
                break;
            }

            /* walk */
            pFree = pFree->pPrev;
        }
    }

    LIBCLOG_RETURN_P(pvRet);
}


/**
 * Internal free which works from within the semaphore.
 *
 * @returns 0 on success.
 * @returns -1 on failure.
 * @param   pv  Pointer to free.
 * @internal
 */
static int spmFree(void *pv)
{
    LIBCLOG_ENTER("pv=%p\n", pv);
    __LIBC_PSPMPOOLCHUNKFREE    pFree;
    __LIBC_PSPMPOOLCHUNKFREE    pLeft;
    __LIBC_PSPMPOOLCHUNKFREE    pRight;

    /*
     * Validate.
     */
    if (!pv)
        LIBCLOG_RETURN_INT(0);
    if (!SPM_VALID_PTR(pv) || SPM_POOL_ALIGN((uintptr_t)pv) != (uintptr_t)pv)
    {
        LIBC_ASSERTM_FAILED("Invalid pointer %p\n", pv);
        errno = EINVAL;
        LIBCLOG_RETURN_INT(-1);
    }

    /*
     * Insert into the free list (which is sorted on address).
     * This is slower than one would like, but it's the price to pay for
     * having an 8 byte header instead of a 16 byte header (when aligned).
     *
     * For processes we are generally not freeing them, but recycling them
     * so allocating and freeing those is very efficient.
     */
    pFree = (__LIBC_PSPMPOOLCHUNKFREE)((__LIBC_PSPMPOOLCHUNK)pv - 1);
    SPM_ASSERT_PTR_NULL(pFree->core.pNext);
    SPM_ASSERT_PTR_NULL(pFree->core.pPrev);
    pLeft = gpSPMHdr->pPoolFreeTail;
    while (pFree < pLeft)
    {
        SPM_ASSERT_PTR_NULL(pLeft);
        SPM_ASSERT_PTR_NULL(pLeft->pNext);
        pLeft = pLeft->pPrev;
    }
    /* (pLeft is the node before pFree) */
    SPM_ASSERT_PTR_NULL(pLeft);

    if (pLeft == pFree)
    {
        LIBC_ASSERTM_FAILED("Freed twice! pv=%pv\n", pv);
        errno = EINVAL;
        LIBCLOG_RETURN_INT(-1);
    }

    if (!pLeft)
    {   /* we're head free chunk. */
        pFree->pPrev = NULL;
        pRight = pFree->pNext = gpSPMHdr->pPoolFreeHead;
        if (pRight)
            pRight->pPrev = pFree;
        gpSPMHdr->pPoolFreeHead = pFree;
    }
    else
    {
        SPM_ASSERT_PTR_NULL(pLeft->pNext);
        SPM_ASSERT_PTR_NULL(pLeft->pPrev);
        SPM_ASSERT_PTR_NULL(pLeft->core.pNext);
        SPM_ASSERT_PTR_NULL(pLeft->core.pPrev);

        if ((__LIBC_PSPMPOOLCHUNK)pLeft == pFree->core.pPrev)
        {   /* merge with left free chunk. */
            pLeft->core.pNext = pFree->core.pNext;
            if (pLeft->core.pNext)
                pLeft->core.pNext->pPrev = &pLeft->core;
            else
                gpSPMHdr->pPoolTail      = &pLeft->core;
            gpSPMHdr->cbFree -= pLeft->cb;
            pFree = pLeft;
            pRight = pLeft->pNext;
        }
        else
        {   /* link into the free list then. */
            pFree->pPrev = pLeft;
            pFree->pNext = pLeft->pNext;
            pLeft->pNext = pFree;
            if (pFree->pNext)
                pFree->pNext->pPrev     = pFree;
            else
                gpSPMHdr->pPoolFreeTail = pFree;
            pRight = pFree->pNext;
        }
    }

    /* Check if we can merge with right hand free chunk. */
    if (pRight && (__LIBC_PSPMPOOLCHUNK)pRight == pFree->core.pNext)
    {   /* merge with right free chunk. */
        pFree->core.pNext = pRight->core.pNext;
        if (pFree->core.pNext)
            pFree->core.pNext->pPrev = &pFree->core;
        else
            gpSPMHdr->pPoolTail      = &pFree->core;

        pFree->pNext = pRight->pNext;
        if (pFree->pNext)
            pFree->pNext->pPrev      = pFree;
        else
            gpSPMHdr->pPoolFreeTail  = pFree;
        gpSPMHdr->cbFree -= pRight->cb;
    }

    /* calculate the size. */
    pFree->cb = (pFree->core.pNext ? (uintptr_t)pFree->core.pNext : (uintptr_t)gpSPMHdr + gpSPMHdr->cb) - (uintptr_t)(&pFree->core + 1);
    gpSPMHdr->cbFree += pFree->cb;

    SPM_ASSERT_PTR_NULL(pFree->pNext);
    SPM_ASSERT_PTR_NULL(pFree->pPrev);
    SPM_ASSERT_PTR_NULL(pFree->core.pNext);
    SPM_ASSERT_PTR_NULL(pFree->core.pPrev);

    LIBCLOG_RETURN_INT(0);
}




/**
 * Exception handle use while owning the SPM data.
 *
 * @returns See cpref.
 * @param   pRepRec     Exception report. See cpref for details.
 * @param   pRegRec     Pointer to our __LIBC_SPMXCPTREGREC structure.
 * @param   pCtx        CPU context. See cpref for details.
 * @param   pvWhatever  Nobody knows.
 */
static ULONG _System spmXcptHandler(PEXCEPTIONREPORTRECORD pRepRec, PEXCEPTIONREGISTRATIONRECORD pRegRec, PCONTEXTRECORD pCtx, PVOID pvWhatever)
{
    PPIB    pPib = NULL;
    PTIB    pTib = NULL;
    PID     pid;
    TID     tid;
    ULONG   cNesting;
    int     rc;

    LIBCLOG_MSG2("!!! SPM: num %08lx flags %08lx nested %p whatever %p\n",
                 pRepRec->ExceptionNum, pRepRec->fHandlerFlags,
                 (void *)pRepRec->NestedExceptionReportRecord, pvWhatever);
    /*
     * Skip unwinding exceptions.
     */
    if (pRepRec->fHandlerFlags & (EH_UNWINDING | EH_EXIT_UNWIND))
        return XCPT_CONTINUE_SEARCH;

    /*
     * Switch out and see what went wrong.
     */
    switch (pRepRec->ExceptionNum)
    {
        /*
         * This isn't wrong, just unlikely and must be ignored!
         */
        case XCPT_ASYNC_PROCESS_TERMINATE:
        case XCPT_PROCESS_TERMINATE:
        case XCPT_SIGNAL:
        {
            PPIB    pPib;
            PTIB    pTib;
            PID     pid;
            TID     tid;
            ULONG   cNesting;
            int     rc;
            DosGetInfoBlocks(&pTib, &pPib);
            rc = DosQueryMutexSem(ghmtxSPM, &pid, &tid, &cNesting);
            if (!rc && pPib->pib_ulpid == pid && pTib->tib_ptib2->tib2_ultid == tid)
                return XCPT_CONTINUE_EXECUTION;
            return XCPT_CONTINUE_SEARCH;
        }

        /*
         * This is serious stuff.
         */
        case XCPT_ACCESS_VIOLATION:
        {
            const char *psz = "???";
            switch (pRepRec->ExceptionInfo[0])
            {
                case XCPT_WRITE_ACCESS:     psz = "Write"; break;
                case XCPT_READ_ACCESS:      psz = "Read"; break;
                case XCPT_EXECUTE_ACCESS:   psz = "Exec"; break;
                case XCPT_UNKNOWN_ACCESS:   psz = "Unknown"; break;
            }
            LIBCLOG_REL("SPM Exception! Access violation. %s access. Address=%lx (info[1]).\n",
                        psz, pRepRec->ExceptionInfo[1]);
            break;
        }

        case XCPT_INTEGER_DIVIDE_BY_ZERO:
            LIBCLOG_REL("SPM Exception! divide by zero.\n");
            break;

        case XCPT_BREAKPOINT:
            LIBCLOG_REL("SPM Exception! breakpoint (assertion).\n");
            break;

        default:
            return XCPT_CONTINUE_SEARCH;
    }

    /*
     * Write context.
     */
    LIBCLOG_REL("SPM Exception! cs:eip=%04lx:%08lx  ss:esp=%04lx:%08lx\n",
                pCtx->ctx_SegCs,  pCtx->ctx_RegEip, pCtx->ctx_SegSs,  pCtx->ctx_RegEsp);
    LIBCLOG_REL("SPM Exception! eax=%08lx ebx=%08lx ecx=%08lx edx=%08lx\n",
                pCtx->ctx_RegEax, pCtx->ctx_RegEbx, pCtx->ctx_RegEcx, pCtx->ctx_RegEdx);
    LIBCLOG_REL("SPM Exception! edi=%08lx edi=%08lx ebp=%08lx efl=%08lx\n",
                pCtx->ctx_RegEdi, pCtx->ctx_RegEsi, pCtx->ctx_RegEbp, pCtx->ctx_EFlags);
    LIBCLOG_REL("SPM Exception! ds=%04lx es=%04lx fs=%04lx gs=%04lx\n",
                pCtx->ctx_SegDs, pCtx->ctx_SegEs, pCtx->ctx_SegFs, pCtx->ctx_SegGs);

    /*
     * Check up on the semaphore.
     */
    DosGetInfoBlocks(&pTib, &pPib);
    rc = DosQueryMutexSem(ghmtxSPM, &pid, &tid, &cNesting);
    if (!rc && pPib->pib_ulpid == pid && pTib->tib_ptib2->tib2_ultid == tid)
        LIBCLOG_REL("SPM Exception! Owner of the mutex!\n");
    else
        LIBCLOG_REL("SPM Exception! Not owner of the mutex! owner: tid=%lx (%ld) pid=%lx (%ld) cNesting=%ld\n",
                    tid, tid, pid, pid, cNesting);

    /*
     * Dump the memory.
     */
    spmCheck(0, 1);

#ifdef DEBUG
    /*
     * Remove the exception handler and die.
     */
    pRegRec->prev_structure = (void *)~0;
    DosUnsetExceptionHandler(pRegRec);
__asm__("int $3\n");
    spmCheck(1, 1);
#endif
    __libc_Back_panic(__LIBC_PANIC_NO_SPM_TERM, pCtx,
                      "SPM Exception %x (%x,%x,%x,%x)%s\n",
                      pRepRec->ExceptionNum,
                      pRepRec->ExceptionInfo[0],
                      pRepRec->ExceptionInfo[1],
                      pRepRec->ExceptionInfo[2],
                      pRepRec->ExceptionInfo[3],
                      !rc && pPib->pib_ulpid == pid && pTib->tib_ptib2->tib2_ultid == tid ? " - Owner of the mutex" : "");
    return XCPT_CONTINUE_SEARCH;
}


/**
 * Checks and dumps the SPM memory.
 */
static int spmCheck(int fBreakpoint, int fVerbose)
{
    int                         i;
    int                         cErrors = 0;
    __LIBC_PSPMPOOLCHUNK        pChunk;
    __LIBC_PSPMPOOLCHUNK        pChunkPrev;
    __LIBC_PSPMPOOLCHUNKFREE    pFree;
    __LIBC_PSPMPOOLCHUNKFREE    pFreePrev;
    size_t                      cbTotal;
    size_t                      cbOverhead;


#define CHECK_LOG(...) \
    do { if (fVerbose) LIBCLOG_REL(__VA_ARGS__); } while (0)
#define CHECK_FAILED(...) \
    do { CHECK_LOG(__VA_ARGS__); if (fBreakpoint) __asm__ __volatile__("int3\n"); cErrors++; } while (0)
#define CHECK_PTR(ptr, msg) \
    do { if (!SPM_VALID_PTR(ptr))      { CHECK_FAILED("Invalid pointer %p (%s). %s\n", (void *)ptr, #ptr, msg); } } while (0)
#define CHECK_PTR_NULL(ptr, msg) \
    do { if (!SPM_VALID_PTR_NULL(ptr)) { CHECK_FAILED("Invalid pointer %p (%s). %s\n", (void *)ptr, #ptr, msg); } } while (0)


    /*
     * Header.
     */
    CHECK_LOG("\n"
              "SPM Dump\n"
              "========\n"
              "\n"
              "uVersion               %#x\n"
              "cb                     %#x\n"
              "cbFree                 %#x\n"
              "pPoolHead              %p\n"
              "pPoolTail              %p\n"
              "pPoolFreeHead          %p\n"
              "pPoolFreeTail          %p\n"
              "cbProcess              %d\n"
              ,
              gpSPMHdr->uVersion,
              gpSPMHdr->cb,
              gpSPMHdr->cbFree,
              (void *)gpSPMHdr->pPoolHead,
              (void *)gpSPMHdr->pPoolTail,
              (void *)gpSPMHdr->pPoolFreeHead,
              (void *)gpSPMHdr->pPoolFreeTail,
              gpSPMHdr->cbProcess);
    for (i = 0; i < __LIBC_PROCSTATE_MAX; i++)
        CHECK_LOG("apHeads[%d]             %p\n", i, (void *)gpSPMHdr->apHeads[i]);
    CHECK_LOG("pTcpip                 %p\n"
              "pidCreate              %#x (%d)\n"
              "dtCreate               %04d-%02d-%02d %02d:%02d:%02d.%02d\n",
              (void *)gpSPMHdr->pTcpip,
              gpSPMHdr->pidCreate, gpSPMHdr->pidCreate,
              gpSPMHdr->dtCreate.year,
              gpSPMHdr->dtCreate.month,
              gpSPMHdr->dtCreate.day,
              gpSPMHdr->dtCreate.hours,
              gpSPMHdr->dtCreate.minutes,
              gpSPMHdr->dtCreate.seconds,
              gpSPMHdr->dtCreate.hundredths);

    /*
     * Validate the header.
     */
    CHECK_PTR(gpSPMHdr->pPoolHead, "");
    CHECK_PTR(gpSPMHdr->pPoolTail, "");
    CHECK_PTR_NULL(gpSPMHdr->pPoolFreeHead, "");
    CHECK_PTR_NULL(gpSPMHdr->pPoolFreeTail, "");
    CHECK_PTR_NULL(gpSPMHdr->pTcpip, "");

    /*
     * Validate the lists.
     */
    for (i = 0; i < __LIBC_PROCSTATE_MAX; i++)
    {
        __LIBC_PSPMPROCESS  pProcess;
        __LIBC_PSPMPROCESS  pProcessLast;
        unsigned            cProcesses;
        char                sz[64];
        sprintf(sz, "i=%d\n", i);
        CHECK_PTR_NULL(gpSPMHdr->apHeads[i], sz);

        /* check head backpointer. */
        if (SPM_VALID_PTR(gpSPMHdr->apHeads[i]) && gpSPMHdr->apHeads[i]->pPrev)
            CHECK_FAILED("Invalid list head in list %i. pPrev != NULL. pPrev=%p pProcess=%p pid=%#x\n",
                         i, (void *)gpSPMHdr->apHeads[i]->pPrev, (void *)gpSPMHdr->apHeads[i], gpSPMHdr->apHeads[i]->pid);

        /* validate the list. */
        cProcesses = 0;
        pProcessLast = NULL;
        pProcess = gpSPMHdr->apHeads[i];
        while (pProcess)
        {
            cProcesses++;
            CHECK_PTR(pProcess, sz);
            if (!SPM_VALID_PTR(pProcess))
                break;
            CHECK_LOG("pProcess=%08x enmState=%d cReferences=%d pid=%#06x pidParent=%#06x pInherit=%08x cSPMOpens=%d pNext=%08x pPrev=%08x\n",
                      (uintptr_t)pProcess, pProcess->enmState, pProcess->cReferences, pProcess->pid, pProcess->pidParent,
                      (uintptr_t)pProcess->pInherit, pProcess->cSPMOpens, (uintptr_t)pProcess->pNext, (uintptr_t)pProcess->pPrev);

            sprintf(sz, "i=%d pid=%#06x\n", i, pProcess->pid);
            CHECK_PTR_NULL(pProcess->pInherit, sz);
            CHECK_PTR_NULL(pProcess->pNext, sz);
            CHECK_PTR_NULL(pProcess->pPrev, sz);
            if (pProcess->enmState != i)
                CHECK_FAILED("Invalid state! enmState=%d i=%d\n",
                             pProcess->enmState, i);
            if (pProcess->pPrev != pProcessLast)
                CHECK_FAILED("Invalid back pointer! pPrev=%p pProcessLast=%p\n",
                             (void *)pProcess->pPrev, (void *)pProcessLast);
            if (pProcess->pPrev == pProcess)
                CHECK_FAILED("Cylic back pointer! pPrev=%p pProcessLast=%p pProcess=%p\n",
                             (void *)pProcess->pPrev, (void *)pProcessLast, (void *)pProcess);
            if (pProcess->pNext == pProcess)
                CHECK_FAILED("Cylic next pointer! pNext=%p pProcessLast=%p pProcess=%p\n",
                             (void *)pProcess->pNext, (void *)pProcessLast, (void *)pProcess);
            if (pProcess->pNext == gpSPMHdr->apHeads[i])
                CHECK_FAILED("Cylic next pointer (to head)! pNext=%p pHead=%p pProcess=%p\n",
                             (void *)pProcess->pNext, (void *)gpSPMHdr->apHeads[i], (void *)pProcess);
            if (pProcess->pPrev == pProcess->pNext && pProcess->pNext)
                CHECK_FAILED("Cylic back & next pointers! pPrev=%p pNext=%p pProcessLast=%p pProcess=%p\n",
                             (void *)pProcess->pPrev, (void *)pProcess->pNext, (void *)pProcessLast, (void *)pProcess);

            /* next */
            if (    pProcess->pNext == pProcess
                || (pProcess->pPrev == pProcess->pNext && pProcess->pNext)
                || pProcess->pNext == gpSPMHdr->apHeads[i])
                break;
            pProcessLast = pProcess;
            pProcess = pProcess->pNext;
        }
        CHECK_LOG("%d processes of type %d\n", cProcesses, i);
    }

    /*
     * Validate the heap free list.
     */
    cbTotal = 0;
    cbOverhead = 0;
    pFreePrev = NULL;
    pFree = gpSPMHdr->pPoolFreeHead;
    while (pFree)
    {
        size_t  cb;
        char    szMsg[64];
        CHECK_PTR(pFree, "");
        if (!SPM_VALID_PTR(pFree))
            break;
        CHECK_LOG("pFree=%08x pNext=%08x pPrev=%08x cb=%08x Core.pNext=%08x Core.pPrev=%08x\n",
                        (uintptr_t)pFree, (uintptr_t)pFree->pNext, (uintptr_t)pFree->pPrev, pFree->cb,
                        (uintptr_t)pFree->core.pNext, (uintptr_t)pFree->core.pPrev);
        sprintf(szMsg, "pFree=%p\n", (void *)pFree);
        CHECK_PTR_NULL(pFree->pNext, szMsg);
        CHECK_PTR_NULL(pFree->pPrev, szMsg);
        CHECK_PTR_NULL(pFree->core.pNext, szMsg);
        CHECK_PTR_NULL(pFree->core.pPrev, szMsg);
        if (pFree->pPrev != pFreePrev)
            CHECK_FAILED("Invalid back pointer! pFree=%p pPrev=%p pFreePrev=%p\n", (void *)pFree, (void *)pFree->pPrev, (void *)pFreePrev);
        if (pFree->pPrev == pFree)
            CHECK_FAILED("Cyclic back pointer!  pFree=%p pPrev=%p pFreePrev=%p\n", (void *)pFree, (void *)pFree->pPrev, (void *)pFreePrev);
        if (pFree->pNext == pFree)
            CHECK_FAILED("Cyclic next pointer!  pFree=%p pNext=%p pFreePrev=%p\n", (void *)pFree, (void *)pFree->pNext, (void *)pFreePrev);
        if (pFree->pNext == pFree->pPrev && pFree->pNext)
            CHECK_FAILED("Cyclic next & back pointer!  pFree=%p pNext=%p pPrev=%p pFreePrev=%p\n",
                         (void *)pFree, (void *)pFree->pNext, (void *)pFree->pNext, (void *)pFreePrev);
        if (pFree->pNext == gpSPMHdr->pPoolFreeHead)
            CHECK_FAILED("Cyclic next pointer (to head)!  pFree=%p pNext=%p pFreePrev=%p pHead=%p\n",
                         (void *)pFree, (void *)pFree->pNext, (void *)pFreePrev, (void *)gpSPMHdr->pPoolFreeHead);
        if (!pFree->pNext && gpSPMHdr->pPoolFreeTail != pFree)
            CHECK_FAILED("Invalid tail pointer!  pFree=%p pHead=%p pTail=%p\n",
                         (void *)pFree, (void *)gpSPMHdr->pPoolFreeHead, (void *)gpSPMHdr->pPoolFreeTail);
        if (pFreePrev > pFree)
            CHECK_FAILED("Invalid storting! pFree=%p pFreePrev=%p\n", (void *)pFree, (void *)pFreePrev);

        if (pFree->core.pNext)
            cb = (uintptr_t)pFree->core.pNext - (uintptr_t)(&pFree->core + 1);
        else
            cb = (uintptr_t)gpSPMHdr + gpSPMHdr->cb - (uintptr_t)(&pFree->core + 1);
        if (pFree->cb != cb)
            CHECK_FAILED("Invalid size of free block %p. Claimed %d, actual %d\n", (void *)pFree, pFree->cb, cb);

        /* next */
        if (    pFree->pNext == pFree
            ||  (pFree->pNext == pFree->pPrev && pFree->pNext)
            ||  pFree->pNext == gpSPMHdr->pPoolFreeHead)
            break;
        cbTotal += pFree->cb;
        cbOverhead += sizeof(pFree->core);
        pFreePrev = pFree;
        pFree = pFree->pNext;
    }

    CHECK_LOG("Free: cbTotal=%d cbOverhead=%d (core); cbFree=%d cb=%d\n", cbTotal, cbOverhead, gpSPMHdr->cbFree, gpSPMHdr->cb);
    if (cbTotal != gpSPMHdr->cbFree)
        CHECK_FAILED("Free memory count is bad. cbFree=%d in free list %d\n", gpSPMHdr->cbFree, cbTotal);


    /*
     * Validate the heap free list.
     */
    cbTotal = 0;
    cbOverhead = 0;
    pChunkPrev = NULL;
    pChunk = gpSPMHdr->pPoolHead;
    while (pChunk)
    {
        char    szMsg[64];
        CHECK_PTR(pChunk, "");
        if (!SPM_VALID_PTR(pChunk))
            break;
        CHECK_LOG("pChunk=%08x pNext=%08x pPrev=%08x\n",
                        (uintptr_t)pChunk, (uintptr_t)pChunk->pNext, (uintptr_t)pChunk->pPrev);
        sprintf(szMsg, "pChunk=%p\n", (void *)pChunk);
        CHECK_PTR_NULL(pChunk->pNext, szMsg);
        CHECK_PTR_NULL(pChunk->pPrev, szMsg);
        if (pChunk->pPrev != pChunkPrev)
            CHECK_FAILED("Invalid back pointer! pChunk=%p pPrev=%p pChunkPrev=%p\n", (void *)pChunk, (void *)pChunk->pPrev, (void *)pChunkPrev);
        if (pChunk->pPrev == pChunk)
            CHECK_FAILED("Cyclic back pointer!  pChunk=%p pPrev=%p pChunkPrev=%p\n", (void *)pChunk, (void *)pChunk->pPrev, (void *)pChunkPrev);
        if (pChunk->pNext == pChunk)
            CHECK_FAILED("Cyclic next pointer!  pChunk=%p pNext=%p pChunkPrev=%p\n", (void *)pChunk, (void *)pChunk->pNext, (void *)pChunkPrev);
        if (pChunk->pNext == pChunk->pPrev && pChunk->pNext)
            CHECK_FAILED("Cyclic next & back pointer!  pChunk=%p pNext=%p pPrev=%p pChunkPrev=%p\n",
                         (void *)pChunk, (void *)pChunk->pNext, (void *)pChunk->pNext, (void *)pChunkPrev);
        if (pChunk->pNext == gpSPMHdr->pPoolHead)
            CHECK_FAILED("Cyclic next pointer (to head)!  pChunk=%p pNext=%p pChunkPrev=%p pHead=%p\n",
                         (void *)pChunk, (void *)pChunk->pNext, (void *)pChunkPrev, (void *)gpSPMHdr->pPoolHead);
        if (!pChunk->pNext && gpSPMHdr->pPoolTail != pChunk)
            CHECK_FAILED("Invalid tail pointer!  pChunk=%p pHead=%p pTail=%p\n",
                         (void *)pChunk, (void *)gpSPMHdr->pPoolHead, (void *)gpSPMHdr->pPoolTail);
        if (pChunkPrev > pChunk)
            CHECK_FAILED("Invalid storting! pChunk=%p pChunkPrev=%p\n", (void *)pChunk, (void *)pChunkPrev);

        /* next */
        if (    pChunk->pNext == pChunk
            ||  (pChunk->pNext == pChunk->pPrev && pChunk->pNext)
            ||  pChunk->pNext == gpSPMHdr->pPoolHead)
            break;
        if (pChunk->pNext)
            cbTotal += (uintptr_t)pChunk->pNext - (uintptr_t)(pChunk + 1);
        else
            cbTotal += (uintptr_t)gpSPMHdr + gpSPMHdr->cb - (uintptr_t)(pChunk + 1);
        cbOverhead += sizeof(*pChunk);
        pChunkPrev = pChunk;
        pChunk = pChunk->pNext;
    }

    CHECK_LOG("Memory: cbTotal=%d cbOverhead=%d gpSPMHdr->cb=%d\n", cbTotal, cbOverhead, gpSPMHdr->cb);
    if (cbTotal + cbOverhead != gpSPMHdr->cb - gpSPMHdr->cbProcess)
        CHECK_FAILED("Memory accouting count is bad. Found %d expected %d ; cbTotal=%d cbOverhead=%d gpSPMHdr->cb=%d\n",
                     cbTotal + cbOverhead, gpSPMHdr->cb - gpSPMHdr->cbProcess, cbTotal, cbOverhead, gpSPMHdr->cb);

    CHECK_LOG("spmCheck returns cErrors=%d\n", cErrors);
    return cErrors;
}


