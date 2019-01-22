/* $Id: libcfork.c 3805 2014-02-06 11:37:29Z ydario $ */
/** @file
 *
 * LIBC SYS Backend fork().
 *
 * Copyright (c) 2004-2005 knut st. osmundsen <bird-srcspam@anduin.net>
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
#define NO_EXAPIS_MAPPINGS
#define INCL_BASE
#define INCL_FSMACROS
#define INCL_FPCWMACROS
#define INCL_EXAPIS
#define INCL_ERRORS
#define INCL_DOSINFOSEG
#include <os2emx.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <setjmp.h>
#include <emx/syscalls.h>
#include <emx/startup.h>
#include <386/builtin.h>
#include <sys/fmutex.h>
#include <InnoTekLIBC/fork.h>
#include <InnoTekLIBC/sharedpm.h>
#include <InnoTekLIBC/FastInfoBlocks.h>
#define __LIBC_LOG_GROUP    __LIBC_LOG_GRP_FORK
#include <InnoTekLIBC/logstrict.h>
#include "syscalls.h"
#include "DosEx.h"
#include "b_process.h"


/*******************************************************************************
*   Structures and Typedefs                                                    *
*******************************************************************************/
/**
 * Duplicate pages chunk.
 */
typedef struct FORKPGCHUNK
{
    /** Address of the chunk. */
    void       *pv;
    /** Size of (the data portion of) this chunk. */
    size_t      cb;
    /** Virtual size of the chunk.
     * This is not cb, but the range for which the page attributes should be
     * applied. */
    size_t      cbVirt;
    /** Page attributes of the chunk.
     * Only applies if the __LIBC_FORK_FLAGS_PAGE_ATTR is set. */
    unsigned    fFlags;
    /** Alignment skip. */
    uint32_t    offData;
    /** The page data - add offData. */
    char        achData[1];
} FORKPGCHUNK, *PFORKPGCHUNK;


/**
 * Exception handler registration record for the parent process.
 */
typedef struct __LIBC_FORKXCPTREGREC
{
    /** The exception registration record. */
    EXCEPTIONREGISTRATIONRECORD Core;
    /** Indicator set by the exception handler if it did the completion handlers. */
    volatile int                fDoneCompletion;
} __LIBC_FORKXCPTREGREC, *__LIBC_PFORKXCPTREGREC;

/**
 * Exception registration record used for the module registration
 * and deregistration APIs.
 */
typedef struct __LIBC_FORKXCPTREGREC2
{
    /** The OS/2 exception record. */
    EXCEPTIONREGISTRATIONRECORD Core;
    /** Our jump buffer. */
    jmp_buf                     JmpBuf;
} __LIBC_FORKXCPTREGREC2, *__LIBC_PFORKXCPTREGREC2;



/**
 * 80-bit MMX/FPU register type.
 */
typedef struct X86FPUMMX
{
    uint8_t reg[10];
} X86FPUMMX;
/** Pointer to 80-bit MMX/FPU register type. */
typedef X86FPUMMX *PX86FPUMMX;

/**
 * FPU Extended state (aka FXSAVE/FXRSTORE Memory Region).
 */
#pragma pack(1)
typedef struct X86FXSTATE
{
    /** Control word. */
    uint16_t    FCW;
    /** Status word. */
    uint16_t    FSW;
    /** Tag word (it's a byte actually). */
    uint8_t     FTW;
    uint8_t     huh1;
    /** Opcode. */
    uint16_t    FOP;
    /** Instruction pointer. */
    uint32_t    FPUIP;
    /** Code selector. */
    uint16_t    CS;
    uint16_t    Rsvrd1;
    /* - offset 16 - */
    /** Data pointer. */
    uint32_t    FPUDP;
    /** Data segment */
    uint16_t    DS;
    uint16_t    Rsrvd2;
    uint32_t    MXCSR;
    uint32_t    MXCSR_MASK;
    /* - offset 32 - */
    union
    {
        /** MMX view. */
        uint64_t    mmx;
        /** FPU view - todo. */
        X86FPUMMX   fpu;
        /** 8-bit view. */
        uint8_t     au8[16];
        /** 16-bit view. */
        uint16_t    au16[8];
        /** 32-bit view. */
        uint32_t    au32[4];
        /** 64-bit view. */
        uint64_t    au64[2];
    } aRegs[8];
    /* - offset 160 - */
    union
    {
        /** 8-bit view. */
        uint8_t     au8[16];
        /** 16-bit view. */
        uint16_t    au16[8];
        /** 32-bit view. */
        uint32_t    au32[4];
        /** 64-bit view. */
        uint64_t    au64[2];
    } aXMM[8];
    /* - offset 288 - */
    uint32_t    au32RsrvdRest[(512 - 288) / sizeof(uint32_t)];
} X86FXSTATE;
#pragma pack()
/** Pointer to FPU Extended state. */
typedef X86FXSTATE  *PX86FXSTATE;


/*******************************************************************************
*   Internal Functions                                                         *
*******************************************************************************/
/*
 * The function have prefixes indicating where they belong:
 *
 *      forkPar*() are executed in the parent context only.
 *      forkChl*() are executed in the child context only.
 *      forkBth*() are executed in both contexts.
 *      forkPrm*() are fork primitives expored thru the fork handle.
 *
 * The order of functions are in the first section in the execution order.
 * In the 2nd section they are ordered by what they do.
 */
/* sequential */
CRT_DATA_USED
static int                  forkParDo(void *pvForkRet, void *pvStackRet, __LIBC_PFORKXCPTREGREC pXcptRegRec);
static int                  forkParCanFork(__LIBC_PFORKMODULE pModules);
static int                  forkParValidateModules(__LIBC_PFORKMODULE pModules);
static __LIBC_PFORKHANDLE   forkParAllocHandle(__LIBC_PFORKMODULE pModules, void *pvStackRet, void *pvForkRet);
static int                  forkParRunPreExecParent(__LIBC_PFORKHANDLE pForkHandle, __LIBC_PFORKMODULE pModules);
static int                  forkParExec(__LIBC_PFORKHANDLE pForkHandle, pid_t *ppid);
static __LIBC_PFORKHANDLE   forkChlOpenHandle(void *pvForkHandle);
static int                  forkChlDoFork(__LIBC_PFORKHANDLE pForkHandle, __LIBC_PFORKMODULE pModule, int fExecutable);
static int                  forkChlDoFork2(__LIBC_PFORKHANDLE pForkHandle);
static int                  forkParRunForkParent(__LIBC_PFORKHANDLE pForkHandle, __LIBC_PFORKMODULE pModules);
static int                  forkParRunForkChild(__LIBC_PFORKHANDLE pForkHandle, __LIBC_PFORKMODULE pModules);
static int                  forkChlRunForkChild(__LIBC_PFORKHANDLE pForkHandle, __LIBC_PFORKMODULE pModules);
static void                 forkParCompletionChild(__LIBC_PFORKHANDLE pForkHandle, int rc);
static void                 forkChlCompletionChild(__LIBC_PFORKHANDLE pForkHandle, int rc);
static void                 forkParCompletionParent(__LIBC_PFORKHANDLE pForkHandle, int rc);
static void                 forkBothCompletion(__LIBC_PFORKHANDLE pForkHandle, int rc, __LIBC_FORKCTX enmCtx, unsigned volatile *piCompletionCallback);
static void                 forkBthCloseHandle(__LIBC_PFORKHANDLE pForkHandle, __LIBC_FORKCTX enmContext);
/* any time */
static int                  forkPrmDuplicatePages(__LIBC_PFORKHANDLE pForkHandle, void *pvStart, void *pvEnd, unsigned fFlags);
static int                  forkPrmInvoke(__LIBC_PFORKHANDLE pForkHandle, int (*pfn)(__LIBC_PFORKHANDLE pForkHandle, void *pvArg, size_t cbArg), void *pvArg, size_t cbArg);
static int                  forkPrmFlush(__LIBC_PFORKHANDLE pForkHandle);
static int                  forkPrmCompletionCallback(__LIBC_PFORKHANDLE pForkHandle, __LIBC_PFNCOMPLETIONCALLBACK pfnCallback, void *pvArg, __LIBC_FORKCTX enmContext);
static __LIBC_PFORKMODULE   forkBthGetModules(void);
static int                  forkBthProcessModules(__LIBC_PFORKHANDLE pForkHandle, __LIBC_PFORKMODULE pModules, __LIBC_FORKOP enmOperation);
static int                  forkBthBufferGive(__LIBC_PFORKHANDLE pForkHandle, __LIBC_FORKCTX enmCtx);
static int                  forkBthBufferWait(__LIBC_PFORKHANDLE pForkHandle, __LIBC_FORKCTX enmCtx);
static int                  forkBthBufferProcess(__LIBC_PFORKHANDLE pForkHandle, __LIBC_FORKCTX enmCtx, int *pfNext);
static int                  forkBthBufferDuplicate(__LIBC_PFORKHANDLE pForkHandle, __LIBC_PFORKPKGHDR pHdr);
static void                 forkBthBufferReset(__LIBC_PFORKHANDLE pForkHandle);
static void                 forkBthBufferEnd(__LIBC_PFORKHANDLE pForkHandle);
static void                 forkBthBufferNext(__LIBC_PFORKHANDLE pForkHandle);
static void                 forkBthBufferAbort(__LIBC_PFORKHANDLE pForkHandle, int rc);
static int                  forkBthBufferSpace(__LIBC_PFORKHANDLE pForkHandle, size_t cb);
static int                  forkBthBufferGiveWaitNext(__LIBC_PFORKHANDLE pForkHandle, __LIBC_FORKCTX enmCtx);
static void                 forkBthCopyPagesDetect(void *pvDst, const void *pvSrc, size_t cb);
static void                 forkBthCopyPagesPlain(void *pvDst, const void *pvSrc, size_t cb);
static void                 forkBthCopyPagesMMX(void *pvDst, const void *pvSrc, size_t cb);
#if 0
static void                 forkBthCopyPagesMMXNonTemporal(void *pvDst, const void *pvSrc, size_t cb);
static void                 forkBthCopyPagesSSE2(void *pvDst, const void *pvSrc, size_t cb);
#endif
static void                 forkBthCallbacksSort(__LIBC_PFORKCALLBACK *papCallbacks);
static int                  forkBthCallbacksCompare(const void *pv1, const void *pv2);
static int                  forkBthCallbacksCall(__LIBC_PFORKCALLBACK *papCallbacks, __LIBC_PFORKHANDLE pForkHandle, __LIBC_FORKOP enmOperation);
static void                 forkBthDumpMemFlags(void *pv);
static void                 forkChlFatalError(__LIBC_PFORKHANDLE pForkHandle, int rc, void *pvCtx) __attribute__((__noreturn__));
static ULONG _System        forkChlExceptionHandler(PEXCEPTIONREPORTRECORD       pXcptRepRec,
                                                    PEXCEPTIONREGISTRATIONRECORD pXcptRegRec,
                                                    PCONTEXTRECORD               pCtx,
                                                    PVOID                        pvWhatEver);
static ULONG _System        forkParExceptionHandler(PEXCEPTIONREPORTRECORD       pXcptRepRec,
                                                    PEXCEPTIONREGISTRATIONRECORD pXcptRegRec,
                                                    PCONTEXTRECORD               pCtx,
                                                    PVOID                        pvWhatEver);
static ULONG _System        forkBthExceptionHandlerRegDereg(PEXCEPTIONREPORTRECORD       pXcptRepRec,
                                                            PEXCEPTIONREGISTRATIONRECORD pXcptRegRec,
                                                            PCONTEXTRECORD               pCtx,
                                                            PVOID                        pvWhatEver);


/*******************************************************************************
*   Global Variables                                                           *
*******************************************************************************/
/** Pointer to the page copy function. */
static void                (*pfnForkBthCopyPages)(void *pvDst, const void *pvSrc, size_t cb) = forkBthCopyPagesDetect;
/** Pointer to the forkhandle for the current fork operation.
 * This is *only* used by the exception and signal handlers! */
static volatile __LIBC_PFORKHANDLE g_pForkHandle = NULL;




/*//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\*/
/*//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\*/
/*//                                                                                                                \\*/
/*//                                                                                                                \\*/
/*//        Here follows the functions making up the public fork api.                                               \\*/
/*//                                                                                                                \\*/
/*//                                                                                                                \\*/
/*//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\*/
/*//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\*/

#ifdef TIMEBOMB
static void forkTimebomb(void)
{
    ULONG aul[2];
    FS_VAR();
    FS_SAVE_LOAD();
    /* this will trigger after the default one, so no message. */
    if (    !DosQuerySysInfo(QSV_TIME_LOW, QSV_TIME_HIGH, &aul, sizeof(aul))
        &&  aul[1] == 0 && aul[0] >= 0x41860bd1 && aul[0] <= 0x41dbbcba)
    {
        FS_RESTORE();
        return;
    }
    for (;;)
        DosExit(EXIT_PROCESS, 127);
}
#endif


#ifdef DEBUG_LOGGING
/**
 * Logs the name, handle and stuff of the module which the give address belongs to.
 *
 * @param   pvInModule  Address within a module.
 */
static void forkLogModuleByAddr(PVOID pvInModule)
{
    HMODULE hmod = NULLHANDLE;
    ULONG   offObj = 0;
    ULONG   iObj   = 0;
    CHAR    szName[32];
    APIRET  rc = DosQueryModFromEIP(&hmod, &iObj, sizeof(szName), &szName[0], &offObj, (uintptr_t)pvInModule);
    if (rc == NO_ERROR)
    {
        CHAR szFullName[CCHMAXPATH];
        if (DosQueryModuleName(hmod, sizeof(szFullName), szFullName) != NO_ERROR)
            szFullName[0] = '\0';
        LIBCLOG_MSG2("pvInModule=%p: hmod=%lx iObj=%#lx offObj=%lx %s %s\n",
                     pvInModule, hmod, iObj, offObj, szName, szFullName);
    }
    else
        LIBCLOG_MSG2("pvInModule=%p: rc=%ld", pvInModule, rc);
}
#endif /* DEBUG_LOGGING */


/**
 * Register a forkable module. Called by crt0 and dll0.
 *
 * The call links pModule into the list of forkable modules
 * which is maintained in the process block.
 *
 * @returns 0 on normal process startup.
 *
 * @returns 1 on forked child process startup.
 *          The caller should respond by not calling any _DLL_InitTerm
 *          or similar constructs. If fExecutable the call will not
 *          return in this situation.
 *
 * @returns negative on failure.
 *          The caller should return from the dll init returning FALSE.
 *          If called from crt0 the function will it self call DosExit.
 *
 * @param   pModule     Pointer to the fork module structure for the
 *                      module which is to registered.
 * @param   fExecutable Indicator that the call originates from crt0.s and
 *                      the final forking should start. The function
 *                      will not return if this flag is set and the process
 *                      was forked.
 */
int __libc_ForkRegisterModule(__LIBC_PFORKMODULE pModule, int fExecutable)
{
    LIBCLOG_ENTER("pModule=%p:{.uVersion=%#x, .pfnAtFork=%p, .papParent1=%p, .papChild1=%p, .pvDataSegBase=%p, .pvDataSegEnd=%p, .fFlags=%#x, .pNext=%p} fExecutable=%d\n",
                  (void *)pModule, pModule->uVersion, (void *)pModule->pfnAtFork, (void *)pModule->papParent1, (void *)pModule->papChild1,
                  (void *)pModule->pvDataSegBase, (void *)pModule->pvDataSegEnd, pModule->fFlags, (void *)pModule->pNext, fExecutable);
#ifdef DEBUG_LOGGING
    forkLogModuleByAddr(pModule->pvDataSegBase);
#endif
    int                     rc;
    __LIBC_PSPMPROCESS      pProcess;
    __LIBC_PFORKHANDLE      pForkHandle;
    PTIB                    pTib;
    PPIB                    pPib;
    __LIBC_FORKXCPTREGREC2  XcptRegRec;

    /*
     * Install an exceptionhandler.
     */
    DosGetInfoBlocks(&pTib, &pPib);
    XcptRegRec.Core.ExceptionHandler = forkBthExceptionHandlerRegDereg;
    XcptRegRec.Core.prev_structure = pTib->tib_pexchain;
    if (!setjmp(XcptRegRec.JmpBuf))
    {
        pTib->tib_pexchain = &XcptRegRec;

#ifdef TIMEBOMB
        __libc_Timebomb();
#endif

        /*
         * Find the SPM process.
         */
        pProcess = __libc_spmSelf();
        if (!pProcess)
        {
            static char szMsg[] = "LIBC Error: Couldn't register process in shared memory!\r\n";
            ULONG       ul;
            LIBC_ASSERTM_FAILED("couldn't register process!\n");

            DosWrite(2, szMsg, sizeof(szMsg), &ul);
            while (fExecutable)
            {
                LIBCLOG_MSG("Calling DosExit(EXIT_PROCESS, 0xffff)...\n");
                DosExit(EXIT_PROCESS, /*fixme*/0xffff);
            }
            pTib->tib_pexchain = XcptRegRec.Core.prev_structure;
            LIBCLOG_RETURN_INT(-1);
        }

        /*
         * Register the module.
         * (We're hoping for OS/2 to indirectly serialize this... not properly verified yet.)
         */
        if (pModule->fFlags & __LIBC_FORKMODULE_FLAGS_DEREGISTERED)
            LIBCLOG_MSG("__LIBC_FORKMODULE_FLAGS_DEREGISTERED!\n");
        pModule->fFlags &= ~__LIBC_FORKMODULE_FLAGS_DEREGISTERED;
        pModule->pNext = NULL;
        if (pProcess->pvModuleHead)
            *(__LIBC_PFORKMODULE*)pProcess->ppvModuleTail = pModule;
        else
            pProcess->pvModuleHead = pModule;
        pProcess->ppvModuleTail = (void **)&pModule->pNext;


#if 0
        /*
         * Debug checks.
         */
        static char sz[] = "forking\0forking\0";
        PPIB pPib;
        PTIB pTib;
        DosGetInfoBlocks(&pTib, &pPib);
        if (!memcmp(pPib->pib_pchcmd, sz, sizeof(sz)))
        {
            LIBC_ASSERTM(pProcess->pvForkHandle, "fork arguments, no fork handle!!! process %p\n", pProcess);
            if (!pProcess->pvForkHandle)
                __libc_SpmCheck(0, 1);
        }
        else
        {
            LIBC_ASSERTM(!pProcess->pvForkHandle, "no fork arguments, fork handle!!! process %p handle %p\n", pProcess, pProcess->pvForkHandle);
            if (pProcess->pvForkHandle)
                __libc_SpmCheck(0, 1);
        }
#endif

        /*
         * Are we forking?
         */
#ifdef TIMEBOMB
        forkTimebomb();
#endif
        if (!pProcess->pvForkHandle)
        {
            pTib->tib_pexchain = XcptRegRec.Core.prev_structure;
            LIBCLOG_RETURN_INT(0);
        }
        /* we are! */

        /*
         * Open the fork handle.
         */
        pForkHandle = forkChlOpenHandle(pProcess->pvForkHandle);
        if (!pForkHandle)
        {
            while (fExecutable)
            {
                LIBCLOG_MSG("Calling DosExit(EXIT_PROCESS, 0xffff)...\n");
                DosExit(EXIT_PROCESS, /*fixme*/0xffff);
            }
            pTib->tib_pexchain = XcptRegRec.Core.prev_structure;
            LIBCLOG_RETURN_INT(-1);
        }
        g_pForkHandle = pForkHandle;

        /*
         * Let pfnDoFork() process the module.
         */
        XcptRegRec.Core.ExceptionHandler = forkChlExceptionHandler;
        rc = pForkHandle->pfnDoFork(pForkHandle, pModule, fExecutable);
        if (rc < 0)
        {
            while (fExecutable)
            {
                LIBCLOG_MSG("Calling DosExit(EXIT_PROCESS, 0xffff)...\n");
                DosExit(EXIT_PROCESS, /*fixme*/0xffff);
            }
            pTib->tib_pexchain = XcptRegRec.Core.prev_structure;
            LIBCLOG_RETURN_INT(-1);
        }

        /*
         * Done.
         */
        LIBC_ASSERT(!fExecutable);
        pTib->tib_pexchain = XcptRegRec.Core.prev_structure;
        LIBCLOG_RETURN_INT(1);
    }

    /* Exception! */
    LIBCLOG_RETURN_INT(-1);
}


/**
 * Deregister a forkable module. Called by dll0.
 *
 * The call links pModule out of the list of forkable modules
 * which is maintained in the process block.
 *
 * @param   pModule     Pointer to the fork module structure for the
 *                      module which is to registered.
 */
void __libc_ForkDeregisterModule(__LIBC_PFORKMODULE pModule)
{
    LIBCLOG_ENTER("pModule=%p:{.uVersion=%#x, .pfnAtFork=%p, .papParent1=%p, .papChild1=%p, .pvDataSegBase=%p, .pvDataSegEnd=%p, .fFlags=%#x, .pNext=%p}\n",
                  (void *)pModule, pModule->uVersion, (void *)pModule->pfnAtFork, (void *)pModule->papParent1, (void *)pModule->papChild1,
                  (void *)pModule->pvDataSegBase, (void *)pModule->pvDataSegEnd, pModule->fFlags, (void *)pModule->pNext);
#ifdef DEBUG_LOGGING
    forkLogModuleByAddr(pModule->pvDataSegBase);
#endif
    PTIB                    pTib;
    PPIB                    pPib;
    __LIBC_FORKXCPTREGREC2  XcptRegRec;

    /*
     * Install an exceptionhandler.
     */
    DosGetInfoBlocks(&pTib, &pPib);
    XcptRegRec.Core.ExceptionHandler = forkBthExceptionHandlerRegDereg;
    XcptRegRec.Core.prev_structure = pTib->tib_pexchain;
    if (!setjmp(XcptRegRec.JmpBuf))
    {
        pTib->tib_pexchain = &XcptRegRec;

        /*
         * Don't deregister modules which has already been deregistered.
         * Don't deregister if we're in shutting down the process (waste of time and
         * SPM might already be shut down).
         */
        if (pModule->fFlags & __LIBC_FORKMODULE_FLAGS_DEREGISTERED)
        {
            pTib->tib_pexchain = XcptRegRec.Core.prev_structure;
            LIBCLOG_RETURN_MSG_VOID( "ret void (__LIBC_FORKMODULE_FLAGS_DEREGISTERD)\n");
        }
        if (!__libc_GpFIBLIS)
        {
            pTib->tib_pexchain = XcptRegRec.Core.prev_structure;
            LIBCLOG_RETURN_MSG_VOID( "ret void (__libc_GpFIBLIS)\n");
        }
        if (fibIsInExit())
        {
            pTib->tib_pexchain = XcptRegRec.Core.prev_structure;
            LIBCLOG_RETURN_MSG_VOID( "ret void (fibIsInExit)\n");
        }

        /*
         * Find the SPM process.
         */
        __LIBC_PSPMPROCESS pProcess = __libc_spmSelf();
        if (!pProcess)
        {
            pTib->tib_pexchain = XcptRegRec.Core.prev_structure;
            LIBC_ASSERTM_FAILED("can't find the process! weird!\n");
            LIBCLOG_RETURN_VOID();
        }

        /*
         * Deregister the module.
         * (We're hoping for OS/2 to indirectly serialize this... not properly verified yet.)
         */
        if ((__LIBC_PFORKMODULE)pProcess->pvModuleHead == pModule)
        {
            pProcess->pvModuleHead = pModule->pNext;
            if (!pModule->pNext)
                pProcess->ppvModuleTail = &pProcess->pvModuleHead;
        }
        else
        {
            __LIBC_PFORKMODULE pPrev = (__LIBC_PFORKMODULE)pProcess->pvModuleHead;
            while (pPrev && pPrev->pNext != pModule)
                pPrev = pPrev->pNext;
            if (!pPrev)
            {
                pTib->tib_pexchain = XcptRegRec.Core.prev_structure;
                LIBC_ASSERTM_FAILED("can't find the module! weird!\n");
                LIBCLOG_RETURN_VOID();
            }

            pPrev->pNext = pModule->pNext;
            if (!pModule->pNext)
                pProcess->ppvModuleTail = (void **)&pPrev->pNext;
        }

        pModule->pNext = NULL;
        pModule->fFlags |= __LIBC_FORKMODULE_FLAGS_DEREGISTERED;

        pTib->tib_pexchain = XcptRegRec.Core.prev_structure;
        LIBCLOG_RETURN_VOID();
    }

    /* Exception! */
    LIBCLOG_RETURN_VOID();
}


/**
 * Checks if the current CPU supports CPUID.
 *
 * @returns 1 if CPUID is supported, 0 if it doesn't.
 */
static inline int HasCpuId(void)
{
    int         fRet = 0;
    uint32_t    u1;
    uint32_t    u2;
    __asm__ ("pushf\n\t"
             "pop   %1\n\t"
             "mov   %1, %2\n\t"
             "xorl  $0x200000, %1\n\t"
             "push  %1\n\t"
             "popf\n\t"
             "pushf\n\t"
             "pop   %1\n\t"
             "cmpl  %1, %2\n\t"
             "setne %0\n\t"
             "push  %2\n\t"
             "popf\n\t"
             : "=m" (fRet), "=r" (u1), "=r" (u2));
    return fRet;
}


/**
 * Performs the cpuid instruction returning edx.
 *
 * @param   uOperator   CPUID operation (eax).
 * @returns EDX after cpuid operation.
 */
static inline uint32_t CpuIdEDX(unsigned uOperator)
{
    uint32_t    u32EDX;
    __asm__ ("cpuid"
             : "=a" (uOperator),
               "=d" (u32EDX)
             : "0" (uOperator)
             : "ebx", "ecx");
    return u32EDX;
}


/**
 * Fork a child process pretty much identical to the calling process.
 * See SuS for full description of what fork() does and doesn't.
 *
 * @returns 0 in the child process.
 * @returns process identifier of the new child in the parent process. (positive, non-zero)
 * @returns Negative error code (errno.h) on failure.
 */
pid_t __libc_Back_processFork(void)
{
    LIBCLOG_ENTER("__libc_Back_processFork:\n");

    /*
     * Determine fxsave/fxrstor support.
     */
    static int fHaveFXSR = -1;
    if (fHaveFXSR == -1)
        fHaveFXSR = HasCpuId() && (CpuIdEDX(1) & 0x1000000); /* bit 24 - fxsr */

    /*
     * Save the FPU state
     */
    PX86FXSTATE pFPU = (PX86FXSTATE)alloca(512 + 16);
    pFPU = (PX86FXSTATE)(((uintptr_t)pFPU + 15) & ~15);
    if (fHaveFXSR)
        __asm__ __volatile__ ("fxsave %0" : "=m" (*pFPU));
    else
        __asm__ __volatile__ ("fnsave %0" : "=m" (*pFPU));

    /*
     * Take the Exec Semaphore.
     */
    int rc = _fmutex_request(&__libc_gmtxExec, 0);
    if (rc)
    {
        rc = -__libc_native2errno(rc);
        LIBCLOG_ERROR_RETURN_INT(rc);
    }

    /*
     * Install exception handler, enter must complete section (pretending we're in the kernel).
     */
    g_pForkHandle = NULL;               /* exception handler variable */

    PTIB  pTib = NULL;
    PPIB  pPib;
    FS_VAR_SAVE_LOAD();
    DosGetInfoBlocks(&pTib, &pPib);
    __LIBC_FORKXCPTREGREC XcptRegRec;
    XcptRegRec.fDoneCompletion       = 0;
    XcptRegRec.Core.ExceptionHandler = forkParExceptionHandler;
    XcptRegRec.Core.prev_structure   = pTib->tib_pexchain;
    pTib->tib_pexchain = &XcptRegRec.Core;

    ULONG cNesting = 0;
    DosEnterMustComplete(&cNesting);

    /*
     * Save registers.
     */
    pid_t pid = -1;
    __asm__ __volatile__ (
        "pushf\n\t"
        "pushl   %%ebx\n\t"
        "pushl   %%esi\n\t"
        "pushl   %%edi\n\t"
        "pushl   %%ebp\n\t"
        "pushl   %%gs\n\t"

    /*
     * Call the fork worker, forkParDo().
     */
        "movl    %%esp, %%eax\n\t"
        "pushl   %%edx\n\t"
        "pushl   %%eax\n\t"
        "pushl   $fork_ret\n\t"
        "call    _forkParDo\n\t"
        "addl    $12, %%esp\n\t"         /* __cdecl */
        "jmp     fork_skip_regs\n\t"

    /*
     * The Child returns here.
     */
        "fork_ret:\n\t"
        "popl    %%gs\n\t"
        "popl    %%ebp\n\t"
        "popl    %%edi\n\t"
        "popl    %%esi\n\t"
        "popl    %%ebx\n\t"
        "popf\n\t"
        "\n"
        "fork_skip_regs:\n\t"
        : "=a" (pid): "d" (&XcptRegRec) : "ecx");

    /*
     * Restore the exception handler and exit the must complete section.
     */
    pTib->tib_pexchain = XcptRegRec.Core.prev_structure;
    DosExitMustComplete(&cNesting);

    /*
     * Release the exec semaphore.
     */
    _fmutex_release(&__libc_gmtxExec);

    /*
     * Restore the FPU state.
     */
    if (fHaveFXSR)
        __asm__ __volatile__ ("fxrstor %0" : "=m" (*pFPU));
    else
        __asm__ __volatile__ ("frstor %0" : "=m" (*pFPU));

    FS_RESTORE();
    LIBCLOG_RETURN_INT(pid);
}


/**
 * This is the function called by __libc_Back_processFork() to drive the
 * parent side of the fork.
 *
 * At this point we're in a must-complete section, owning the exec mutex,
 * and have forkParExceptionHandler installed.
 *
 *
 * The comunication between parent and child is as follows:
 *
 *      -# Parent releases fork buffer and does DosExecPgm.
 *      -# Child takes fork buffer and processes it.
 *         Childs then does initialization and gives the buffer
 *         back to the parent.
 *      -# Parent does the main fork run.
 *         During this the buffer might go back and forth between the
 *         two processes a lot.
 *      -# Child does the main fork run.
 *         During this the buffer might go back and forth a bit, but
 *         it's more unlikely.
 *
 *
 * @returns pid of the forked child process on success.
 * @returns Negative error code (errno.h) on failure.
 *
 * @param   pvForkRet       Return address for the child.
 * @param   pvStackRet      ESP for the child when returning from fork.
 * @param   pXcptRegRec     Pointer to the exception registration record (fDoneCompletion).
 */
static int forkParDo(void *pvForkRet, void *pvStackRet, __LIBC_PFORKXCPTREGREC pXcptRegRec)
{
    LIBCLOG_ENTER("pvForkRet=%p pvStackRet=%p\n", pvForkRet, pvStackRet);

    /*
     * Get module list.
     */
    __LIBC_PFORKMODULE pModules = forkBthGetModules();
    if (!pModules)
        LIBCLOG_ERROR_RETURN_INT(-ENOSYS);

    /*
     * Check if we have an executable in the module list.
     * It's impossible to pull off a fork without the executable being ready for it!
     */
    if (forkParCanFork(pModules))
    {
        LIBC_ASSERTM_FAILED("Can't fork this process, the executable wasn't built with -Zfork!\n");
        LIBCLOG_ERROR_RETURN_INT(-ENOSYS);
    }

    /*
     * Check that we can handle all the modules in the chain.
     */
    int rc = forkParValidateModules(pModules);
    if (rc < 0)
        LIBCLOG_ERROR_RETURN_INT(rc);

    /*
     * Allocate and initialize the memory for the fork handle.
     * Install signal handlers after that.
     */
    pid_t   pid = -1;
    __LIBC_PFORKHANDLE pForkHandle = forkParAllocHandle(pModules, pvStackRet, pvForkRet);
    g_pForkHandle = pForkHandle;
    if (pForkHandle)
    {
        /*
         * Take the pre-exec run.
         */
        rc = forkParRunPreExecParent(pForkHandle, pModules);
        if (rc >= 0)
        {
            /*
             * Create child.
             */
            rc = forkParExec(pForkHandle, &pid);
            if (rc >= 0)
            {
                /*
                 * Do the fork run in parent context.
                 */
                rc = forkParRunForkParent(pForkHandle, pModules);
                if (rc >= 0)
                {
                    /*
                     * Do the fork run in child context and call child completion callbacks.
                     */
                    rc = forkParRunForkChild(pForkHandle, pModules);
                }
                else
                {
                    /*
                     * If parent fails, we need to execute completion callbacks in child context.
                     */
                    forkParCompletionChild(pForkHandle, rc);
                }

                /*
                 * Try reap the child in case of failure.
                 */
                if (rc < 0)
                {
                    RESULTCODES resc;
                    PID         pibReaped;
                    DosWaitChild(DCWA_PROCESS,DCWW_NOWAIT, &resc, &pibReaped, pid);
                }
            }
        }

        /*
         * Process child context completion callbacks.
         */
        if (!pXcptRegRec->fDoneCompletion)
            forkParCompletionParent(pForkHandle, rc);
        forkBthCloseHandle(pForkHandle, __LIBC_FORK_CTX_PARENT);
    }
    else
        rc = -ENOMEM;


    /*
     * Return.
     */
    if (rc >= 0)
    {
        __libc_back_processWaitNotifyExec(pid);
        LIBCLOG_RETURN_INT(pid);
    }
    LIBCLOG_ERROR_RETURN_INT(rc);
}


/**
 * Called multiple times during fork() both in the parent and the child.
 *
 * This default LIBC implementation will:
 *      1) schedule the data segment for duplication.
 *      2) do ordered LIBC fork() stuff.
 *      3) do unordered LIBC fork() stuff, _CRT_FORK1 vector.
 *
 * @returns 0 on success.
 * @returns appropriate negative errno on failure.
 * @returns appropriate positive errno as warning.
 * @param   pModule         Pointer to the module record which is being
 *                          processed.
 * @param   pForkHandle     Handle of the current fork operation.
 * @param   enmOperation    Which callback operation this is.
 *                          Any value can be used, the implementation
 *                          of this function must just respond to the
 *                          one it knows and return successfully on the
 *                          others.
 *                          Operations:
 *                              __LIBC_FORK_OP_CHECK_PARENT
 *                              __LIBC_FORK_OP_CHECK_CHILD
 *                              __LIBC_FORK_OP_FORK_PARENT
 *                              __LIBC_FORK_OP_FORK_CHILD
 */
int __libc_ForkDefaultModuleCallback(__LIBC_PFORKMODULE pModule, __LIBC_PFORKHANDLE pForkHandle, enum __LIBC_FORKOP enmOperation)
{
    LIBCLOG_ENTER("pModule=%p pForkHandle=%p enmOperation=%d\n", (void *)pModule, (void *)pForkHandle, enmOperation);
    int                     rc;
    __LIBC_PFORKCALLBACK   *papCallbacks;

    /*
     * Module actions.
     */
    switch (enmOperation)
    {
        /*
         * Duplicate data pages.
         */
        case __LIBC_FORK_OP_FORK_PARENT:
            rc = pForkHandle->pfnDuplicatePages(pForkHandle, pModule->pvDataSegBase, pModule->pvDataSegEnd, __LIBC_FORK_FLAGS_ONLY_DIRTY);
            if (rc < 0)
                LIBCLOG_RETURN_INT(rc);
            break;
        default: break;
    }

    /*
     * Sort the list.
     */
    switch (enmOperation)
    {
        case __LIBC_FORK_OP_EXEC_PARENT:
            forkBthCallbacksSort(pModule->papParent1);
            break;
        case __LIBC_FORK_OP_EXEC_CHILD:
        case __LIBC_FORK_OP_FORK_CHILD: /* was overwritten :-/ */
            forkBthCallbacksSort(pModule->papChild1);
            break;
        default: break;
    }

    /*
     * Process the callbacks.
     */
    switch (enmOperation)
    {
        case __LIBC_FORK_OP_EXEC_PARENT:
        case __LIBC_FORK_OP_FORK_PARENT:
            papCallbacks = pModule->papParent1;
            break;
        case __LIBC_FORK_OP_EXEC_CHILD:
        case __LIBC_FORK_OP_FORK_CHILD:
            papCallbacks = pModule->papChild1;
            break;
        default:
            LIBC_ASSERTM_FAILED("%d is an invalid fork operation!\n", enmOperation);
            papCallbacks = NULL;
            break;
    }
    rc = forkBthCallbacksCall(papCallbacks, pForkHandle, enmOperation);

    LIBCLOG_RETURN_INT(rc);
}






/*//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\*/
/*//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\*/
/*//                                                                                                                \\*/
/*//                                                                                                                \\*/
/*//        Here follows functions as they are called in the natural flow of the fork().                            \\*/
/*//                                                                                                                \\*/
/*//                                                                                                                \\*/
/*//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\*/
/*//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\*/


/**
 * Checks if the process can be forked or not.
 * The requirements are currently that the executable is
 * built with -Zfork, and thus is registered in the fork module list.
 *
 * @returns 0 if forkable.
 * @returns -1 and errno set to EINVAL if not forkable.
 * @param   pModules    Pointer to the head of the module list.
 */
int forkParCanFork(__LIBC_PFORKMODULE pModules)
{
    LIBCLOG_ENTER("pModules=%p\n", (void *)pModules);
    __LIBC_PFORKMODULE pModule = pModules;
    while (pModule)
    {
        if (pModule->fFlags & __LIBC_FORKMODULE_FLAGS_EXECUTABLE)
            LIBCLOG_RETURN_INT(0);
        pModule = pModule->pNext;
    }
    errno = EINVAL;
    LIBCLOG_RETURN_INT(-1);
}


/**
 * Validate that we can handle all the modules in the list.
 *
 * This routine will check the version number of each module and make
 * sure we're able to cope with it.
 *
 * @returns 0 if all modules returns succesfully.
 * @returns negative error code (errno.h) on incompatability.
 * @param   pModules        Head of the module list.
 */
static int forkParValidateModules(__LIBC_PFORKMODULE pModules)
{
    LIBCLOG_ENTER("pModules=%p\n", (void *)pModules);
    unsigned            i = 0;
    __LIBC_PFORKMODULE  pModule;
    for (pModule = pModules; pModule; pModule = pModule->pNext)
    {
        (void)i;
        LIBCLOG_MSG("%p: #%d uVersion=%#10x pfnAtFork=%p papParent=%p papChild=%p pvDataSegBase=%p pvDataSegEnd=%p fFlags=%#010x pNext=%p\n",
                    (void *)pModule, i++, pModule->uVersion, (void *)pModule->pfnAtFork, (void *)pModule->papParent1, (void *)pModule->papChild1,
                    pModule->pvDataSegBase, pModule->pvDataSegEnd, pModule->fFlags, (void *)pModule->pNext);
        if ((pModule->uVersion >> 16) != (__LIBC_FORK_MODULE_VERSION >> 16))
        {
            LIBCLOG_ERROR("Encountered a new and incompatible module version, %#x. pModule=%p\n", pModule->uVersion, (void *)pModule);
            LIBCLOG_ERROR_RETURN_INT(-ENEWVER);
        }
    }
    LIBCLOG_RETURN_INT(0);
}


/**
 * Allocate and initialize the fork handle.
 *
 * Some builtin intelligence is used to determin the  size of the buffer. The
 * size of the all the data segments plus the allocations done by DosEx makes
 * up a sort of maximum. While the size of the DosEx pools are a absolute
 * minimum.
 *
 * The handle is attempted allocated from high memory, but low memory is used
 * in the event of an prehistoric OS/2 version.
 *
 * The fork buffer is owned by the caller upon successful return. The child
 * will go directly into wait on the mutex. I.e. the event sems are only used
 * for accessing the mutex, and the mutex is the semaphore to wait on. This is
 * much more reliable than sleeping on event sems, since mutex sems get
 * invalidated if an owner dies.
 *
 * The handle is created with buffer flushing disabled.
 *
 * @returns Pointer to fork handle on success.
 * @returns NULL on failure, the assumed failure is memory shortage.
 *
 * @param   pModules        Pointer to module list.
 *                          This is used to sum the data segments.
 * @param   pvStackRet      ESP for the child when returning from fork.
 * @param   pvForkRet       Return address for the child.
 */
__LIBC_PFORKHANDLE   forkParAllocHandle(__LIBC_PFORKMODULE pModules, void *pvStackRet, void *pvForkRet)
{
    LIBCLOG_ENTER("pModules=%p pvStackRet=%p pvForkRet=%p\n", (void *)pModules, pvStackRet, pvForkRet);
    PVOID               pv;
    __LIBC_PFORKHANDLE  pForkHandle = NULL;
    int                 rc;
    size_t              cb;
    size_t              cbMin;
    size_t              cbSum;
    ULONG               cMBLimit;
    __LIBC_PFORKMODULE  pModule;
    PPIB                pPib;
    PTIB                pTib;

    /*
     * Guess how much memory is required.
     */
    /* collect data */
    rc = DosQuerySysInfo(QSV_VIRTUALADDRESSLIMIT, QSV_VIRTUALADDRESSLIMIT, &cMBLimit, sizeof(cMBLimit));
    if (rc)
        cMBLimit = 512;
    __libc_dosexGetMemStats(&cbMin, &cbSum);
    for (pModule = pModules; pModule; pModule = pModule->pNext)
        cbSum += (((uintptr_t)pModule->pvDataSegEnd + 0xfff) & ~0xfff) - ((uintptr_t)pModule->pvDataSegBase & ~0xfff) + 64;

    /* calc extremes */
    cbMin += sizeof(__LIBC_FORKHANDLE) + 16384;
    if (cbMin < __LIBC_FORK_BUFFER_SIZE_MIN)
        cbMin = __LIBC_FORK_BUFFER_SIZE_MIN;
    cbSum += sizeof(__LIBC_FORKHANDLE) + 256*1024;
    if (cbMin > cbSum)
        cbSum = cbMin;

    /* guess */
    if (cMBLimit > 512)
    {
        cb = __LIBC_FORK_BUFFER_SIZE_MAX;
        if (cb > cbSum + 1024*1024)
            cb = cbSum;
    }
    else
    {
        cb = __LIBC_FORK_BUFFER_SIZE_MAX / 4;
        if (cb > cbSum + 256*1024)
            cb = cbSum;
    }
    if (cb < cbMin)
        cb = cbMin;
    LIBCLOG_MSG("cbMin=%d cbSum=%d cb=%d\n", cbMin, cbSum, cb);

    /*
     * Allocate handle.
     */
    for (;;)
    {
        cb = (cb + 0xffff) & ~0xffff;
        rc = DosAllocSharedMem(&pv, NULL, cb, (cMBLimit > 512 ? OBJ_ANY : 0) | PAG_READ | PAG_WRITE | PAG_COMMIT | OBJ_GETTABLE);
        if (!rc)
            break;

        /* Try decrease the handle size. */
        cb -= 512*1024;
        if (cb < cbMin || cb > __LIBC_FORK_BUFFER_SIZE_MAX)
        {
            LIBC_ASSERTM_FAILED("Failed to allocated memory cb=%d cbMin=%d cbSum=%d, rc=%d,\n", cb, cbMin, cbSum, rc);
            LIBCLOG_RETURN_P(NULL);
        }
    }

    /*
     * Initialize the handle.
     */
    pForkHandle                 = pv;
    pForkHandle->uVersion       = __LIBC_FORK_VERSION;
    DosGetInfoBlocks(&pTib, &pPib);
    pForkHandle->pidParent      = pPib->pib_ulpid;
    pForkHandle->pidChild       = -1;
    pForkHandle->cb             = cb;
    rc = DosCreateMutexSem(NULL, &pForkHandle->hmtx, DC_SEM_SHARED, TRUE);
    if (!rc)
    {
        rc = DosCreateEventSem(NULL, &pForkHandle->hevParent, DC_SEM_SHARED, FALSE);
        if (!rc)
        {
            rc = DosCreateEventSem(NULL, &pForkHandle->hevChild, DC_SEM_SHARED, TRUE);
            if (!rc)
            {
                pForkHandle->cmsecTimeout           = __LIBC_FORK_SEM_TIMEOUT;
                pForkHandle->pfnDoFork              = forkChlDoFork;

                pForkHandle->pfnDuplicatePages      = forkPrmDuplicatePages;
                pForkHandle->pfnInvoke              = forkPrmInvoke;
                pForkHandle->pfnFlush               = forkPrmFlush;
                pForkHandle->pfnCompletionCallback  = forkPrmCompletionCallback;

                pForkHandle->pvStackLow             = pTib->tib_pstack;
                pForkHandle->pvStackHigh            = pTib->tib_pstacklimit;
                pForkHandle->pvStackRet             = pvStackRet;
                pForkHandle->pvForkRet              = pvForkRet;
                if ((uintptr_t)pvStackRet - (uintptr_t)pTib->tib_pstack < (uintptr_t)pForkHandle->pvStackHigh - (uintptr_t)pTib->tib_pstack)
                {
                    pForkHandle->papfnCompletionCallbacks = (__LIBC_PFORKCOMPLETIONCALLBACK)((uintptr_t)pForkHandle + pForkHandle->cb);
                    pForkHandle->pBuf               = (__LIBC_PFORKPKGHDR)(((uintptr_t)(pForkHandle + 1) + 0x1f) & ~0x1f);
                    pForkHandle->cbBuf              = cb - ((uintptr_t)pForkHandle->pBuf - (uintptr_t)pForkHandle);
                    pForkHandle->pBufCur            = pForkHandle->pBuf;
                    pForkHandle->cbBufLeft          = pForkHandle->cbBuf;
                    pForkHandle->enmStage           = __LIBC_FORK_STAGE_PRE_EXEC;

                    LIBCLOG_MSG("Created fork handle %p of size %d and buffer size %d; hmtx=%08lx hevParent=%08lx hevChild=%08lx\n",
                                (void *)pForkHandle, pForkHandle->cb, pForkHandle->cbBuf,
                                pForkHandle->hmtx, pForkHandle->hevParent, pForkHandle->hevChild);
                    LIBCLOG_RETURN_P(pForkHandle);
                }
                else
                    LIBC_ASSERTM_FAILED("stack is not within the stack in the TIB. pstack=%p pstacklimit=%p pvStackRet=%p\n",
                                        pTib->tib_pstack, pTib->tib_pstacklimit, pvStackRet);
                DosCloseEventSem(pForkHandle->hevParent);
            }
            else
                LIBC_ASSERTM_FAILED("DosCreateEventSem(child) -> %d\n", rc);
            DosCloseEventSem(pForkHandle->hevChild);
        }
        else
            LIBC_ASSERTM_FAILED("DosCreateEventSem(parent) -> %d\n", rc);
        DosCloseMutexSem(pForkHandle->hmtx);
    }
    else
        LIBC_ASSERTM_FAILED("DosCreateMutexSem() -> %d\n", rc);
    DosFreeMem(pv);
    LIBCLOG_RETURN_P(NULL);
}


/**
 * The __LIBC_FORK_OP_EXEC_PARENT run.
 *
 * This run is vital for DosEx to work and for loaded modules to not crash the child
 * after data segments are copied.
 *
 *
 * @returns 0 on success.
 * @returns negative error code (errno.h) on failure.
 * @returns positive error code (errno.h) as warning.
 *
 * @param   pForkHandle Forkhandle. Caller owns the buffer on call and will remain
 *                      the owner during this call.
 * @param   pModules    Fork modules.
 */
int forkParRunPreExecParent(__LIBC_PFORKHANDLE pForkHandle, __LIBC_PFORKMODULE pModules)
{
    LIBCLOG_ENTER("pForkHandle=%p pModules=%p\n", (void *)pForkHandle, (void *)pModules);
    int rc = forkBthProcessModules(pForkHandle, pModules, __LIBC_FORK_OP_EXEC_PARENT);
    LIBCLOG_MSG("%d bytes of the fork buffer was used during EXEC_PARENT.\n", pForkHandle->cbBuf - pForkHandle->cbBufLeft);
    LIBCLOG_RETURN_INT(rc);
}


/**
 * Executes the child forked process and waits for it to finish the initial
 * child run.
 *
 * The fork buffer will be released before DosExecPgm() so the child can
 * do init time processing of the DosEx stuff for instance. After DosExecPgm()
 * we'll be wait on the event semaphore and subseqently on the mutex till
 * we obtain owner ship of the buffer again. The buffer will then contain
 * tidings from the child and we will return accordingly after the buffer data
 * have been processed.
 *
 * Until DosExecPgm returns the buffer will not permit flushing, the child
 * will flip this as it enters __libc_ForkRegisterModule() from it's executable.
 *
 * @returns 0 on success.
 * @returns negative error code (errno.h) on failure.
 * @returns positive error code (errno.h) as warning.
 *
 * @param   pForkHandle Forkhandle. Caller owns the buffer on call, and
 *                      it will own it again upon return.
 * @param   pModules    Fork modules.
 */
int forkParExec(__LIBC_PFORKHANDLE pForkHandle, pid_t  *ppid)
{
    LIBCLOG_ENTER("pForkHandle=%p ppid=%p\n", (void *)pForkHandle, (void *)ppid);
    __LIBC_PSPMPROCESS  pProcessChild;
    char                szPgm[CCHMAXPATH];
    int                 rc;

    /*
     * Create embryo process.
     */
    pProcessChild = __libc_spmCreateEmbryo(pForkHandle->pidParent);
    if (!pProcessChild)
        LIBCLOG_RETURN_INT(-errno);
    pProcessChild->pvForkHandle = pForkHandle;

    /*
     * End and Release the fork buffer.
     */
    forkBthBufferEnd(pForkHandle);
    rc = forkBthBufferGive(pForkHandle, __LIBC_FORK_CTX_PARENT);
    if (rc < 0)
    {
        __libc_spmRelease(pProcessChild);
        LIBCLOG_RETURN_INT(rc);
    }

    /*
     * Spawn the child.
     */
    pForkHandle->enmStage = __LIBC_FORK_STAGE_EXEC;
    rc = DosQueryModuleName(fibGetExeHandle(), sizeof(szPgm), &szPgm[0]);
    if (!rc)
    {
        char        szErr[64];
        RESULTCODES rsc = {0,0};
        #if 1
        rc = DosExecPgm(szErr, sizeof(szErr), EXEC_ASYNCRESULT, (PCSZ)fibGetCmdLine(), NULL, &rsc, (PCSZ)&szPgm[0]);
        #else /* debugging checks */
        rc = DosExecPgm(szErr, sizeof(szErr), EXEC_ASYNCRESULT, (PCSZ)"forking\0forking\0", NULL, &rsc, (PCSZ)&szPgm[0]);
        #endif
        if (!rc)
        {
            __atomic_cmpxchg32((volatile uint32_t *)(void *)&pProcessChild->pid, (uint32_t)rsc.codeTerminate, 0);

            *ppid = rsc.codeTerminate;
            pForkHandle->fFlushable = 1;
            LIBCLOG_MSG("DosExecPgm created process 0x%04x (%d); pidChild=0x%04x\n", *ppid, *ppid, pForkHandle->pidChild);

            /*
             * Wait for the buffer till a NEXT packet is found or we fail.
             */
            rc = forkBthBufferWait(pForkHandle, __LIBC_FORK_CTX_PARENT);
            if (!rc)
            {
                int fNext = 0;
                LIBC_ASSERTM(pForkHandle->pidChild == *ppid,
                             "Invalid child process id! pidChild=%04x *ppid=%04x\n", pForkHandle->pidChild, *ppid);
                rc = forkBthBufferProcess(pForkHandle, __LIBC_FORK_CTX_PARENT, &fNext);
                if (!rc && !fNext)
                    rc = forkBthBufferGiveWaitNext(pForkHandle, __LIBC_FORK_CTX_PARENT);
            }
        }
        else
        {
            LIBC_ASSERTM_FAILED("DosExecPgm(,,,%s,,,%s..) failed with rc=%d, szErr=%s\n", szPgm, fibGetCmdLine(), rc, szErr);
            rc = -__libc_native2errno(rc);
        }
    }
    else
    {
        LIBC_ASSERTM_FAILED("DosQueryModuleName(%lx,,) failed with rc=%d\n", fibGetExeHandle(), rc);
        rc = -__libc_native2errno(rc);
    }

#if 1
    if (pProcessChild->enmState == __LIBC_PROCSTATE_EMBRYO)
    {
        LIBCLOG_MSG("The embryo %p wasn't used. pNext=%p pPrev=%p\n", (void *)pProcessChild,
                    (void *)pProcessChild->pNext, (void *)pProcessChild->pPrev);
        __libc_SpmCheck(0, 1);
    }
#endif
    __libc_spmRelease(pProcessChild);
    LIBCLOG_RETURN_INT(rc);
}


/**
 * Opens a fork handle from the child process.
 *
 * The child will do this during __libc_ForkRegisterModule(). It will do so
 * for all versions of LIBC and uses the handle state to check if the semaphores
 * and memory actually needed opening. The Child process will only have one
 * reference to the memory and semaphores.
 *
 * @returns Pointer to for handle on success.
 * @returns NULL on failure.
 * @param   pvForkHandle    Pointer to the fork handle as it appeares in the
 *                          LIBC Shared Process Management process.
 */
__LIBC_PFORKHANDLE   forkChlOpenHandle(void *pvForkHandle)
{
    LIBCLOG_ENTER("pvForkHandle=%p\n", pvForkHandle);
    __LIBC_PFORKHANDLE pForkHandle = NULL;
    int rc;

    rc = DosGetSharedMem(pvForkHandle, PAG_READ | PAG_WRITE);
    if (!rc)
    {
        pForkHandle = (__LIBC_PFORKHANDLE)pvForkHandle;
        if (pForkHandle->enmStage == __LIBC_FORK_STAGE_EXEC)
        {
            if (pForkHandle->uVersion == __LIBC_FORK_VERSION)
            {
                rc = DosOpenMutexSem(NULL, &pForkHandle->hmtx);
                if (!rc)
                {
                    rc = DosOpenEventSem(NULL, &pForkHandle->hevParent);
                    if (!rc)
                    {
                        rc = DosOpenEventSem(NULL, &pForkHandle->hevChild);
                        if (!rc)
                        {
                            /*
                             * Succesfully opened the handle for the first time.
                             * Set the pidChild.
                             */
                            PTIB pTib;
                            PPIB pPib;
                            DosGetInfoBlocks(&pTib, &pPib);
                            pForkHandle->pidChild = pPib->pib_ulpid;
                            LIBC_ASSERTM(pForkHandle->pidParent == pPib->pib_ulppid,
                                         "Invalid parent pid... pidParent=%04x pib_ulppid=%04lx (pib_ulpid=%04lx).\n",
                                         pForkHandle->pidParent, pPib->pib_ulppid, pPib->pib_ulpid);
                            LIBCLOG_RETURN_P(pForkHandle);
                        }
                        else
                            LIBC_ASSERTM_FAILED("DosOpenEventSem(NULL,..:{%lx}) -> %d (child)\n", pForkHandle->hevChild, rc);
                    }
                    else
                        LIBC_ASSERTM_FAILED("DosOpenEventSem(NULL,..:{%lx}) -> %d (parent)\n", pForkHandle->hevParent, rc);
                }
                else
                    LIBC_ASSERTM_FAILED("pForkHandle->uVersion (%#x) != __LIBC_FORK_VERSION (%#x)\n", pForkHandle->uVersion, __LIBC_FORK_VERSION);
            }
            else
                LIBC_ASSERTM_FAILED("DosOpenMutexSem(NULL,..:{%lx}) -> %d\n", pForkHandle->hmtx, rc);

            /* indicate failure. */
            pForkHandle = NULL;
            DosFreeMem(pForkHandle);
        }
        /* If it's an extra open we must not do DosFreeMem() since OS/2
         * obviously doesn't keeps a per process open counter on memory objects.
         */
    }
    else
        LIBC_ASSERTM_FAILED("DosGetSharedMem(%p,..) -> %d\n", pvForkHandle, rc);

    LIBCLOG_RETURN_P(pForkHandle);
}


/**
 * __LIBC_PFORKHANDLE::pfnDoFork for a general description.
 *
 * @returns 0 on success.
 * @returns -errno on failure.
 * @param   pForkHandle     Fork handle.
 * @param   pModule         The module it's called for.
 * @param   fExecutable     Indicates that the module is the executable module
 *                          and that the main part of fork() can start.
 */
int                   forkChlDoFork(__LIBC_PFORKHANDLE pForkHandle, __LIBC_PFORKMODULE pModule, int fExecutable)
{
    LIBCLOG_ENTER("pForkHandle=%p pModule=%p fExecutable=%d\n", (void *)pForkHandle, (void *)pModule, fExecutable);
    int         rc;
    uintptr_t   uNewESP;

    /*
     * Block signals while we're in here. We cannot handle them properly anyway.
     */
    ULONG cNesting = 0;
    DosEnterMustComplete(&cNesting);

    /*
     * Open and process the buffer on first call.
     *
     * Because of DosLoadModuleEx processing, we can end up here in while
     * processing the initial fork buffer. This means that if any of those
     * DLLS tries to talk to the parent at __LIBC_FORK_EXEC_CHILD time the
     * fork() will fail. Fortunately, this is very unlikely. (Otherwise, we
     * should've delayed those pfnAtFork() calls.)
     */
    if (pForkHandle->enmStage == __LIBC_FORK_STAGE_EXEC)
    {
        rc = forkBthBufferWait(pForkHandle, __LIBC_FORK_CTX_CHILD);
        if (rc < 0)
        {
            DosExitMustComplete(&cNesting);
            LIBCLOG_RETURN_INT(rc);
        }
        pForkHandle->enmStage = __LIBC_FORK_STAGE_INIT_CHILD;
        rc = forkBthBufferProcess(pForkHandle, __LIBC_FORK_CTX_CHILD, NULL);
        if (rc < 0)
        {
            DosExitMustComplete(&cNesting);
            LIBCLOG_RETURN_INT(rc);
        }
    }

    /*
     * Do the exec child call.
     */
    rc = pModule->pfnAtFork(pModule, pForkHandle, __LIBC_FORK_OP_EXEC_CHILD);
    if (rc < 0)
    {
        DosExitMustComplete(&cNesting);
        LIBCLOG_RETURN_INT(rc);
    }

    /* done? */
    if (!fExecutable)
    {
        DosExitMustComplete(&cNesting);
        LIBCLOG_RETURN_INT(0);
    }

    /*
     * Do fork.
     */
    pForkHandle->enmStage = __LIBC_FORK_STAGE_INIT_DONE;

    /*
     * Relocate to the fork stack address resuming execution in forkChlDoFork2().
     * The stack might need allocating if fork() was not called in thread
     * one. If not we'll have to touch pages as the stack of thread 1 might
     * be using guard pages.
     */
    uNewESP = ((uintptr_t)pForkHandle->pvStackRet - 0xfff) & ~0xfff;
    if (pForkHandle->fStackAlloc)
    {
        rc = DosAllocMemEx(pForkHandle->pvStackLow,
                           (uintptr_t)pForkHandle->pvStackHigh - (uintptr_t)pForkHandle->pvStackLow,
                           PAG_READ | PAG_WRITE | PAG_COMMIT);
        if (rc)
            forkChlFatalError(pForkHandle, -__libc_native2errno(rc), NULL);
    }
    else
    {
        volatile char *pch = (volatile char *)pForkHandle->pvStackHigh - 16;
        while ((uintptr_t)pch >= uNewESP)
        {
            *pch = *pch;
            pch -= 0x1000;
        }
    }
    __asm__ __volatile__("movl  %0, %%esp\n\t"
                         "pushl %1\n\t"
                         "call  _forkChlDoFork2\n\t"
                         "movl  %2, %%esp\n\t"
                         "jmp   *%3\n\t"
                         ::"r" (uNewESP),
                           "r" (pForkHandle),
                           "D" ((uintptr_t)pForkHandle->pvStackRet),
                           "S" (pForkHandle->pvForkRet));
    /* just for referencing the functions so GCC doesn't optimize them away - we'll never get here!!! */
    return forkChlDoFork2(pForkHandle) + forkParDo(NULL, NULL, NULL);
}


/**
 * This worker is called on after relocating the stack.
 */
CRT_DATA_USED
int                  forkChlDoFork2(__LIBC_PFORKHANDLE pForkHandle)
{
    LIBCLOG_ENTER("pForkHandle=%p\n", (void *)pForkHandle);
    int                 rc;
    __LIBC_PFORKMODULE  pModules;
    PTIB                pTib;
    PPIB                pPib;
    EXCEPTIONREGISTRATIONRECORD XcptRegRec;

    /*
     * Reregister the exception handler but now replacing the existing chain.
     */
    DosGetInfoBlocks(&pTib, &pPib);
    XcptRegRec.prev_structure = END_OF_CHAIN;
    XcptRegRec.ExceptionHandler = forkChlExceptionHandler;
    pTib->tib_pexchain = &XcptRegRec;
    g_pForkHandle = pForkHandle;

    /*
     * The __LIBC_FORK_OP_EXEC_CHILD run is complete,
     * return the buffer to the parent and process the buffer till
     * the next child run is encountered.
     */
    rc = forkBthBufferSpace(pForkHandle, offsetof(__LIBC_FORKPKGHDR, u.Next.achStart));
    if (rc < 0)
        forkChlFatalError(pForkHandle, rc, NULL);
    forkBthBufferNext(pForkHandle);
    forkBthBufferEnd(pForkHandle);
    rc = forkBthBufferGiveWaitNext(pForkHandle, __LIBC_FORK_CTX_CHILD);
    if (rc < 0)
        forkChlFatalError(pForkHandle, rc, NULL);

    /*
     * Get the modules for the current process.
     *
     * We have to do this _after_ init and _after_ the initial copying
     * of data segments because the parent fork run can reorder the
     * fork module list.
     */
    pModules = forkBthGetModules();
    LIBC_ASSERTM(pModules, "No fork modules in child process!!!!\n");
    if (!pModules)
        forkChlFatalError(pForkHandle, -EDOOFUS, NULL);

    /*
     * Do the __LIBC_FORK_OP_FORK_CHILD run.
     */
    rc = forkChlRunForkChild(pForkHandle, pModules);
    if (rc < 0)
        forkChlFatalError(pForkHandle, rc, NULL);

    /*
     * Do completion child calls (success).
     */
    forkChlCompletionChild(pForkHandle, rc);

    /*
     * Send the buffer back to the parent and tell it we're done.
     */
    forkBthBufferNext(pForkHandle);
    forkBthBufferEnd(pForkHandle);
    forkBthBufferGive(pForkHandle, __LIBC_FORK_CTX_CHILD); /* may fail all it wants now. */

    /*
     * Close the fork handle and NULL the handle pointer in the process structure.
     */
    forkBthCloseHandle(pForkHandle, __LIBC_FORK_CTX_CHILD);
    __LIBC_PSPMPROCESS pProcess = __libc_spmSelf();
    LIBC_ASSERTM(pProcess, "No self process!!!\n");
    pProcess->pvForkHandle = NULL;

    /*
     * The return here will go back to the inline assmebly which called us in forkChlDoFork()
     * and restore the stack pointer to the __libc_Back_processFork() and jump to the fork_ret
     * label in __libc_Back_processFork().
     * __libc_Back_processFork() will restore the exception handler chain, exit
     * the must-complete section and release the execute semaphore.
     */
    __libc_spmExeInited();
    pTib->tib_pexchain = END_OF_CHAIN;
    LIBCLOG_RETURN_INT(0);
}


/**
 * The main fork run in the parent context.
 *
 * @returns 0 on success.
 * @returns negative error code (errno.h) on failure.
 * @returns positive error code (errno.h) on warning.
 * @param   pForkHandle     The fork handle.
 * @param   pModules        Pointer to the fork module list.
 *                          (At this point the callbacks are already sorted.)
 */
int                  forkParRunForkParent(__LIBC_PFORKHANDLE pForkHandle, __LIBC_PFORKMODULE pModules)
{
    LIBCLOG_ENTER("pForkHandle=%p pModules=%p\n", (void *)pForkHandle, (void *)pModules);
    int rc;

    /*
     * Duplicate the stack.
     */
    rc = pForkHandle->pfnDuplicatePages(pForkHandle, pForkHandle->pvStackRet, pForkHandle->pvStackHigh,
                                        __LIBC_FORK_FLAGS_ALL);
    if (rc < 0)
    {
        LIBC_ASSERTM_FAILED("Failed to duplicate stack rc=%d\n", rc);
        LIBCLOG_RETURN_INT(rc);
    }

    /*
     * Call the modules as normal.
     */
    rc = forkBthProcessModules(pForkHandle, pModules, __LIBC_FORK_OP_FORK_PARENT);
    LIBCLOG_RETURN_INT(rc);
}


/**
 * The main fork run in the child context.
 *
 * @returns 0 on success.
 * @returns negative error code (errno.h) on failure.
 * @returns positive error code (errno.h) on warning.
 * @param   pForkHandle     The fork handle.
 * @param   pModules        Pointer to the fork module list.
 *                          (At this point the callbacks are already sorted.)
 */
int                  forkParRunForkChild(__LIBC_PFORKHANDLE pForkHandle, __LIBC_PFORKMODULE pModules)
{
    LIBCLOG_ENTER("pForkHandle=%p\n", (void *)pForkHandle);
    int                 rc;

    /*
     * Add next and end packets to the buffer flushing it first if we're out of space.
     */
    rc = forkBthBufferSpace(pForkHandle, offsetof(__LIBC_FORKPKGHDR, u.Next.achStart));
    if (rc < 0)
        LIBCLOG_RETURN_INT(rc);
    forkBthBufferNext(pForkHandle);
    forkBthBufferEnd(pForkHandle);

    /*
     * Wait for the buffer processing it till either
     * an abort or a next run packet is encountered.
     */
    rc = forkBthBufferGiveWaitNext(pForkHandle, __LIBC_FORK_CTX_PARENT);

    LIBCLOG_RETURN_INT(rc);
}


/**
 * The main fork run in the child context.
 *
 * @returns 0 on success.
 * @returns negative error code (errno.h) on failure.
 * @returns positive error code (errno.h) on warning.
 * @param   pForkHandle     The fork handle.
 * @param   pModules        Pointer to the fork module list.
 */
int                  forkChlRunForkChild(__LIBC_PFORKHANDLE pForkHandle, __LIBC_PFORKMODULE pModules)
{
    LIBCLOG_ENTER("pForkHandle=%p pModules=%p\n", (void *)pForkHandle, (void *)pModules);
    int rc = forkBthProcessModules(pForkHandle, pModules, __LIBC_FORK_OP_FORK_CHILD);
    LIBCLOG_RETURN_INT(rc);
}


/**
 * Execute registered completion callbacks in the child context.
 * This is called when the parent context fails.
 *
 * @param   pForkHandle Fork handle.
 * @param   rc          The error code (errno.h) for the fork operation.
 *                      Negative means error, zero or positive means success.
 */
void                 forkParCompletionChild(__LIBC_PFORKHANDLE pForkHandle, int rc)
{
    LIBCLOG_ENTER("pForkHandle=%p rc=%d\n", (void *)pForkHandle, rc);
    /*
     * Purge the fork buffer on failure.
     */
    forkBthBufferReset(pForkHandle);
    forkBthBufferAbort(pForkHandle, rc);
    forkBthBufferEnd(pForkHandle);

    /*
     * Give and wait on the fork buffer.
     */
    rc = forkBthBufferGive(pForkHandle, __LIBC_FORK_CTX_CHILD);
    if (rc >= 0)
        rc = forkBthBufferWait(pForkHandle, __LIBC_FORK_CTX_CHILD);
    if (rc < 0 && pForkHandle->pidChild > 0)
        DosKillProcess(DKP_PROCESS, pForkHandle->pidChild); /** @todo move me into the above functions? */

    LIBCLOG_RETURN_VOID();
}


/**
 * Execute registered completion callbacks in the parent context.
 *
 * @param   pForkHandle Fork handle.
 * @param   rc          The error code (errno.h) for the fork operation.
 *                      Negative means error, zero or positive means success.
 */
void                 forkChlCompletionChild(__LIBC_PFORKHANDLE pForkHandle, int rc)
{
    LIBCLOG_ENTER("pForkHandle=%p rc=%d\n", (void *)pForkHandle, rc);
    forkBothCompletion(pForkHandle, rc, __LIBC_FORK_CTX_CHILD, &pForkHandle->iCompletionCallbackChild);
    LIBCLOG_RETURN_VOID();
}


/**
 * Execute registered completion callbacks in the parent context.
 *
 * @param   pForkHandle Fork handle.
 * @param   rc          The error code (errno.h) for the fork operation.
 *                      Negative means error, zero or positive means success.
 */
void                 forkParCompletionParent(__LIBC_PFORKHANDLE pForkHandle, int rc)
{
    LIBCLOG_ENTER("pForkHandle=%p rc=%d\n", (void *)pForkHandle, rc);
    forkBothCompletion(pForkHandle, rc, __LIBC_FORK_CTX_PARENT, &pForkHandle->iCompletionCallbackParent);
    LIBCLOG_RETURN_VOID();
}


/**
 * Execute completion callbacks for a context.
 *
 * @param   pForkHandle             Fork Handle.
 * @param   rc                      The error code (errno.h) for the fork operation.
 *                                  Negative means error, zero or positive means success.
 * @param   enmCtx                  The Context.
 * @param   piCompletionCallaback   Pointer to the variable containing the current completion callback index.
 *                                  This is both input and output.
 */
void                 forkBothCompletion(__LIBC_PFORKHANDLE pForkHandle, int rc, __LIBC_FORKCTX enmCtx, unsigned volatile *piCompletionCallback)
{
    /*
     * Call completion callbacks in reverse registration order.
     *
     * This is a tiny bit uncertain stuff because we might not be the owner of the
     * fork buffer and thus the other side might add new callbacks just now.
     */
    unsigned                        cCCs = (unsigned)pForkHandle->cCompletionCallbacks;
    __LIBC_PFORKCOMPLETIONCALLBACK  papfn = pForkHandle->papfnCompletionCallbacks;
    unsigned                        iCC;
    while ((iCC = *piCompletionCallback) < cCCs)
    {
        *piCompletionCallback += 1;
        if (    papfn[iCC].enmContext == __LIBC_FORK_CTX_BOTH
            ||  papfn[iCC].enmContext == enmCtx)
            papfn[iCC].pfnCallback(papfn[iCC].pvArg, rc, enmCtx);
    }
}


/**
 * Frees the fork handle and resources associated with it.
 *
 * This is called from both parent and child sides when a fork session is
 * at an end (be that good or bad). The child will make use of this
 * call during the completion run. The parent will do it from the main
 * driver after the parent completion run.
 *
 * @param   pForkHandle     For handle to free.
 * @param   enmContext      Caller context.
 */
void                 forkBthCloseHandle(__LIBC_PFORKHANDLE pForkHandle, __LIBC_FORKCTX enmContext)
{
    int rc;

    /*
     * Close the semaphores and free the memory.
     */
    g_pForkHandle = NULL;
    rc = DosCloseEventSem(pForkHandle->hevChild);
    LIBC_ASSERTM(!rc, "DosCloseEventSem(%lx) failed rc=%d (child)\n", pForkHandle->hevChild, rc);
    rc = DosCloseEventSem(pForkHandle->hevParent);
    LIBC_ASSERTM(!rc, "DosCloseEventSem(%lx) failed rc=%d (parent)\n", pForkHandle->hevParent, rc);
    rc = DosCloseMutexSem(pForkHandle->hmtx);
    if (rc == ERROR_SEM_BUSY)
    {
        DosReleaseMutexSem(pForkHandle->hmtx);
        rc = DosCloseMutexSem(pForkHandle->hmtx);
    }
    LIBC_ASSERTM(!rc, "DosCloseMutexSem(%lx) failed rc=%d\n", pForkHandle->hmtx, rc);
    rc = DosFreeMem(pForkHandle);
    LIBC_ASSERTM(!rc, "DosFreeMem(%p) failed rc=%d\n", (void *)pForkHandle, rc);
    rc = rc;
}


/*//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\*/
/*//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\*/
/*//                                                                                                                \\*/
/*//                                                                                                                \\*/
/*//        Here follows functions which can be called multiple times and at no particual time of the fork().       \\*/
/*//                                                                                                                \\*/
/*//                                                                                                                \\*/
/*//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\*/
/*//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\*/


/**
 * Duplicating a number of pages from pvStart to pvEnd.
 *
 * @returns 0 on success.
 * @returns appropriate negative error code (errno.h) on failure.
 *
 * @param   pForkHandle Handle of the current fork operation.
 * @param   pvStart     Pointer to start of the pages. Rounded down.
 * @param   pvEnd       Pointer to end of the pages. Rounded up.
 * @param   fFlags      __LIBC_FORK_FLAGS_* defines.
 */
int forkPrmDuplicatePages(__LIBC_PFORKHANDLE pForkHandle, void *pvStart, void *pvEnd, unsigned fFlags)
{
    LIBCLOG_ENTER("pForkHandle=%p pvStart=%p pvEnd=%p fFlags=%#x\n", (void *)pForkHandle, pvStart, pvEnd, fFlags);
    size_t              cbRange;

    /*
     * Page align and validate input.
     */
    pvStart = (void *)((uintptr_t)pvStart & ~0xfff);
    pvEnd   = (void *)(((uintptr_t)pvEnd + 0xfff) & ~0xfff);
    if (pvEnd < pvStart)
    {
        LIBC_ASSERTM_FAILED("pvEnd (%p) < pvStart (%p)\n", pvEnd, pvStart);
        LIBCLOG_RETURN_INT(-EINVAL);
    }
    cbRange = (char *)pvEnd - (char *)pvStart;
    if (cbRange > 0x20000000)
    {
        LIBC_ASSERTM_FAILED("cbRange (%#x) > 512MB\n", cbRange);
        LIBCLOG_RETURN_INT(-EINVAL);
    }
    if (fFlags & ~(__LIBC_FORK_FLAGS_ONLY_DIRTY | __LIBC_FORK_FLAGS_ALL | __LIBC_FORK_FLAGS_PAGE_ATTR | __LIBC_FORK_FLAGS_ALLOC_FLAGS | __LIBC_FORK_FLAGS_ALLOC_MASK))
    {
        LIBC_ASSERTM_FAILED("invalid fFlags(%08x)\n", fFlags);
        LIBCLOG_RETURN_INT(-EINVAL);
    }

    /* we assume this: */
    LIBC_ASSERT(__LIBC_FORK_FLAGS_ONLY_DIRTY == 0);

    /*
     * Space to a header + empty chunk?
     */
    if (pForkHandle->cbBufLeft < offsetof(__LIBC_FORKPKGHDR, u.Duplicate.achStart) + offsetof(FORKPGCHUNK, achData))
    {
        int rc = forkPrmFlush(pForkHandle);
        if (rc < 0)
            LIBCLOG_RETURN_INT(rc);
    }

    /*
     * Outer packing loop.
     */
    for (;;)
    {
        int                 rc;
        __LIBC_PFORKPKGHDR  pHdr = pForkHandle->pBufCur;
        PFORKPGCHUNK        pChunk = (PFORKPGCHUNK)&pHdr->u.Duplicate.achStart[0];
        size_t              cbPacket = offsetof(__LIBC_FORKPKGHDR, u.Duplicate.achStart);
        int                 fFlush = 0;
        memcpy(pHdr->szMagic, __LIBC_FORK_HDR_MAGIC, sizeof(__LIBC_FORK_HDR_MAGIC));
        pHdr->enmType =__LIBC_FORK_HDR_TYPE_DUPLICATE;
        pHdr->u.Duplicate.fFlags = fFlags;

        /*
         * Inner page packing loop.
         */
        do
        {
            pChunk->pv = pvStart;
            pChunk->cb = pChunk->cbVirt = cbRange;
            pChunk->offData = 64 - ((uintptr_t)&pChunk->achData[0] & 63);
            if (pChunk->offData == 64)
                pChunk->offData = 0;    /* fixme: probably could do this without if's. */


            /*
             * Query memory flags if required.
             */
            if ((fFlags & __LIBC_FORK_FLAGS_PAGE_ATTR) || !(fFlags & __LIBC_FORK_FLAGS_ALL))
            {
                ULONG cb = cbRange;
                ULONG fl = PAG_READ | PAG_WRITE | PAG_EXECUTE | PAG_GUARD | PAG_COMMIT | PAG_DECOMMIT;
                rc = DosQueryMem(pvStart, &cb, &fl);
                if (rc)
                    rc = DosQueryMem(pvStart, &cb, &fl); /* for some warp4 fixpack this might work the 2nd time... */
                if (rc)
                {
                    LIBC_ASSERTM_FAILED("DosQueryMem(%p, ..:{%x}, ..:{%x}) failed with rc=%d\n", pvStart, cbRange,
                                        PAG_READ | PAG_WRITE | PAG_EXECUTE | PAG_GUARD | PAG_COMMIT | PAG_DECOMMIT | PAG_FREE, rc);
                    rc = -__libc_native2errno(rc);
                    LIBCLOG_RETURN_INT(rc);
                }
                LIBCLOG_MSG("DosQueryMem(%p,,) -> cb=%08lx fl=%08lx\n", pvStart, cb, fl);
                pChunk->fFlags = fl;
                pChunk->cb = pChunk->cbVirt = (cb + 0xfff) & ~0xfff; /* result is not necessarily aligned. */
            }

            if (fFlags & __LIBC_FORK_FLAGS_ALL)
            {
                /*
                 * The simple case. Just need to adjust the size to the
                 * available fork buffer space.
                 */
                if (  cbPacket + (uintptr_t)&((PFORKPGCHUNK)0)->achData[pChunk->offData + pChunk->cb]
                    > pForkHandle->cbBufLeft)
                {
                    fFlush = 1;
                    pChunk->cb = pChunk->cbVirt = (pForkHandle->cbBufLeft - cbPacket - pChunk->offData - offsetof(FORKPGCHUNK, achData)) & ~0xfff;
                    if (!pChunk->cb)
                        break;          /* need to flush the buffer. */
                }
                pfnForkBthCopyPages(&pChunk->achData[pChunk->offData], pvStart, pChunk->cb);
            }
            else
            {
                /*
                 * The not so simple case.
                 *
                 * We need to check the state of each page.
                 *
                 * Due to a bug in the API this will have to be called per page. The size
                 * isn't reflecting the size of contiguous pages with the same state. Pitty,
                 * because this is causing a good bit of overhead. :-/
                 *
                 * Actually the DosQueryMemState() API is a bit too cryptic to me right now
                 * to be useful. We'll skip pages DosQueryMem() indicates as uncommitted,
                 * free or guard instead. We ASSUME here that PAG_GUARD is only used for
                 * lazy committing the stack.
                 */
                if (!(pChunk->fFlags & PAG_COMMIT) || (pChunk->fFlags & (PAG_FREE | PAG_GUARD | PAG_DECOMMIT)))
                    pChunk->cb = 0;
                else
                {
                    #if 0
                    enm     { DPLPG_FIRST, DPLPG_NOCOPY, DPLPG_COPY }   enm = DPLPG_FIRST;
                    void   *pvState = pvStart;
                    size_t  cb = pChunk->cb;

                    do
                    {
                        ULONG cb = 0x1000;
                        ULONG fl = ~0;
                        rc = DosQueryMemState(pvState, &cb, fl);
                        if (rc)
                        {
                            LIBC_ASSERTM_FAILED("DosQueryMemState(%p,..:{0x1000},..) failed with rc=%d\n", pvState, rc);
                            rc = -__libc_native2errno(rc);
                            LIBCLOG_RETURN_INT(rc);
                        }

                        /* decide */
                        if (PAG_NPOUT)
                        {
                        }

                        /* next page */
                        cb     -= 0x1000;
                        pvState = (char *)pvState + 0x1000;
                    } while (cb);

                    /*
                     * Recalc sizes and do page copying if needed.
                     */
                    pChunk->cb = pChunk->cbVirt = (char *)pvState - (char *)pvStart;
                    if (enm == DPLPG_NOCOPY)
                        pChunk->cb = 0;
                    else
                    #endif
                    {
                        /*
                         * Copy pages. Just check how much fits in the buffer first.
                         */
                        if (  cbPacket + (uintptr_t)&((PFORKPGCHUNK)0)->achData[pChunk->offData + pChunk->cb]
                            > pForkHandle->cbBufLeft)
                        {
                            pChunk->cb = pChunk->cbVirt = (pForkHandle->cbBufLeft - cbPacket - pChunk->offData - offsetof(FORKPGCHUNK, achData)) & ~0xfff;
                            fFlush = 1;
                            if (!pChunk->cb)
                                break;          /* need to flush the buffer. */
                        }
                        pfnForkBthCopyPages(&pChunk->achData[pChunk->offData], pvStart, pChunk->cb);
                    }
                }
            }


            /* next */
            cbPacket += pChunk->cb + pChunk->offData + offsetof(FORKPGCHUNK, achData);
            cbRange  -= pChunk->cbVirt;
            LIBCLOG_MSG("Chunk %p: pv=%08x cb=%08x cbVirt=%08x fFlags=%08x\n",
                        (void *)pChunk, (uintptr_t)pChunk->pv, pChunk->cbVirt, pChunk->cb, pChunk->fFlags);
            pvStart   = (char *)pvStart + pChunk->cbVirt;
            pChunk    = (PFORKPGCHUNK)&pChunk->achData[pChunk->cb + pChunk->offData];
        } while (cbRange && !fFlush);

        /*
         * Complete the package and check if more todo.
         */
        if (cbPacket != offsetof(__LIBC_FORKPKGHDR, u.Duplicate.achStart))
        {
            pHdr->cb                = cbPacket;
            /** @todo fix bug where we have only flags. */
            LIBC_ASSERT(pForkHandle->cbBufLeft >= pHdr->cb);
            pForkHandle->cbBufLeft -= cbPacket;
            pForkHandle->pBufCur    = (__LIBC_PFORKPKGHDR)((char *)pHdr + cbPacket);
        }

        if (!cbRange)
            break;

        /*
         * Ok, we ran out of buffer space. Flush the buffer.
         */
        rc = forkPrmFlush(pForkHandle);
        if (rc < 0)
            LIBCLOG_RETURN_INT(rc);
    }

    LIBCLOG_RETURN_INT(0);
}


/**
 * Invoke a function in the child process giving it an chunk of input.
 * The function is invoked the next time the fork buffer is flushed,
 * call pfnFlush() if the return code is desired.
 *
 * @returns 0 on success.
 * @returns appropriate negative error code (errno.h) on failure.
 * @param   pForkHandle Handle of the current fork operation.
 * @param   pfn         Pointer to the function to invoke in the child.
 *                      The function gets the fork handle, pointer to
 *                      the argument memory chunk and the size of that.
 *                      The function must return 0 on success, and non-zero
 *                      on failure.
 * @param   pvArg       Pointer to a block of memory of size cbArg containing
 *                      input to be copied to the child and given to pfn upon
 *                      invocation.
 */
int forkPrmInvoke(__LIBC_PFORKHANDLE pForkHandle, int (*pfn)(__LIBC_PFORKHANDLE pForkHandle, void *pvArg, size_t cbArg),
                  void *pvArg, size_t cbArg)
{
    LIBCLOG_ENTER("pForkHandle=%p pfn=%p pvArg=%p cbArg=%u\n", (void *)pForkHandle, (void *)pfn, pvArg, cbArg);
    int                 rc;
    __LIBC_PFORKPKGHDR  pHdr;
    size_t              cb = offsetof(__LIBC_FORKPKGHDR, u.Invoke.achStart) + cbArg;

    /*
     * Check if the buffer is full.
     */
    if (pForkHandle->cbBufLeft < cb)
    {
        rc = forkPrmFlush(pForkHandle);
        if (rc < 0)
            LIBCLOG_RETURN_INT(rc);
        if (pForkHandle->cbBufLeft < cb)
            LIBCLOG_RETURN_INT(-E2BIG);
    }

    /*
     * Put the data into the fork buffer.
     */
    pHdr = pForkHandle->pBufCur;
    memcpy(pHdr->szMagic, __LIBC_FORK_HDR_MAGIC, sizeof(__LIBC_FORK_HDR_MAGIC));
    pHdr->cb                = cb;
    pHdr->enmType           = __LIBC_FORK_HDR_TYPE_INVOKE;
    pHdr->u.Invoke.cbArg    = cbArg;
    pHdr->u.Invoke.pfn      = pfn;
    if (cbArg)
        memcpy(&pHdr->u.Invoke.achStart[0], pvArg, cbArg);

    /* advance fork buffer. */
    pForkHandle->pBufCur    = (__LIBC_PFORKPKGHDR)((char *)pHdr + cb);
    pForkHandle->cbBufLeft -= cb;

    LIBCLOG_RETURN_INT(0);
}


/**
 * Flush the fork() buffer. Meaning taking what ever is in the fork buffer
 * and let the child process it.
 * This might be desired to get the result of a pfnInvoke() in a near
 * synchornous way.
 *
 * @returns 0 on success.
 * @returns appropriate negative error code (errno.h) on failure.
 * @param   pForkHandle Handle of the current fork operation.
 */
int forkPrmFlush(__LIBC_PFORKHANDLE pForkHandle)
{
    LIBCLOG_ENTER("pForkHandle=%p\n", (void *)pForkHandle);
    __LIBC_FORKCTX      enmCtx;
    PPIB                pPib;
    PTIB                pTib;
    int                 rc;

    /*
     * Check state.
     */
    if (!pForkHandle->fFlushable)
    {
        LIBC_ASSERTM_FAILED("Can't flush buffer before DosExecPgm returns! stage=%d\n", pForkHandle->enmStage);
        LIBCLOG_RETURN_INT(-EPERM);
    }

    /*
     * Write end package.
     */
    forkBthBufferEnd(pForkHandle);

    /*
     * Figure out which event sems to use for what.
     */
    DosGetInfoBlocks(&pTib, &pPib);
    if (pPib->pib_ulpid == pForkHandle->pidParent)
        enmCtx = __LIBC_FORK_CTX_PARENT;
    else
        enmCtx = __LIBC_FORK_CTX_CHILD;

    /*
     * Give the buffer to the other process.
     */
    rc = forkBthBufferGive(pForkHandle, enmCtx);
    if (rc < 0)
        LIBCLOG_RETURN_INT(rc);

    /*
     * Wait for the buffer.
     */
    rc = forkBthBufferWait(pForkHandle, enmCtx);
    if (rc < 0)
        LIBCLOG_RETURN_INT(rc);

    /*
     * Process the buffer.
     */
    rc = forkBthBufferProcess(pForkHandle, enmCtx, NULL);
    if (rc < 0)
        LIBCLOG_RETURN_INT(rc);

    LIBCLOG_RETURN_INT(0);
}


/**
 * Register a fork() completion callback.
 *
 * Use this primitive to do post fork() cleanup.
 * The callbacks are executed first in the child, then in the parent. The
 * order is reversed registration order.
 *
 * @returns 0 on success.
 * @returns appropriate non-zero error code on failure.
 * @param   pForkHandle Handle of the current fork operation.
 * @param   pfnCallback Pointer to the function to call back.
 *                      This will be called when fork() is about to
 *                      complete (the fork() result is established so to
 *                      speak). A zero rc argument indicates success,
 *                      a non zero rc argument indicates failure.
 * @param   pvArg       Argument to pass to pfnCallback as 3rd argument.
 * @param   enmContext  __LIBC_FORK_CTX_CHILD, __LIBC_FORK_CTX_PARENT, or
 *                      __LIBC_FORK_CTX_BOTH. May be ORed with
 *                      __LIBC_FORK_CTX_FLAGS_LAST to add the callback to
 *                      the end of the list so that it will be called after
 *                      all other registered callbacks. 
 *
 * @remark  Use with care, the memory used to remember these is taken from the
 *          fork buffer.
 */
int forkPrmCompletionCallback(__LIBC_PFORKHANDLE pForkHandle, __LIBC_PFNCOMPLETIONCALLBACK pfnCallback,
                              void *pvArg, __LIBC_FORKCTX enmContext)
{
    LIBCLOG_ENTER("pForkHandle=%p pfnCallback=%p pvArg=%p enmContext=%d\n", (void *)pForkHandle, (void *)pfnCallback, pvArg, enmContext);

    /*
     * Check if there is place in the buffer for it, if not flush it.
     */
    if (pForkHandle->cbBufLeft < sizeof(__LIBC_FORKCOMPLETIONCALLBACK))
    {
        int rc = forkPrmFlush(pForkHandle);
        if (rc < 0)
            LIBCLOG_RETURN_INT(rc);
    }

    int fLast = enmContext & __LIBC_FORK_CTX_FLAGS_LAST;
    enmContext &= __LIBC_FORK_CTX_MASK;

    /*
     * Add it to the completion callbacks.
     * These are eating of the buffer from the end of it.
     */
    pForkHandle->cbBuf      -= sizeof(__LIBC_FORKCOMPLETIONCALLBACK);
    pForkHandle->cbBufLeft  -= sizeof(__LIBC_FORKCOMPLETIONCALLBACK);
    pForkHandle->cCompletionCallbacks++;
    pForkHandle->papfnCompletionCallbacks--;
    if (fLast)
    {
        /* Add to the tail of the list to have it executed last */
        unsigned cCallbacks = pForkHandle->cCompletionCallbacks - 1;
        memmove(pForkHandle->papfnCompletionCallbacks, pForkHandle->papfnCompletionCallbacks + 1,
                sizeof(__LIBC_FORKCOMPLETIONCALLBACK) * cCallbacks);
        pForkHandle->papfnCompletionCallbacks[cCallbacks].pfnCallback  = pfnCallback;
        pForkHandle->papfnCompletionCallbacks[cCallbacks].enmContext   = enmContext;
        pForkHandle->papfnCompletionCallbacks[cCallbacks].pvArg        = pvArg;
    }
    else
    {
        /* Add to the head of the list to have it executed first */
        pForkHandle->papfnCompletionCallbacks->pfnCallback  = pfnCallback;
        pForkHandle->papfnCompletionCallbacks->enmContext   = enmContext;
        pForkHandle->papfnCompletionCallbacks->pvArg        = pvArg;
    }

    LIBCLOG_RETURN_INT(0);
}


/**
 * Get the module list.
 *
 * @returns Pointer to the head of the module list.
 * @returns NULL and errno on failure.
 * @remark  There must always be at least one module in that list
 */
__LIBC_PFORKMODULE  forkBthGetModules(void)
{
    __LIBC_PFORKMODULE  pModules;
    __LIBC_PSPMPROCESS  pProcessSelf = __libc_spmSelf();
    if (pProcessSelf)
    {
        pModules = pProcessSelf->pvModuleHead;
        __libc_spmRelease(pProcessSelf);
    }
    else
    {
        errno = ENOMEM;
        pModules = NULL;
    }
    return pModules;
}


/**
 * Process a module list calling the pfnAtFork() callback with a given operation.
 *
 * @returns 0 if all modules returns succesfully.
 * @returns negative error code (errno.h) as returned by failing module.
 * @returns positive error code (errno.h) as returned by the last warning module.
 * @param   pForkHandle     The fork handle.
 * @param   pModules        Head of the module list.
 * @param   enmOperation    The operation to perform on the modules.
 */
int forkBthProcessModules(__LIBC_PFORKHANDLE pForkHandle, __LIBC_PFORKMODULE pModules, __LIBC_FORKOP enmOperation)
{
    LIBCLOG_ENTER("pForkHandle=%p pModules=%p enmOperation=%d\n", (void *)pForkHandle, (void *)pModules, enmOperation);
    int                 rcRet = 0;
    __LIBC_PFORKMODULE  pModule;

    /*
     * Iterate the modules and let the module
     */
    for (pModule = pModules; pModule; pModule = pModule->pNext)
    {
        int rc = pModule->pfnAtFork(pModule, pForkHandle, enmOperation);
        if (rc)
        {
            LIBCLOG_MSG("Check parent returned %s rc=%d pModule=%p\n", rc > 0 ? "warning" : "error", rc, (void *)pModule);
            if (rc < 0)
                LIBCLOG_RETURN_INT(rc);
            if (!rcRet)
                rcRet = rc;
        }
    }
    LIBCLOG_RETURN_INT(rcRet);
}


/**
 * Give the buffer to the other context.
 *
 * @returns 0 on success.
 * @returns negative error code (errno.h) on failure.
 * @param   pForkHandle The fork handle.
 * @param   enmCtx      The context we're called in.
 */
int     forkBthBufferGive(__LIBC_PFORKHANDLE pForkHandle, __LIBC_FORKCTX enmCtx)
{
    LIBCLOG_ENTER("pForkHandle=%p enmCtx=%d\n", (void *)pForkHandle, enmCtx);
    HEV     hevWait;
    HEV     hevSignal;
    int     rc;
    ULONG   ul;

    /*
     * Figure out what to signal and wait on.
     */
    if (enmCtx == __LIBC_FORK_CTX_PARENT)
    {
        hevWait   = pForkHandle->hevParent;
        hevSignal = pForkHandle->hevChild;
    }
    else
    {
        hevWait   = pForkHandle->hevChild;
        hevSignal = pForkHandle->hevParent;
    }

    /*
     * Actually give the buffer.
     */
    rc = DosResetEventSem(hevWait, &ul);
    if (rc != NO_ERROR && rc != ERROR_ALREADY_RESET)
    {
        LIBC_ASSERTM_FAILED("DosResetEventSem(%#lx,) -> %d\n", hevWait, rc);
        rc = -__libc_native2errno(rc);
        LIBCLOG_RETURN_INT(rc);
    }
    rc = DosPostEventSem(hevSignal);    /* this should already be signaled, so we could skip this, but it's safer this way... */
    if (rc != NO_ERROR && rc != ERROR_ALREADY_POSTED)
    {
        LIBC_ASSERTM_FAILED("DosPostEventSem(%#lx) -> %d\n", hevSignal, rc);
        rc = -__libc_native2errno(rc);
        LIBCLOG_RETURN_INT(rc);
    }
    rc = DosReleaseMutexSem(pForkHandle->hmtx);
    if (rc != NO_ERROR)
    {
        LIBC_ASSERTM_FAILED("DosReleaseMutexSem(%#lx) -> %d\n", pForkHandle->hmtx, rc);
        rc = -__libc_native2errno(rc);
        LIBCLOG_RETURN_INT(rc);
    }
    LIBCLOG_RETURN_INT(0);
}


/**
 * Wait for the buffer while it's begin processed and perhaps refilled by the
 * other process.
 *
 * @returns 0 on success.
 * @returns negative error code (errno.h) on failure.
 * @param   pForkHandle The fork handle.
 * @param   enmCtx      The context we're called in.
 */
int forkBthBufferWait(__LIBC_PFORKHANDLE pForkHandle, __LIBC_FORKCTX enmCtx)
{
    LIBCLOG_ENTER("pForkHandle=%p enmCtx=%d\n", (void *)pForkHandle, enmCtx);

    HEV     hevWait;
    HEV     hevSignal;
    int     rc;

    /*
     * Figure out what to signal and wait on.
     */
    if (enmCtx == __LIBC_FORK_CTX_PARENT)
    {
        hevWait   = pForkHandle->hevParent;
        hevSignal = pForkHandle->hevChild;
    }
    else
    {
        hevWait   = pForkHandle->hevChild;
        hevSignal = pForkHandle->hevParent;
    }

    /*
     * Wait on the event sem and then on the mutex.
     */
    rc = DosWaitEventSem(hevWait, pForkHandle->cmsecTimeout);
    if (rc != NO_ERROR)
    {
        LIBC_ASSERTM_FAILED("DosWaitEventSem(%#lx, %lu) -> %d\n", hevWait, pForkHandle->cmsecTimeout, rc);
        rc = -__libc_native2errno(rc);
        LIBCLOG_RETURN_INT(rc);
    }
    rc = DosRequestMutexSem(pForkHandle->hmtx, pForkHandle->cmsecTimeout);
    if (rc != NO_ERROR)
    {
        LIBC_ASSERTM_FAILED("DosRequestMutexSem(%#lx, %lu) -> %d\n", pForkHandle->hmtx, pForkHandle->cmsecTimeout, rc);
        rc = -__libc_native2errno(rc);
        LIBCLOG_RETURN_INT(rc);
    }
    rc = DosPostEventSem(hevSignal);
    if (rc != NO_ERROR && rc != ERROR_ALREADY_POSTED)
    {
        LIBC_ASSERTM_FAILED("DosPostEventSem(%#lx) -> %d\n", hevSignal, rc);
        rc = -__libc_native2errno(rc);
        LIBCLOG_RETURN_INT(rc);
    }

    LIBCLOG_RETURN_INT(0);
}

/**
 * Process the fork buffer.
 *
 * @returns 0 on success. Buffer is reset to empty state.
 * @returns negative error code (errno.h) on failure.
 * @param   pForkHandle The fork handle.
 * @param   enmCtx      The context we're called in.
 * @param   pfNext      Where to store the next run indicator.
 *                      This doesn't apply to all calls and can thus be NULL.
 */
int forkBthBufferProcess(__LIBC_PFORKHANDLE pForkHandle, __LIBC_FORKCTX enmCtx, int *pfNext)
{
    LIBCLOG_ENTER("pForkHandle=%p enmCtx=%d\n", (void *)pForkHandle, enmCtx);

    /*
     * Iterate thru the buffer.
     */
    int                 rc = 0;
    __LIBC_PFORKPKGHDR  pHdr = pForkHandle->pBuf;
    pForkHandle->pBufCur = NULL;
    if (pfNext)
        *pfNext = 0;
    LIBCLOG_MSG("Processing %d bytes of buffer data\n", pForkHandle->cbBuf - pForkHandle->cbBufLeft);
    while (rc >= 0 && pHdr->enmType != __LIBC_FORK_HDR_TYPE_END)
    {
        LIBCLOG_MSG("at offset %#08x packet size %d (%#x) type %d\n",
                    (char *)pHdr - (char *)pForkHandle->pBuf, pHdr->cb, pHdr->cb, pHdr->enmType);
        LIBC_ASSERTM((char *)pHdr - (char *)pForkHandle->pBuf <= pForkHandle->cbBuf - pForkHandle->cbBufLeft,
                     "Buffer is missing an END packet!\n");
        LIBC_ASSERTM(!memcmp(pHdr->szMagic, __LIBC_FORK_HDR_MAGIC, sizeof(__LIBC_FORK_HDR_MAGIC)),
                     "Fork package corrupt! Magic mismatch!\n");
        switch (pHdr->enmType)
        {
            /*
             * Next run.
             * This must be the last package in the buffer, save the end one.
             */
            case __LIBC_FORK_HDR_TYPE_NEXT:
                LIBC_ASSERTM(pfNext, "Got NEXT package bug pfNext was NULL!\n");
                *pfNext = 1;
                break;

            /*
             * The fork() is aborted!
             */
            case __LIBC_FORK_HDR_TYPE_ABORT:
                LIBCLOG_MSG("ABORT err=%d\n", pHdr->u.Abort.err);
                rc = pHdr->u.Abort.err;
                if (enmCtx == __LIBC_FORK_CTX_CHILD)
                    forkChlFatalError(pForkHandle, rc, NULL);
                break;

            /*
             * Duplicate pages.
             */
            case __LIBC_FORK_HDR_TYPE_DUPLICATE:
                rc = forkBthBufferDuplicate(pForkHandle, pHdr);
                break;

            /*
             * Invoke function.
             */
            case __LIBC_FORK_HDR_TYPE_INVOKE:
                rc = pHdr->u.Invoke.pfn(pForkHandle, &pHdr->u.Invoke.achStart[0], pHdr->u.Invoke.cbArg);
                LIBC_ASSERTM(rc >= 0, "Function at address %p failed with rc=%d\n", (void *)pHdr->u.Invoke.pfn, rc);
                break;

            default:
                LIBC_ASSERTM_FAILED("shouldn't be here!\n");
                break;
        }

        /* next */
        pHdr = (__LIBC_PFORKPKGHDR)((char *)pHdr + pHdr->cb);
    }

    /*
     * Reset the buffer.
     */
    if (rc >= 0)
        forkBthBufferReset(pForkHandle);

    LIBCLOG_RETURN_INT(rc);
}


/**
 * Unpacks pages from forkPrmDuplicatePages() into the child context.
 *
 * @returns 0 on success.
 * @returns negative error code (errno.h) on failure.
 * @param   pForkHandle Fork handle.
 * @param   pHdr        The DuplicatePages fork packet header.
 */
int forkBthBufferDuplicate(__LIBC_PFORKHANDLE pForkHandle, __LIBC_PFORKPKGHDR pHdr)
{
    LIBCLOG_ENTER("pForkHandle=%p pHdr=%p:{.cb=%d,u.Duplicate.fFlags=%08x}\n", (void *)pForkHandle, (void *)pHdr, pHdr->cb, pHdr->u.Duplicate.fFlags);
    PFORKPGCHUNK    pChunk = (PFORKPGCHUNK)&pHdr->u.Duplicate.achStart[0];
    char           *pchEnd = (char *)pHdr + pHdr->cb;
    int             fDoSetMem = (pHdr->u.Duplicate.fFlags & __LIBC_FORK_FLAGS_PAGE_ATTR) != 0;
    ULONG           flFlagsAlloc = pHdr->u.Duplicate.fFlags & (PAG_READ | PAG_WRITE | PAG_EXECUTE | PAG_GUARD | PAG_COMMIT);
    int             rc = 0;

    /*
     * Process the packet.
     */
    while ((char *)pChunk < pchEnd)
    {
        LIBCLOG_MSG("Chunk %p: pv=%08x cb=%08x cbVirt=%08x fFlags=%08x\n",
                    (void *)pChunk, (uintptr_t)pChunk->pv, pChunk->cbVirt, pChunk->cb, pChunk->fFlags);

        /*
         * Set page attributes.
         */
        if (fDoSetMem)
        {
            ULONG flFlags = pChunk->fFlags & (PAG_READ | PAG_WRITE | PAG_EXECUTE | PAG_GUARD | PAG_COMMIT);
            if (flFlagsAlloc != flFlags)  /* (We thus ignore fFlags = 0 too in some cases, but who cares.) */
            {
                /** @todo write this properly to handle ALL cases!!! */
                rc = DosSetMem(pChunk->pv, pChunk->cbVirt, flFlags);
                if (rc && (flFlags & PAG_COMMIT)) /** @todo figure out the error code here! */
                    rc = DosSetMem(pChunk->pv, pChunk->cbVirt, flFlags & ~PAG_COMMIT);
                if (rc)
                {
                    forkBthDumpMemFlags(pChunk->pv);
                    LIBC_ASSERTM_FAILED("DosSetMem(%p, %08x, %08lx) failed with rc=%d\n", pChunk->pv, pChunk->cbVirt, flFlags, rc);
                    rc = -__libc_native2errno(rc);
                    break;
                }
            }
        }

        /*
         * Copy bits.
         */
        if (pChunk->cb)
            pfnForkBthCopyPages(pChunk->pv, &pChunk->achData[pChunk->offData], pChunk->cb);

        /* next */
        pChunk = (PFORKPGCHUNK)&pChunk->achData[pChunk->cb + pChunk->offData];
    }

    LIBCLOG_RETURN_INT(rc);
}


/**
 * Resets the fork buffer leaving it in an empty state but with space
 * reserved for the end package.
 *
 * @param   pForkHandle The fork handle.
 */
void forkBthBufferReset(__LIBC_PFORKHANDLE pForkHandle)
{
    LIBCLOG_ENTER("pForkHandle=%p\n", (void *)pForkHandle);
    /*
     * Reset the fork buffer.
     */
    pForkHandle->pBufCur   = pForkHandle->pBuf;
    pForkHandle->cbBufLeft = pForkHandle->cbBuf - offsetof(__LIBC_FORKPKGHDR, u.End.achStart);
    LIBCLOG_RETURN_VOID();
}


/**
 * Adds the end packet to the buffer and thereby
 * terminating the buffer.
 *
 * @param   pForkHandle The fork handle.
 */
void forkBthBufferEnd(__LIBC_PFORKHANDLE pForkHandle)
{
    LIBCLOG_ENTER("pForkHandle=%p cbBufLeft=%d\n", (void *)pForkHandle, pForkHandle->cbBufLeft);

    /*
     * Create end packet.
     */
    static const size_t cb = offsetof(__LIBC_FORKPKGHDR, u.End.achStart);
    __LIBC_PFORKPKGHDR  pHdr = pForkHandle->pBufCur;
    memcpy(pHdr->szMagic, __LIBC_FORK_HDR_MAGIC, sizeof(__LIBC_FORK_HDR_MAGIC));
    pHdr->cb                = cb;
    pHdr->enmType           = __LIBC_FORK_HDR_TYPE_END;
    /* Commit it. */
    pForkHandle->pBufCur    = (__LIBC_PFORKPKGHDR)((char *)pHdr + cb);
    pForkHandle->cbBufLeft  = 0;

    LIBCLOG_RETURN_VOID();
}


/**
 * Adds a NEXT packet to the buffer.
 *
 * @param   pForkHandle The fork handle.
 */
void forkBthBufferNext(__LIBC_PFORKHANDLE pForkHandle)
{
    LIBCLOG_ENTER("pForkHandle=%p cbBufLeft=%d\n", (void *)pForkHandle, pForkHandle->cbBufLeft);

    /*
     * Create end packet.
     */
    static const size_t cb = offsetof(__LIBC_FORKPKGHDR, u.Next.achStart);
    __LIBC_PFORKPKGHDR  pHdr = pForkHandle->pBufCur;
    LIBC_ASSERT(pForkHandle->cbBufLeft >= cb);
    memcpy(pHdr->szMagic, __LIBC_FORK_HDR_MAGIC, sizeof(__LIBC_FORK_HDR_MAGIC));
    pHdr->cb                = cb;
    pHdr->enmType           = __LIBC_FORK_HDR_TYPE_NEXT;
    /* Commit it. */
    pForkHandle->cbBufLeft -= cb;
    pForkHandle->pBufCur    = (__LIBC_PFORKPKGHDR)((char *)pHdr + cb);

    LIBCLOG_RETURN_VOID();
}


/**
 * Adds an ABORT packet to the buffer.
 *
 * @param   pForkHandle The fork handle.
 * @param   rc          The native errno of the fork() operation.
 */
void forkBthBufferAbort(__LIBC_PFORKHANDLE pForkHandle, int rc)
{
    LIBCLOG_ENTER("pForkHandle=%p rc=%d\n", (void *)pForkHandle, rc);

    /*
     * Create end packet.
     */
    static const size_t cb = offsetof(__LIBC_FORKPKGHDR, u.Abort.achStart);
    __LIBC_PFORKPKGHDR  pHdr = pForkHandle->pBufCur;
    LIBC_ASSERT(pForkHandle->cbBufLeft >= cb);
    memcpy(pHdr->szMagic, __LIBC_FORK_HDR_MAGIC, sizeof(__LIBC_FORK_HDR_MAGIC));
    pHdr->cb                = cb;
    pHdr->enmType           = __LIBC_FORK_HDR_TYPE_ABORT;
    pHdr->u.Abort.err       = rc;
    /* Commit it. */
    pForkHandle->cbBufLeft -= cb;
    pForkHandle->pBufCur    = (__LIBC_PFORKPKGHDR)((char *)pHdr + cb);

    LIBCLOG_RETURN_VOID();
}


/**
 * Checks if the buffer can contain a packet of size cb,
 * if not the buffer is flushed.
 *
 * @returns 0 on success.
 * @returns negative error code (errno.h) on failure.
 * @param   ForkHandle  The fork handle.
 * @param   cb          The number of bytes of required buffer space.
 */
int forkBthBufferSpace(__LIBC_PFORKHANDLE pForkHandle, size_t cb)
{
    LIBCLOG_ENTER("pForkHandle=%p cb=%d (cbBufLeft=%d)\n", (void *)pForkHandle, cb, pForkHandle->cbBufLeft);
    if (pForkHandle->cbBufLeft < cb)
    {
        int rc = forkPrmFlush(pForkHandle);
        if (rc < 0)
        {
            LIBC_ASSERTM_FAILED("forkPrmFlush failed rc=%d\n", rc);
            LIBCLOG_RETURN_INT(rc);
        }
        if (pForkHandle->cbBufLeft < cb)
        {
            LIBC_ASSERT_FAILED();
            LIBCLOG_RETURN_INT(-E2BIG);
        }
    }
    LIBCLOG_RETURN_INT(0);
}


/**
 * Gives the buffer to the other context and wait+process until a next
 * package is encountered.
 *
 * @returns 0 on success. Buffer owner.
 * @returns negative error code (errno.h) on failure.
 * @param   pForkHandle The fork handle.
 * @param   enmCtx      The calling context.
 */
int     forkBthBufferGiveWaitNext(__LIBC_PFORKHANDLE pForkHandle, __LIBC_FORKCTX enmCtx)
{
    LIBCLOG_ENTER("pForkHandle=%p enmCtx=%d\n", (void *)pForkHandle, enmCtx);

    for (;;)
    {
        int rc;
        int fNext = 0;

        /*
         * Give the buffer to the other process.
         */
        rc = forkBthBufferGive(pForkHandle, enmCtx);
        if (rc < 0)
            LIBCLOG_RETURN_INT(rc);

        /*
         * Wait for the buffer.
         */
        rc = forkBthBufferWait(pForkHandle, enmCtx);
        if (rc < 0)
            LIBCLOG_RETURN_INT(rc);

        /*
         * Process the buffer.
         */
        rc = forkBthBufferProcess(pForkHandle, enmCtx, &fNext);
        if (rc < 0)
            LIBCLOG_RETURN_INT(rc);
        if (fNext)
            LIBCLOG_RETURN_INT(0);

        /*
         * Insert end packet before looping.
         */
        forkBthBufferEnd(pForkHandle);
    }
}


/**
 * Copy page memory, detect the right worker function and calls it.
 *
 * @param   pvDst   Target address.
 * @param   pvSrc   Source address.
 * @param   cb      Number of bytes to copy.
 */
void forkBthCopyPagesDetect(void *pvDst, const void *pvSrc, size_t cb)
{
    /*
     * Decide which function to use.
     */
    pfnForkBthCopyPages = forkBthCopyPagesPlain; /* default, always works. */
    if (HasCpuId())
    {
        unsigned uEdx = CpuIdEDX(1);
        if (uEdx & (23/*MMX*/ << 1))
        {
            pfnForkBthCopyPages = forkBthCopyPagesMMX;
#if 0  /* missing hardware to test this. */
            /* hope this is right */
            if (    (uEdx & (1 << 24)) /*FXSR*/
                &&  (uEdx & (1 << 25)) /*SSE*/
                &&  (uEdx & (1 << 26)) /*SSE2*/)
                pfnForkBthCopyPages = forkBthCopyPagesSSE2;
#endif
#if 0 /* too slow for some peculiar reason... */
            else
            {
                if (    uEdx & (23/*MMX*/ << 1)
                    &&  uEdx & (25/*SSE*/ << 1))
                    pfnForkBthCopyPages = forkBthCopyPagesMMXNonTemporal;
            }
#endif
        }
    }
    pfnForkBthCopyPages(pvDst, pvSrc, cb);
}


__asm__(".align 4\n\t");
/**
 * Copy page memory, plain stupid rep movsl.
 *
 * @param   pvDst   Target address.
 * @param   pvSrc   Source address.
 * @param   cb      Number of bytes to copy.
 */
static void forkBthCopyPagesPlain(void *pvDst, const void *pvSrc, size_t cb)
{
    LIBCLOG_ENTER("pvDst=%p pvSrc=%p cb=0x%08x\n", pvDst, pvSrc, cb);
    register unsigned long int r0,r1,r2;
    __asm__ __volatile__
      ("cld\n\t"
       "rep; movsl"
       : "=&c" (r0), "=&D" (r1), "=&S" (r2)
       : "0" (cb / 4), "1" (pvDst), "2" (pvSrc)
       : "memory");
    LIBCLOG_RETURN_VOID();
}


__asm__(".align 4\n\t");
/**
 * Copy page memory, MMX.
 *
 * @param   pvDst   Target address.
 * @param   pvSrc   Source address.
 * @param   cb      Number of bytes to copy.
 */
static void forkBthCopyPagesMMX(void *pvDst, const void *pvSrc, size_t cb)
{
    LIBCLOG_ENTER("pvDst=%p pvSrc=%p cb=0x%08x\n", pvDst, pvSrc, cb);
    register unsigned long int r0,r1,r2;
    __asm__ __volatile__
      ("\
1:                                                              \n\
        movq   (%0), %%mm1                                      \n\
        movq  8(%0), %%mm2                                      \n\
        movq 16(%0), %%mm3                                      \n\
        movq 24(%0), %%mm4                                      \n\
        movq %%mm1,   (%1)                                      \n\
        movq %%mm2,  8(%1)                                      \n\
        movq %%mm3, 16(%1)                                      \n\
        movq %%mm4, 24(%1)                                      \n\
        movq 32(%0), %%mm5                                      \n\
        movq 40(%0), %%mm6                                      \n\
        movq 48(%0), %%mm7                                      \n\
        movq 56(%0), %%mm0                                      \n\
        addl $64, %0                                            \n\
        movq %%mm5, 32(%1)                                      \n\
        movq %%mm6, 40(%1)                                      \n\
        movq %%mm7, 48(%1)                                      \n\
        movq %%mm0, 56(%1)                                      \n\
        addl $64, %1                                            \n\
                                                                \n\
        decl %2                                                 \n\
        jnz 1b                                                  \n\
                                                                \n\
        emms                                                    \n\
        "
       : "=&r" (r0), "=&r" (r1), "=&r" (r2)
       : "0" (pvSrc), "1" (pvDst), "2" (cb/64)
       : "memory");
    LIBCLOG_RETURN_VOID();
}


#if 0 /* too slow */
__asm__(".align 4\n\t");
/**
 * Copy page memory, MMX with non-temporal stores.
 *
 * @param   pvDst   Target address.
 * @param   pvSrc   Source address.
 * @param   cb      Number of bytes to copy.
 * @remark requires the Pentium III MMX extensions (appears to mean SSE).
 */
static void forkBthCopyPagesMMXNonTemporal(void *pvDst, const void *pvSrc, size_t cb)
{
    LIBCLOG_ENTER("pvDst=%p pvSrc=%p cb=0x%08x\n", pvDst, pvSrc, cb);
    register unsigned long int r0,r1,r2;
    __asm__ __volatile__
      ("\n\
        movq   (%0), %%mm1                                          \n\
        movq  8(%0), %%mm2                                          \n\
        prefetchnta 320(%0)                                         \n\
        movq 16(%0), %%mm3                                          \n\
        movq 24(%0), %%mm4                                          \n\
        movntq %%mm1,   (%1)                                        \n\
        movntq %%mm2,  8(%1)                                        \n\
        movntq %%mm3, 16(%1)                                        \n\
        movntq %%mm4, 24(%1)                                        \n\
        movq 32(%0), %%mm5                                          \n\
        movq 40(%0), %%mm6                                          \n\
        movq 48(%0), %%mm7                                          \n\
        movq 56(%0), %%mm0                                          \n\
        addl $64, %0                                                \n\
        movntq %%mm5, 32(%1)                                        \n\
        movntq %%mm6, 40(%1)                                        \n\
        movntq %%mm7, 48(%1)                                        \n\
        movntq %%mm0, 56(%1)                                        \n\
        addl $64, %1                                                \n\
        decl %2                                                     \n\
        1:                                                          \n\
        movq   (%0), %%mm1                                          \n\
        movq  8(%0), %%mm2                                          \n\
        movq 16(%0), %%mm3                                          \n\
        movq 24(%0), %%mm4                                          \n\
        movntq %%mm1,   (%1)                                        \n\
        movntq %%mm2,  8(%1)                                        \n\
        movntq %%mm3, 16(%1)                                        \n\
        movntq %%mm4, 24(%1)                                        \n\
        movq 32(%0), %%mm5                                          \n\
        movq 40(%0), %%mm6                                          \n\
        movq 48(%0), %%mm7                                          \n\
        movq 56(%0), %%mm0                                          \n\
        addl $64, %0                                                \n\
        prefetchnta 320(%0)                                         \n\
        movntq %%mm5, 32(%1)                                        \n\
        movntq %%mm6, 40(%1)                                        \n\
        movntq %%mm7, 48(%1)                                        \n\
        movntq %%mm0, 56(%1)                                        \n\
        addl $64, %1                                                \n\
                                                                    \n\
        decl %2                                                     \n\
        jnz 1b                                                      \n\
        emms                                                        \n\
        "
        : "=&r" (r0), "=&r" (r1), "=&r" (r2)
        : "0" (pvSrc), "1" (pvDst), "2" (cb/64)
        : "memory");
    LIBCLOG_RETURN_VOID();
}
#endif

#if 0 /* untested */
/**
 * Copy page memory, SSE.
 *
 * @param   pvDst   Target address.
 * @param   pvSrc   Source address.
 * @param   cb      Number of bytes to copy.
 */
static void forkBthCopyPagesSSE2(void *pvDst, const void *pvSrc, size_t cb)
{
    LIBCLOG_ENTER("pvDst=%p pvSrc=%p cb=0x%08x\n", pvDst, pvSrc, cb);
    if ((uintptr_t)pvDst & 15)  asm("int3");
    if ((uintptr_t)pvSrc & 15)  asm("int3");
    if (cb & 127)  asm("int3");

    register unsigned long int r0,r1,r2;
    __asm__ __volatile__
      ("prefetchnta    (%0)                             \n\
        prefetchnta  64(%0)                             \n\
        prefetchnta 128(%0)                             \n\
        prefetchnta 192(%0)                             \n\
        prefetchnta 256(%0)                             \n\
        jmp 1f                                          \n\
                                                        \n\
        .align 4                                        \n\
        1:                                              \n\
        prefetchnta 320(%0)                             \n\
        movdqa    (%0), %%xmm0                          \n\
        movdqa  16(%0), %%xmm1                          \n\
        movntdq %%xmm0,   (%1)                          \n\
        movntdq %%xmm1, 16(%1)                          \n\
        movdqa  32(%0), %%xmm2                          \n\
        movdqa  48(%0), %%xmm3                          \n\
        movntdq %%xmm2, 32(%1)                          \n\
        movntdq %%xmm3, 48(%1)                          \n\
        movdqa  64(%0), %%xmm4                          \n\
        movdqa  80(%0), %%xmm5                          \n\
        movntdq %%xmm4, 64(%1)                          \n\
        movntdq %%xmm5, 80(%1)                          \n\
        movdqa  96(%0), %%xmm6                          \n\
        movdqa  112(%0), %%xmm7                         \n\
        movntdq %%xmm6, 96(%1)                          \n\
        movntdq %%xmm7, 112(%1)                         \n\
                                                        \n\
        addl $128, %0                                   \n\
        addl $128, %1                                   \n\
        decl %2                                         \n\
        jnz 1b                                          \n\
                                                        \n\
        sfence                                          \n\
       "
        : "=&r" (r0), "=&r" (r1), "=&r" (r2)
        : "0" (pvDst), "1" (pvSrc), "2" (cb / 128)
        : "memory");
    LIBCLOG_RETURN_VOID();
}
#endif


/**
 * Sorts the array of pointers to callbacks.
 *
 * @param   papCallbacks    Pointer to array of pointers to callbacks.
 *                          This is a a.out set vector.
 */
void forkBthCallbacksSort(__LIBC_PFORKCALLBACK *papCallbacks)
{
    LIBCLOG_ENTER("papCallbacks=%p\n", (void *)papCallbacks);
    __LIBC_PFORKCALLBACK   *papFirst;
    size_t                  cCallbacks;
    __LIBC_PFORKCALLBACK   *ppCur;

    /*
     * Figure out actual start and lenght.
     */
    if (*papCallbacks == (__LIBC_PFORKCALLBACK)-2)          /* emxomf */
    {
        ppCur = papFirst = papCallbacks + 1;
        while (*ppCur)
            ppCur++;
        cCallbacks = ppCur - papFirst;
    }
    else
    {
        if (*papCallbacks == (__LIBC_PFORKCALLBACK)-1)
            papCallbacks--;                                 /* Fix GNU ld bug */
        if (papCallbacks[1] != (__LIBC_PFORKCALLBACK)-1)    /* First element - see crt0/dll0. */
            cCallbacks = 0;
        else
        {
            cCallbacks = (uintptr_t)*papCallbacks - 1;      /* Get size of vector */
            papFirst = &papCallbacks[2];
        }
    }

    /*
     * Do a qsort on the array.
     */
    if (cCallbacks)
        qsort(papFirst, cCallbacks, sizeof(*papFirst), forkBthCallbacksCompare);
    LIBCLOG_RETURN_VOID();
}


/**
 * qsort callback for sorting a callback vector.
 * (The callbacks are sorted descending on uPriority.)
 *
 * @returns 0 if pv1 has a priority equal to pv2.
 * @returns negative if pv1 has a priority lower than pv2.
 * @returns positive if pv1 has a priority higher than pv2.
 * @param   pv1     Pointer to first element.
 * @param   pv2     Pointer to second element.
 */
int forkBthCallbacksCompare(const void *pv1, const void *pv2)
{
    unsigned u1 = (*(__LIBC_PFORKCALLBACK *)(void **)pv1)->uPriority;
    unsigned u2 = (*(__LIBC_PFORKCALLBACK *)(void **)pv2)->uPriority;
    if (u1 > u2)
        return -1;
    if (u1 < u2)
        return 1;
    return 0;
}


/**
 * Sorts the array of pointers to callbacks.
 *
 * @param   papCallbacks    Pointer to array of pointers to callbacks.
 *                          This is a a.out set vector.
 */
int forkBthCallbacksCall(__LIBC_PFORKCALLBACK *papCallbacks, __LIBC_PFORKHANDLE pForkHandle, __LIBC_FORKOP enmOperation)
{
    LIBCLOG_ENTER("papCallback=%p pForkHandle=%p enmOperation=%d\n", (void *)papCallbacks, (void *)pForkHandle, enmOperation);
    int rcRet = 0;

    /*
     * Walk the vector and call the callbacks.
     *
     * The two branches here are due to the different ways emxomf and ld
     * handle set vectors. And really we should fix make emxomf spitt out
     * something which is more similar to ld one day!
     */
    if (*papCallbacks == (__LIBC_PFORKCALLBACK)-2)          /* emxomf */
    {
        __LIBC_PFORKCALLBACK   *ppCur = papCallbacks + 1;
        while (*ppCur)
        {
            int rc = (*ppCur)->pfnCallback(pForkHandle, enmOperation);
            if (rc < 0)
            {
                LIBC_ASSERTM_FAILED("Callback %p in %p with priority %#x failed with rc=%d\n",
                                    (void *)(*ppCur)->pfnCallback, (void *)ppCur, (*ppCur)->uPriority, rc);
                LIBCLOG_RETURN_INT(rc);
            }
            if (rc > 0)
            {
                LIBCLOG_MSG("Callback %p in %p with priority %#x returned warning rc=%d\n",
                            (void *)(*ppCur)->pfnCallback, (void *)ppCur, (*ppCur)->uPriority, rc);
                rcRet = rc;
            }

            /* next */
            ppCur++;
        }
    }
    else
    {
        if (*papCallbacks == (__LIBC_PFORKCALLBACK)-1)
            papCallbacks--;                                 /* Fix GNU ld bug */
        if (papCallbacks[1] == (__LIBC_PFORKCALLBACK)-1)    /* First element - see crt0/dll0. */
        {
            __LIBC_PFORKCALLBACK   *ppCur = &papCallbacks[2];
            size_t                  cCallbacks = (uintptr_t)*papCallbacks; /* Get size of vector */
            while (cCallbacks-- > 1)
            {
                int rc = (*ppCur)->pfnCallback(pForkHandle, enmOperation);
                if (rc < 0)
                {
                    LIBC_ASSERTM_FAILED("Callback %p in %p with priority %#x failed with rc=%d\n",
                                        (void *)(*ppCur)->pfnCallback, (void *)ppCur, (*ppCur)->uPriority, rc);
                    LIBCLOG_RETURN_INT(rc);
                }
                if (rc > 0)
                {
                    LIBCLOG_MSG("Callback %p in %p with priority %#x returned warning rc=%d\n",
                                (void *)(*ppCur)->pfnCallback, (void *)ppCur, (*ppCur)->uPriority, rc);
                    rcRet = rc;
                }

                /* next */
                ppCur++;
            }
        }
    }

    LIBCLOG_RETURN_INT(rcRet);
}

/**
 * Dumps the flags of a memory region.
 * @param   pv      Address in that region.
 */
static void forkBthDumpMemFlags(void *pvIn)
{
#if 0
    LIBCLOG_MSG2("address  size     flags (forkBthDumpMemFlags)\n");
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
#endif
}


/**
 * Aborts the fork operation from the child side.
 *
 * @param   pForkHandle     Pointer to fork handle.
 * @param   rc              The return code (negative) which caused the failure.
 * @param   pvCtx           Context if available.
 */
void                 forkChlFatalError(__LIBC_PFORKHANDLE pForkHandle, int rc, void *pvCtx)
{
    LIBCLOG_ENTER("pForkHandle=%p rc=%d pvCtx=%p\n", (void *)pForkHandle, rc, pvCtx);
    PID                 pid;
    TID                 tid;
    ULONG               cNesting;
    PPIB                pPib;
    PTIB                pTib;
    int                 rc2;
/*__asm__ __volatile__("int3\n\t");*/

    /*
     * Completion callbacks.
     */
    forkChlCompletionChild(pForkHandle, rc);

    /*
     * Who owns the buffer?
     */
    DosGetInfoBlocks(&pTib, &pPib);
    rc2 = DosQueryMutexSem(pForkHandle->hmtx, &pid, &tid, &cNesting);
    /* if we don't, let's try grab it quickly. */
    if (!rc2 && (pid != pPib->pib_ulpid || tid != 1))
    {
        DosRequestMutexSem(pForkHandle->hmtx, 50);
        rc2 = DosQueryMutexSem(pForkHandle->hmtx, &pid, &tid, &cNesting);
    }
    if (!rc2 && pid == pPib->pib_ulpid && tid == 1)
    {
        /*
         * Empty the fork buffer, add abort and end, give buffer to parent.
         */
        LIBCLOG_MSG("Making abort packet with err=%d.\n", rc);
        forkBthBufferReset(pForkHandle);
        forkBthBufferAbort(pForkHandle, rc);
        forkBthBufferEnd(pForkHandle);
        forkBthBufferGive(pForkHandle, __LIBC_FORK_CTX_CHILD);
        DosSleep(1);
        LIBCLOG_MSG("Abort packet sent.\n");
    }
    else
    {
        /* mmm... eer.. yea... I think we'll just quit for now. */
        LIBCLOG_MSG("Cannot send abort packet, not buffer owner.\n");
        /** @todo think out a safe way of flagging this to prevent having to timeout! */
    }

    /*
     * Free the fork handle.
     */
    pForkHandle->pidChild = -1;
    forkBthCloseHandle(pForkHandle, __LIBC_FORK_CTX_CHILD);

    /*
     * Exit process.
     */
    __libc_Back_panic(__LIBC_PANIC_NO_SPM_TERM, pvCtx, "LIBC fork: Child aborting fork()! rc=%x\n", rc);
}


/**
 * Exception handler for the parent process.
 *
 * @returns XCPT_CONTINUE_SEARCH or XCPT_CONTINUE_EXECUTION.
 * @param   pXcptRepRec     Report record.
 * @param   pXcptRegRec     Registration record.
 * @param   pCtx            Context record.
 * @param   pvWhatEver      Not quite sure what this is...
 */
static ULONG _System forkParExceptionHandler(PEXCEPTIONREPORTRECORD       pXcptRepRec,
                                             PEXCEPTIONREGISTRATIONRECORD pXcptRegRec,
                                             PCONTEXTRECORD               pCtx,
                                             PVOID                        pvWhatEver)
{
    __asm__ ("cld");                    /* usual paranoia.  */

    if (pXcptRepRec->fHandlerFlags & (EH_UNWINDING | EH_EXIT_UNWIND))
        return XCPT_CONTINUE_SEARCH;
    LIBCLOG_MSG2("forkParExceptionHandler: ExceptionNum=%#lx ExceptionAddress=%p eip=%#lx ExceptionInfo={%#08lx,%#08lx,%#08lx,%#08lx} fHandlerFlags=%#lx\n",
                 pXcptRepRec->ExceptionNum, pXcptRepRec->ExceptionAddress, pCtx->ctx_RegEip,
                 pXcptRepRec->ExceptionInfo[0], pXcptRepRec->ExceptionInfo[1],
                 pXcptRepRec->ExceptionInfo[2], pXcptRepRec->ExceptionInfo[3],
                 pXcptRepRec->fHandlerFlags);

    switch (pXcptRepRec->ExceptionNum)
    {
        /*
         * Abort the fork operation and pass the exception on.
         */
        case XCPT_BREAKPOINT:
        case XCPT_ACCESS_VIOLATION:
            if (pXcptRepRec->fHandlerFlags & EH_NESTED_CALL)
                return XCPT_CONTINUE_SEARCH;
        case XCPT_SIGNAL:
        case XCPT_DATATYPE_MISALIGNMENT:
        case XCPT_INTEGER_OVERFLOW:
        case XCPT_INTEGER_DIVIDE_BY_ZERO:
        case XCPT_FLOAT_DIVIDE_BY_ZERO:
        case XCPT_FLOAT_OVERFLOW:
        case XCPT_FLOAT_UNDERFLOW:
        case XCPT_FLOAT_DENORMAL_OPERAND:
        case XCPT_FLOAT_INEXACT_RESULT:
        case XCPT_FLOAT_INVALID_OPERATION:
        case XCPT_FLOAT_STACK_CHECK:
        case XCPT_ARRAY_BOUNDS_EXCEEDED:
        case XCPT_ILLEGAL_INSTRUCTION:
        case XCPT_INVALID_LOCK_SEQUENCE:
        case XCPT_PRIVILEGED_INSTRUCTION:
        case XCPT_SINGLE_STEP:
        case XCPT_ASYNC_PROCESS_TERMINATE:
        case XCPT_PROCESS_TERMINATE:
        {
            __LIBC_PFORKHANDLE pForkHandle = g_pForkHandle;
            g_pForkHandle = NULL;
            if (pForkHandle)
            {
                PPIB    pPib;
                PTIB    pTib;
                PID     pid;
                TID     tid;
                ULONG   cNesting;
                int     rc;

                /*
                 * Perform parent completion.
                 */
                DosEnterMustComplete(&cNesting); /* no signals until we're safely backed out, please. */
                ((__LIBC_PFORKXCPTREGREC)pXcptRegRec)->fDoneCompletion = 1;
                forkParCompletionParent(pForkHandle, -EINTR);
                DosExitMustComplete(&cNesting);

                /*
                 * Try send an abort package if we can.
                 */
                DosGetInfoBlocks(&pTib, &pPib);
                rc = DosQueryMutexSem(pForkHandle->hmtx, &pid, &tid, &cNesting);
                /* if we don't, let's try grab it quickly. */
                if (!rc && (pid != pPib->pib_ulpid || tid != pTib->tib_ptib2->tib2_ultid))
                {
                    DosRequestMutexSem(pForkHandle->hmtx, 50);
                    rc = DosQueryMutexSem(pForkHandle->hmtx, &pid, &tid, &cNesting);
                }
                if (!rc && pid == pPib->pib_ulpid && tid == pTib->tib_ptib2->tib2_ultid)
                {
                    /*
                     * Empty the fork buffer, add abort and end, give buffer to parent.
                     */
                    LIBCLOG_MSG2("Making abort packet with err=%d.\n", rc);
                    forkBthBufferReset(pForkHandle);
                    forkBthBufferAbort(pForkHandle, rc);
                    forkBthBufferEnd(pForkHandle);
                    forkBthBufferGive(pForkHandle, __LIBC_FORK_CTX_PARENT);
                    LIBCLOG_MSG2("Abort packet sent.\n");
                    DosWaitEventSem(pForkHandle->hevParent, 500);
                }
                else
                {
                    LIBCLOG_MSG2("Cannot send abort packet, not buffer owner.\n");
                }

                /*
                 * Take evasive action.
                 */
                DosPostEventSem(pForkHandle->hevChild);
                DosPostEventSem(pForkHandle->hevParent);
                DosReleaseMutexSem(pForkHandle->hmtx);
                pid = pForkHandle->pidChild;
                pForkHandle->pidChild = -1;
                if ((pid_t)pid > 0)
                {
                    RESULTCODES resc;
                    PID         pibReaped;
                    DosKillProcess(DKP_PROCESS, pForkHandle->pidChild);
                    DosSleep(0);
                    DosWaitChild(DCWA_PROCESS,DCWW_NOWAIT, &resc, &pibReaped, pid);
                }
            }
            break;
        }
    }

    return XCPT_CONTINUE_SEARCH;
}


/**
 * Exception handler for the child process.
 *
 * @returns XCPT_CONTINUE_SEARCH or XCPT_CONTINUE_EXECUTION.
 * @param   pXcptRepRec     Report record.
 * @param   pXcptRegRec     Registration record.
 * @param   pCtx            Context record.
 * @param   pvWhatEver      Not quite sure what this is...
 */
static ULONG _System forkChlExceptionHandler(PEXCEPTIONREPORTRECORD       pXcptRepRec,
                                             PEXCEPTIONREGISTRATIONRECORD pXcptRegRec,
                                             PCONTEXTRECORD               pCtx,
                                             PVOID                        pvWhatEver)
{
    PPIB    pPib;
    PTIB    pTib;
    __asm__ ("cld");                    /* usual paranoia.  */

    if (pXcptRepRec->fHandlerFlags & (EH_UNWINDING | EH_EXIT_UNWIND))
        return XCPT_CONTINUE_SEARCH;

    DosGetInfoBlocks(&pTib, &pPib);
    pTib->tib_pexchain = END_OF_CHAIN;
    LIBCLOG_MSG2("forkChlExceptionHandler: ExceptionNum=%#lx ExceptionAddress=%p eip=%#lx ExceptionInfo={%#08lx,%#08lx,%#08lx,%#08lx} fHandlerFlags=%#lx\n",
                 pXcptRepRec->ExceptionNum, pXcptRepRec->ExceptionAddress, pCtx->ctx_RegEip,
                 pXcptRepRec->ExceptionInfo[0], pXcptRepRec->ExceptionInfo[1],
                 pXcptRepRec->ExceptionInfo[2], pXcptRepRec->ExceptionInfo[3],
                 pXcptRepRec->fHandlerFlags);

    switch (pXcptRepRec->ExceptionNum)
    {
        /*
         * Abort the fork operation and pass the exception on.
         */
        case XCPT_SIGNAL:
        case XCPT_ACCESS_VIOLATION:
        case XCPT_DATATYPE_MISALIGNMENT:
        case XCPT_INTEGER_OVERFLOW:
        case XCPT_INTEGER_DIVIDE_BY_ZERO:
        case XCPT_FLOAT_DIVIDE_BY_ZERO:
        case XCPT_FLOAT_OVERFLOW:
        case XCPT_FLOAT_UNDERFLOW:
        case XCPT_FLOAT_DENORMAL_OPERAND:
        case XCPT_FLOAT_INEXACT_RESULT:
        case XCPT_FLOAT_INVALID_OPERATION:
        case XCPT_FLOAT_STACK_CHECK:
        case XCPT_ARRAY_BOUNDS_EXCEEDED:
        case XCPT_ILLEGAL_INSTRUCTION:
        case XCPT_INVALID_LOCK_SEQUENCE:
        case XCPT_PRIVILEGED_INSTRUCTION:
        case XCPT_SINGLE_STEP:
        case XCPT_BREAKPOINT:
        case XCPT_ASYNC_PROCESS_TERMINATE:
        case XCPT_PROCESS_TERMINATE:
        {
            __LIBC_PFORKHANDLE pForkHandle = g_pForkHandle;
            g_pForkHandle = NULL;
            if (pForkHandle)
            {
                DosPostEventSem(pForkHandle->hevParent);
                forkChlFatalError(pForkHandle, -EINTR, pCtx);
            }
            break;
        }
    }

    return XCPT_CONTINUE_SEARCH;
}


/**
 * Exception handler for the module registration/deregistration.
 *
 * @returns XCPT_CONTINUE_SEARCH or XCPT_CONTINUE_EXECUTION.
 * @param   pXcptRepRec     Report record.
 * @param   pXcptRegRec     Registration record.
 * @param   pCtx            Context record.
 * @param   pvWhatEver      Not quite sure what this is...
 */
static ULONG _System forkBthExceptionHandlerRegDereg(PEXCEPTIONREPORTRECORD       pXcptRepRec,
                                                     PEXCEPTIONREGISTRATIONRECORD pXcptRegRec,
                                                     PCONTEXTRECORD               pCtx,
                                                     PVOID                        pvWhatEver)
{
    __asm__ ("cld");                    /* usual paranoia.  */

    if (pXcptRepRec->fHandlerFlags & (EH_UNWINDING | EH_EXIT_UNWIND))
        return XCPT_CONTINUE_SEARCH;

    LIBCLOG_MSG2("forkBthExceptionHandlerRegDereg: ExceptionNum=%#lx ExceptionAddress=%p eip=%#lx ExceptionInfo={%#08lx,%#08lx,%#08lx,%#08lx} fHandlerFlags=%#lx\n",
                 pXcptRepRec->ExceptionNum, pXcptRepRec->ExceptionAddress, pCtx->ctx_RegEip,
                 pXcptRepRec->ExceptionInfo[0], pXcptRepRec->ExceptionInfo[1],
                 pXcptRepRec->ExceptionInfo[2], pXcptRepRec->ExceptionInfo[3],
                 pXcptRepRec->fHandlerFlags);

    switch (pXcptRepRec->ExceptionNum)
    {
        /*
         * It's ok to get signaled, terminated or similar.
         */
        case XCPT_SIGNAL:
        case XCPT_ASYNC_PROCESS_TERMINATE:
        case XCPT_PROCESS_TERMINATE:
        default:
            return XCPT_CONTINUE_SEARCH;

        /*
         * Most otherthings should be considered fatal and
         * cause module initialization to fail.
         */
        case XCPT_ACCESS_VIOLATION:
        case XCPT_DATATYPE_MISALIGNMENT:
        case XCPT_INTEGER_OVERFLOW:
        case XCPT_INTEGER_DIVIDE_BY_ZERO:
        case XCPT_FLOAT_DIVIDE_BY_ZERO:
        case XCPT_FLOAT_OVERFLOW:
        case XCPT_FLOAT_UNDERFLOW:
        case XCPT_FLOAT_DENORMAL_OPERAND:
        case XCPT_FLOAT_INEXACT_RESULT:
        case XCPT_FLOAT_INVALID_OPERATION:
        case XCPT_FLOAT_STACK_CHECK:
        case XCPT_ARRAY_BOUNDS_EXCEEDED:
        case XCPT_ILLEGAL_INSTRUCTION:
        case XCPT_INVALID_LOCK_SEQUENCE:
        case XCPT_PRIVILEGED_INSTRUCTION:
        case XCPT_SINGLE_STEP:
        case XCPT_BREAKPOINT:
        {
            __LIBC_PFORKXCPTREGREC2 pRegRec = (__LIBC_PFORKXCPTREGREC2)pXcptRegRec;
            PPIB                    pPib;
            PTIB                    pTib;

            DosGetInfoBlocks(&pTib, &pPib);
            pTib->tib_pexchain = pRegRec->Core.prev_structure;

#if 0 /* can't do this in libc06x because of compatability issues */
            longjmp(pRegRec->JmpBuf, -1);
#endif
            return XCPT_CONTINUE_SEARCH;
        }
    }

}

