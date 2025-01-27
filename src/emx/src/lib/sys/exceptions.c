/* $Id: exceptions.c 1630 2004-11-14 11:28:29Z bird $ */
/** @file
 *
 * LIBC SYS Backend - Exceptions / Signals.
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
#include <os2emx.h>

#include <signal.h>
#include <errno.h>
#include <386/builtin.h>
#include <InnoTekLIBC/thread.h>
#include <InnoTekLIBC/backend.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_SIGNAL
#include <InnoTekLIBC/logstrict.h>
#include "backend.h"
#include "b_signal.h"


/*******************************************************************************
*   External Functions                                                         *
*******************************************************************************/
/* from kLib/kHeapDbg.h */
typedef enum
{
    enmRead, enmWrite, enmUnknown
} ENMACCESS;
extern BOOL _Optlink kHeapDbgException(void *    pvAccess,
                                       ENMACCESS enmAccess,
                                       void *    pvIP,
                                       void *    pvOS);
#pragma weak kHeapDbgException


/**
 * The LIBC Sys Backend exception handler.
 *
 * @returns XCPT_CONTINUE_SEARCH or XCPT_CONTINUE_EXECUTION.
 * @param   pXcptRepRec     Report record.
 * @param   pXcptRegRec     Registration record.
 * @param   pCtx            Context record.
 * @param   pvWhatEver      Not quite sure what this is...
 */
ULONG _System __libc_Back_exceptionHandler(PEXCEPTIONREPORTRECORD       pXcptRepRec,
                                           PEXCEPTIONREGISTRATIONRECORD pXcptRegRec,
                                           PCONTEXTRECORD               pCtx,
                                           PVOID                        pvWhatEver)
{
    __asm__ ("cld");                    /* usual paranoia.  */
    siginfo_t   SigInfo = {0};
    int         rc;                     /* return from __libc_Back_signalRaise() */

    if (pXcptRepRec->fHandlerFlags & (EH_UNWINDING | EH_EXIT_UNWIND))
        return XCPT_CONTINUE_SEARCH;

    /*
     * Init rc and SigInfo with defaults.
     */
    rc = __LIBC_BSRR_PASSITON | __LIBC_BSRR_INTERRUPT;
    SigInfo.si_addr = (void *)pCtx->ctx_RegEip; /* thunk it too? */

    switch (pXcptRepRec->ExceptionNum)
    {
        /*
         * SIGINT, SIGBREAK, SIGTERM
         */
        case XCPT_SIGNAL:
            if (pXcptRepRec->cParameters >= 1) /* (paranoia) */
            {
                switch (pXcptRepRec->ExceptionInfo[0])
                {
                    case XCPT_SIGNAL_INTR:
                    case XCPT_SIGNAL_BREAK:
                    case XCPT_SIGNAL_KILLPROC:
                        /*
                         * We don't wanna process signals unless we've got a thread structure.
                         * Very unlikely event, but we don't wanna allocate stuff at this time.
                         */
                        if (!__libc_threadCurrentNoAuto())
                            return XCPT_CONTINUE_SEARCH;

                        /* Acknowlegde the signal and raise it in the LIBC fashion. */
                        DosAcknowledgeSignalException(pXcptRepRec->ExceptionInfo[0]);
                        switch (pXcptRepRec->ExceptionInfo[0])
                        {
                            case XCPT_SIGNAL_INTR:      SigInfo.si_signo = SIGINT; break;
                            case XCPT_SIGNAL_BREAK:     SigInfo.si_signo = SIGBREAK; break;
                            case XCPT_SIGNAL_KILLPROC:  SigInfo.si_signo = SIGTERM; break;
                        }
                        rc = __libc_Back_signalRaise(SigInfo.si_signo,  0, &pXcptRepRec, __LIBC_BSRF_EXTERNAL);
                        break;
                }
            }
            break;

        /*
         * SIGSEGV
         */
        case XCPT_ACCESS_VIOLATION:
            /* If we're linking libc01.elh or someone is linking static libc
               together with kLib the electric fence heap will get the opportunity
               to check if any access violation was caused by someone touching any
               of the electric fences. */
            if (    kHeapDbgException
                &&  (   pXcptRepRec->ExceptionInfo[0] == XCPT_READ_ACCESS
                     || pXcptRepRec->ExceptionInfo[0] == XCPT_WRITE_ACCESS
                     || pXcptRepRec->ExceptionInfo[0] == XCPT_EXECUTE_ACCESS
                     || pXcptRepRec->ExceptionInfo[0] == XCPT_UNKNOWN_ACCESS)
               )
            {
                ENMACCESS enmAccess = enmRead;
                switch (pXcptRepRec->ExceptionInfo[0])
                {
                    case XCPT_WRITE_ACCESS:     enmAccess = enmWrite; break;
                    case XCPT_UNKNOWN_ACCESS:   enmAccess = enmUnknown; break;
                }
                /* This call returns true if the page was commited in order
                   to workaround the immediate problem. If it returns false
                   the default action should be taken. */
                if (kHeapDbgException((void*)pXcptRepRec->ExceptionInfo[1],
                                      enmAccess,
                                      pXcptRepRec->ExceptionAddress,
                                      pXcptRepRec))
                    return XCPT_CONTINUE_EXECUTION;
            }
            /* take signal */
            SigInfo.si_signo = SIGSEGV;
            SigInfo.si_addr = (void*)pXcptRepRec->ExceptionInfo[1]; /* accessed memory address */
            SigInfo.si_code = SEGV_ACCERR;
            rc = __libc_Back_signalRaise(SigInfo.si_signo, &SigInfo, &pXcptRepRec, __LIBC_BSRF_HARDWARE | __LIBC_BSRF_THREAD);
            break;

        /*
         * SIGBUS
         */
        case XCPT_DATATYPE_MISALIGNMENT:
            SigInfo.si_signo = SIGBUS;
            SigInfo.si_code  = BUS_ADRALN;
            SigInfo.si_addr  = (void*)pXcptRepRec->ExceptionInfo[2]; /* accessed memory address */
            rc = __libc_Back_signalRaise(SigInfo.si_signo, &SigInfo, &pXcptRepRec, __LIBC_BSRF_HARDWARE | __LIBC_BSRF_THREAD);
            break;

        /*
         * SIGFPE
         */
        case XCPT_INTEGER_DIVIDE_BY_ZERO:
            SigInfo.si_signo = SIGFPE;
            SigInfo.si_code  = FPE_INTDIV;
            SigInfo.si_addr  = (void *)pCtx->ctx_RegEip;
            rc = __libc_Back_signalRaise(SigInfo.si_signo, &SigInfo, &pXcptRepRec, __LIBC_BSRF_HARDWARE | __LIBC_BSRF_THREAD);
            break;

        case XCPT_INTEGER_OVERFLOW:
            SigInfo.si_signo = SIGFPE;
            SigInfo.si_code  = FPE_INTOVF;
            SigInfo.si_addr  = (void *)pCtx->ctx_RegEip;
            rc = __libc_Back_signalRaise(SigInfo.si_signo, &SigInfo, &pXcptRepRec, __LIBC_BSRF_HARDWARE | __LIBC_BSRF_THREAD);
            break;

        case XCPT_FLOAT_DIVIDE_BY_ZERO:
            SigInfo.si_signo = SIGFPE;
            SigInfo.si_code  = FPE_FLTDIV;
            SigInfo.si_addr  = (void *)pCtx->ctx_RegEip;
            rc = __libc_Back_signalRaise(SigInfo.si_signo, &SigInfo, &pXcptRepRec, __LIBC_BSRF_HARDWARE | __LIBC_BSRF_THREAD);
            break;

        case XCPT_FLOAT_OVERFLOW:
            SigInfo.si_signo = SIGFPE;
            SigInfo.si_code  = FPE_FLTOVF;
            SigInfo.si_addr  = (void *)pCtx->ctx_RegEip;
            rc = __libc_Back_signalRaise(SigInfo.si_signo, &SigInfo, &pXcptRepRec, __LIBC_BSRF_HARDWARE | __LIBC_BSRF_THREAD);
            break;

        case XCPT_FLOAT_UNDERFLOW:
            SigInfo.si_signo = SIGFPE;
            SigInfo.si_code  = FPE_FLTUND;
            SigInfo.si_addr  = (void *)pCtx->ctx_RegEip;
            rc = __libc_Back_signalRaise(SigInfo.si_signo, &SigInfo, &pXcptRepRec, __LIBC_BSRF_HARDWARE | __LIBC_BSRF_THREAD);
            break;

        case XCPT_FLOAT_DENORMAL_OPERAND:
            SigInfo.si_signo = SIGFPE;
            SigInfo.si_code  = FPE_FLTINV; /* ??? */
            SigInfo.si_addr  = (void *)pCtx->ctx_RegEip;
            rc = __libc_Back_signalRaise(SigInfo.si_signo, &SigInfo, &pXcptRepRec, __LIBC_BSRF_HARDWARE | __LIBC_BSRF_THREAD);
            break;

        case XCPT_FLOAT_INEXACT_RESULT:
            SigInfo.si_signo = SIGFPE;
            SigInfo.si_code  = FPE_FLTRES;
            SigInfo.si_addr  = (void *)pCtx->ctx_RegEip;
            rc = __libc_Back_signalRaise(SigInfo.si_signo, &SigInfo, &pXcptRepRec, __LIBC_BSRF_HARDWARE | __LIBC_BSRF_THREAD);
            break;

        case XCPT_FLOAT_INVALID_OPERATION:
            SigInfo.si_signo = SIGFPE;
            SigInfo.si_code  = FPE_FLTINV;
            SigInfo.si_addr  = (void *)pCtx->ctx_RegEip;
            rc = __libc_Back_signalRaise(SigInfo.si_signo, &SigInfo, &pXcptRepRec, __LIBC_BSRF_HARDWARE | __LIBC_BSRF_THREAD);
            break;

        case XCPT_FLOAT_STACK_CHECK:
            SigInfo.si_signo = SIGFPE;
            SigInfo.si_code  = FPE_FLTINV; /* ??? */
            SigInfo.si_addr  = (void *)pCtx->ctx_RegEip;
            rc = __libc_Back_signalRaise(SigInfo.si_signo, &SigInfo, &pXcptRepRec, __LIBC_BSRF_HARDWARE | __LIBC_BSRF_THREAD);
            break;

        case XCPT_ARRAY_BOUNDS_EXCEEDED:
            SigInfo.si_signo = SIGFPE;
            SigInfo.si_code  = FPE_FLTSUB;
            SigInfo.si_addr  = (void *)pCtx->ctx_RegEip;
            rc = __libc_Back_signalRaise(SigInfo.si_signo, &SigInfo, &pXcptRepRec, __LIBC_BSRF_HARDWARE | __LIBC_BSRF_THREAD);
            break;


        /*
         * SIGILL
         */
        case XCPT_ILLEGAL_INSTRUCTION:
            SigInfo.si_signo = SIGILL;
            SigInfo.si_code  = ILL_ILLOPC; /* this could be any ILL_ILLO* */
            SigInfo.si_addr  = (void *)pCtx->ctx_RegEip;
            rc = __libc_Back_signalRaise(SigInfo.si_signo, &SigInfo, &pXcptRepRec, __LIBC_BSRF_HARDWARE | __LIBC_BSRF_THREAD);
            break;

        case XCPT_INVALID_LOCK_SEQUENCE:
            SigInfo.si_signo = SIGILL;
            SigInfo.si_code  = ILL_ILLADR; /* ?????? */
            SigInfo.si_addr  = (void *)pCtx->ctx_RegEip;
            rc = __libc_Back_signalRaise(SigInfo.si_signo, &SigInfo, &pXcptRepRec, __LIBC_BSRF_HARDWARE | __LIBC_BSRF_THREAD);
            break;

        case XCPT_PRIVILEGED_INSTRUCTION:
            SigInfo.si_signo = SIGILL;
            SigInfo.si_code  = ILL_PRVOPC; /* this could be ILL_PRVREG too. */
            SigInfo.si_addr  = (void *)pCtx->ctx_RegEip;
            rc = __libc_Back_signalRaise(SigInfo.si_signo, &SigInfo, &pXcptRepRec, __LIBC_BSRF_HARDWARE | __LIBC_BSRF_THREAD);
            break;

        /*
         * SIGTRAP (probably non of these are actually implemented by OS/2)
         */
        case XCPT_SINGLE_STEP:
            SigInfo.si_signo = SIGTRAP;
            SigInfo.si_code  = TRAP_TRACE; /* (?) */
            SigInfo.si_addr  = (void *)pCtx->ctx_RegEip;
            rc = __libc_Back_signalRaise(SigInfo.si_signo, &SigInfo, &pXcptRepRec, __LIBC_BSRF_HARDWARE | __LIBC_BSRF_THREAD);
            break;
        case XCPT_BREAKPOINT:
            SigInfo.si_signo = SIGTRAP;
            SigInfo.si_code  = TRAP_BRKPT;
            SigInfo.si_addr  = (void *)pCtx->ctx_RegEip;
            rc = __libc_Back_signalRaise(SigInfo.si_signo, &SigInfo, &pXcptRepRec, __LIBC_BSRF_HARDWARE | __LIBC_BSRF_THREAD);
            break;

        /*
         * SIGKILL: XCPT_PROCESS_TERMINATE, XCPT_ASYNC_PROCESS_TERMINATE
         * But no programs should see this signal.
         */
        case XCPT_ASYNC_PROCESS_TERMINATE:
        {
            /*
             * This exception is special. We use it to poke threads with pending signals.
             * __libc_Back_signalRaisePoked() will retest the flag from within the semaphore
             * protection and returns __LIBC_BSRR_ERROR if it fails.
             */
            __LIBC_PTHREAD  pThrd = __libc_threadCurrentNoAuto();
            if (    !pThrd
                ||  !pThrd->fSigBeingPoked
                ||  (rc = __libc_back_signalRaisePoked(&pXcptRepRec, pXcptRepRec->ExceptionInfo[0])) < 0)
            {
                #if 0 /** bird: no so sure about this. */
                /*
                 * For all but thread 1 we commit suicide.
                 *
                 * This should not cause any particular amount of trouble since we are
                 * the bottom most exception handler if we're installed.
                 */
                PTIB    pTib;
                PPIB    pPib;
                FS_VAR();
                FS_SAVE();

                if (    !DosGetInfoBlocks(&pTib, &pPib)
                    &&  pTib->tib_ptib2->tib2_ultid != 1)
                {
                    for (;;)
                        DosExit(EXIT_THREAD, 0);
                }
                FS_RESTORE();
                #endif
                return XCPT_CONTINUE_SEARCH;
            }
            break;
        }

        case XCPT_PROCESS_TERMINATE:
            /* Do nothing ATM - could be use for tls cleanup later. */
            break;

        case 0x71785158: /* EXCEPTQ_DEBUG_EXCEPTION */
            /*
             * Call panic without termination to trigger an EXCEPTQ report. Note that
             * EXCEPTQ_DEBUG_EXCEPTION supports the format string in pXcptRepRec.ExceptionInfo[0]
             * with arguments coming in ExceptionInfo[1] - ExceptionInfo[3]. Use this as a message
             * if present.
             */
            if (pXcptRepRec->cParameters)
            {
                const char *pszMessage = NULL;
                if (pXcptRepRec->ExceptionInfo[0])
                {
                    ULONG size = 0x1000, flags;
                    APIRET arc = DosQueryMem((PVOID)pXcptRepRec->ExceptionInfo[0], &size, &flags);
                    if (arc == 0 && (flags & PAG_COMMIT))
                        pszMessage = (const char *)pXcptRepRec->ExceptionInfo[0];
                }
                if (!pszMessage)
                {
                    switch (pXcptRepRec->cParameters)
                    {
                        case 2: pszMessage = "[1]=%p"; break;
                        case 3: pszMessage = "[1]=%p [2]=%p"; break;
                        default: pszMessage = "[1]=%p [2]=%p [3]=%p"; break;
                    }
                }
                __libc_Back_panic(__LIBC_PANIC_SIGNAL | __LIBC_PANIC_NO_SPM_TERM | __LIBC_PANIC_XCPTPARAMS | __LIBC_PANIC_NO_TERMINATE,
                                &pXcptRepRec, pszMessage,
                                pXcptRepRec->ExceptionInfo[1], pXcptRepRec->ExceptionInfo[2], pXcptRepRec->ExceptionInfo[3]);
                break;
            }
            __libc_Back_panic(__LIBC_PANIC_SIGNAL | __LIBC_PANIC_NO_SPM_TERM | __LIBC_PANIC_XCPTPARAMS | __LIBC_PANIC_NO_TERMINATE,
                              &pXcptRepRec, NULL);
            break;
    }

    return rc > 0 && (rc & __LIBC_BSRR_PASSITON) ? XCPT_CONTINUE_SEARCH : XCPT_CONTINUE_EXECUTION;
}

