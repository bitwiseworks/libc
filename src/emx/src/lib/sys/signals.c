/* $Id: signals.c 3795 2012-05-24 19:37:59Z bird $ */
/** @file
 *
 * LIBC SYS Backend - Signals.
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


/** @page   LIBCSignals     Signals
 *
 * LIBC fully implements the posix signal handling.
 *
 * Interprocess signaling is only available when target is linked to LIBCxyz.DLL.
 * When the target is a LIBC process the signal is queued in SPM before the
 * target process is actually poked. When the target is a non-LIBC process the
 * signal is attempted converted to EMX and signaled in the EMX fashion. This
 * obviously means that certain restrictions applies when signaling EMX processes.
 *
 *
 * Signals are pending in on of three places depending on how fare they are
 * scheduled. Scheduling levels and pending location:
 *          - Pending on the SPM process, not scheduled.
 *          - Pending in process wide queues, not scheduled.
 *          - Pending on thread, scheduled.
 *
 * There are generally speaking fource sources signals:
 *          - Internal software signals in current thread, raise()/kill(getpid())/sigqueue(getpid()).
 *          - Other thread, pthread_kill(). This needs no process scheduling, only
 *            priority ordering on delivery.
 *          - OS/2 hardware exceptions. Generally no scheduling required, these must be
 *            handled promptly on the current thread.
 *          - External signals, queued in SPM, EMX signals, or OS/2 signal exceptions.
 *
 * When talking scheduling this means we'll reschedule the entire process when:
 *          - an external signal arrives.
 *          - an internal software signal occurs (raise/kill/sigqueue).
 *          - the signal mask of a thread changes.
 *            Two cases, 1) unblocking a signal no, 2) blocking signal no with
 *            pending signal. The 2nd case is very uncommon and could perhaps be
 *            ignored or handled differently. -bird: we do that now!
 *
 *
 */


/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#include "libc-alias.h"
#define INCL_BASE
#define INCL_DOSSIGNALS
#define INCL_FSMACROS
#define INCL_EXAPIS
#include <os2emx.h>

#include <signal.h>
#include <errno.h>
#include <string.h>
#include <malloc.h>
#include <386/builtin.h>
#include <emx/umalloc.h>
#include <InnoTekLIBC/thread.h>
#include <InnoTekLIBC/fork.h>
#include <InnoTekLIBC/backend.h>
#include <InnoTekLIBC/FastInfoBlocks.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_BACK_SIGNAL
#include <InnoTekLIBC/logstrict.h>
#include "syscalls.h"
#include "backend.h"
#include "b_signal.h"
#include "b_process.h"


/*******************************************************************************
*   Defined Constants And Macros                                               *
*******************************************************************************/
/** OS/2 default standard error handle. */
#define HFILE_STDERR    ((HFILE)2)

/** @defgroup libc_back_signals_properties  Signal Properties
 * The return actions are ordered by precendece.
 * @{ */
/** Return Action: Terminate. */
#define SPR_KILL        0x0000
/** Return Action: Restart instruction. */
#define SPR_CONTINUE    0x0001
/** Return Action Mask */
#define SPR_MASK        0x0001

/** Action: Ignore the signal. */
#define SPA_IGNORE      0x0000
/** Action: Terminate the process. */
#define SPA_KILL        0x0010
/** Action: Perform coredump killing the process. */
#define SPA_CORE        0x0020
/** Action: Suspend the process. */
#define SPA_STOP        0x0030
/** Action: Suspend the process from the tty. */
#define SPA_STOPTTY     0x0040
/** Action: Resume(/continue) suspended process. */
#define SPA_RESUME      0x0050
/** Action: Next exception handler. */
#define SPA_NEXT        0x0060
/** Action: Terminate if primary exception handler, next exception handler if not. */
#define SPA_NEXT_KILL   0x0070
/** Action: Coredump+term if primary exception handler, next exception handler if not. */
#define SPA_NEXT_CORE   0x0080
/** Action Mask */
#define SPA_MASK        0x00f0

/** Property: Catchable but not blockable. */
#define SPP_NOBLOCK     0x0100
/** Property: Not catchable. */
#define SPP_NOCATCH     0x0200
/** Property: Anything can service this signal. */
#define SPP_ANYTHRD     0x0400
/** Property: Only thread 1 can service this signal. */
#define SPP_THRDONE     0x0800
/** Property: This signal can be queued. */
#define SPP_QUEUED      0x1000
/** Property: Signal action cannot be automatically reset in SysV fashion. */
#define SPP_NORESET     0x2000
/** @} */


/** EMX signal numbers.
 * @{ */
#define EMX_SIGHUP    1
#define EMX_SIGINT    2
#define EMX_SIGQUIT   3
#define EMX_SIGILL    4
#define EMX_SIGTRAP   5
#define EMX_SIGABRT   6
#define EMX_SIGEMT    7
#define EMX_SIGFPE    8
#define EMX_SIGKILL   9
#define EMX_SIGBUS   10
#define EMX_SIGSEGV  11
#define EMX_SIGSYS   12
#define EMX_SIGPIPE  13
#define EMX_SIGALRM  14
#define EMX_SIGTERM  15
#define EMX_SIGUSR1  16
#define EMX_SIGUSR2  17
#define EMX_SIGCHLD  18
#define EMX_SIGBREAK 21
#define EMX_SIGWINCH 28
/** @} */


/*******************************************************************************
*   Structures and Typedefs                                                    *
*******************************************************************************/

/** A queued signal.
 * (Per process variant.)
 */
typedef struct SignalQueued
{
    /** Pointer to the next signal in the queue. */
    struct SignalQueued    *pNext;
    /** Pointer to the previous signal in the queue. */
    struct SignalQueued    *pPrev;
    /** How to free this signal. */
    enum enmSignalQueuedHowToFree
    {
        /** Free from pre allocated chunk. */
        enmHowFree_PreAllocFree,
        /** Free from shared heap. */
        enmHowFree_HeapFree,
        /** Don't free, keep within thread, it's stack. */
        enmHowFree_Temporary
    }                       enmHowFree;
    /** Signal info. */
    siginfo_t               SigInfo;
} SIGQUEUED, *PSIGQUEUED;


/** Thread enumeration state data.
 * Used by signalScheduleThread() when enumerating threads.
 */
typedef struct SigSchedEnumParam
{
    /** Signal number. */
    int             iSignalNo;
    /** Current thread. */
    __LIBC_PTHREAD  pThrd;
} SIGSCHEDENUMPARAM, *PSIGSCHEDENUMPARAM;


/**
 * Exception handler argument list.
 */
typedef struct XcptParams
{
    PEXCEPTIONREPORTRECORD       pXcptRepRec;
    PEXCEPTIONREGISTRATIONRECORD pXcptRegRec;
    PCONTEXTRECORD               pCtx;
    PVOID                        pvWhatEver;
} XCPTPARAMS, *PEXCPTPARAMS;


/*******************************************************************************
*   Global Variables                                                           *
*******************************************************************************/

/** Signal Mutex.
 * This mutex basically protects all the signal constructs in the process. A
 * complete list of what this is should be provided later.
 */
static __LIBC_SAFESEMMTX    gmtxSignals;

/** Wait Event Semaphore.
 * This is an event semaphore which will never be posted (unless we wanna resolve
 * some nasty deadlock in the future) but is used to wait for a signal to arrive.
 * When a signal arrives the wait will be interrupted to allow for execution of
 * the exception and signal processing. DosWaitEventSem will return ERROR_INTERRUPT.
 */
HEV              __libc_back_ghevWait;

static const char gaszSignalNames[__SIGSET_MAXSIGNALS][12] =
{
    "SIG0",                             /* SIG0 */
    "SIGHUP",                           /* SIGHUP    1     /-- POSIX: Hangup */
    "SIGINT",                           /* SIGINT    2     /-- ANSI: Interrupt (Ctrl-C) */
    "SIGQUIT",                          /* SIGQUIT   3     /-- POSIX: Quit */
    "SIGILL",                           /* SIGILL    4     /-- ANSI: Illegal instruction */
    "SIGTRAP",                          /* SIGTRAP   5     /-- POSIX: Single step (debugging) */
    "SIGABRT",                          /* SIGABRT   6     /-- ANSI: abort () */
    "SIGEMT",                           /* SIGEMT    7     /-- BSD: EMT instruction */
    "SIGFPE",                           /* SIGFPE    8     /-- ANSI: Floating point */
    "SIGKILL",                          /* SIGKILL   9     /-- POSIX: Kill process */
    "SIGBUS",                           /* SIGBUS   10     /-- BSD 4.2: Bus error */
    "SIGSEGV",                          /* SIGSEGV  11     /-- ANSI: Segmentation fault */
    "SIGSYS",                           /* SIGSYS   12     /-- Invalid argument to system call */
    "SIGPIPE",                          /* SIGPIPE  13     /-- POSIX: Broken pipe. */
    "SIGALRM",                          /* SIGALRM  14     /-- POSIX: Alarm. */
    "SIGTERM",                          /* SIGTERM  15     /-- ANSI: Termination, process killed */
    "SIGURG",                           /* SIGURG   16     /-- POSIX/BSD: urgent condition on IO channel */
    "SIGSTOP",                          /* SIGSTOP  17     /-- POSIX: Sendable stop signal not from tty. unblockable. */
    "SIGTSTP",                          /* SIGTSTP  18     /-- POSIX: Stop signal from tty. */
    "SIGCONT",                          /* SIGCONT  19     /-- POSIX: Continue a stopped process. */
    "SIGCHLD",                          /* SIGCHLD  20     /-- POSIX: Death or stop of a child process. (EMX: 18) */
    "SIGTTIN",                          /* SIGTTIN  21     /-- POSIX: To readers pgrp upon background tty read. */
    "SIGTTOU",                          /* SIGTTOU  22     /-- POSIX: To readers pgrp upon background tty write. */
    "SIGIO",                            /* SIGIO    23     /-- BSD: Input/output possible signal. */
    "SIGXCPU",                          /* SIGXCPU  24     /-- BSD 4.2: Exceeded CPU time limit. */
    "SIGXFSZ",                          /* SIGXFSZ  25     /-- BSD 4.2: Exceeded file size limit. */
    "SIGVTALRM",                        /* SIGVTALRM 26    /-- BSD 4.2: Virtual time alarm. */
    "SIGPROF",                          /* SIGPROF  27     /-- BSD 4.2: Profiling time alarm. */
    "SIGWINCH",                         /* SIGWINCH 28     /-- BSD 4.3: Window size change (not implemented). */
    "SIGBREAK",                         /* SIGBREAK 29     /-- OS/2: Break (Ctrl-Break). (EMX: 21) */
    "SIGUSR1",                          /* SIGUSR1  30     /-- POSIX: User-defined signal #1 */
    "SIGUSR2",                          /* SIGUSR2  31     /-- POSIX: User-defined signal #2 */
    "SIGBREAK",                         /* SIGBREAK 32     /-- OS/2: Break (Ctrl-Break). (EMX: 21) */

    "SIGRT0",                           /* SIGRTMIN +  0 */
    "SIGRT1",                           /* SIGRTMIN +  1 */
    "SIGRT2",                           /* SIGRTMIN +  2 */
    "SIGRT3",                           /* SIGRTMIN +  3 */
    "SIGRT4",                           /* SIGRTMIN +  4 */
    "SIGRT5",                           /* SIGRTMIN +  5 */
    "SIGRT6",                           /* SIGRTMIN +  6 */
    "SIGRT7",                           /* SIGRTMIN +  7 */
    "SIGRT8",                           /* SIGRTMIN +  8 */
    "SIGRT9",                           /* SIGRTMIN +  9 */
    "SIGRT10",                          /* SIGRTMIN + 10 */
    "SIGRT11",                          /* SIGRTMIN + 11 */
    "SIGRT12",                          /* SIGRTMIN + 12 */
    "SIGRT13",                          /* SIGRTMIN + 13 */
    "SIGRT14",                          /* SIGRTMIN + 14 */
    "SIGRT15",                          /* SIGRTMIN + 15 */
    "SIGRT16",                          /* SIGRTMIN + 16 */
    "SIGRT17",                          /* SIGRTMIN + 17 */
    "SIGRT18",                          /* SIGRTMIN + 18 */
    "SIGRT19",                          /* SIGRTMIN + 19 */
    "SIGRT20",                          /* SIGRTMIN + 20 */
    "SIGRT21",                          /* SIGRTMIN + 21 */
    "SIGRT22",                          /* SIGRTMIN + 22 */
    "SIGRT23",                          /* SIGRTMIN + 23 */
    "SIGRT24",                          /* SIGRTMIN + 24 */
    "SIGRT25",                          /* SIGRTMIN + 25 */
    "SIGRT26",                          /* SIGRTMIN + 26 */
    "SIGRT27",                          /* SIGRTMIN + 27 */
    "SIGRT28",                          /* SIGRTMIN + 28 */
    "SIGRT29",                          /* SIGRTMIN + 29 */
    "SIGRT30",                          /* SIGRTMIN + 30 == SIGRTMAX */
};

/** Signal Properties.
 * Describes the default actions of a signal.
 */
static const unsigned short    gafSignalProperties[__SIGSET_MAXSIGNALS] =
{
    /* return action | default action | property [|anotherprop] */
    SPR_CONTINUE | SPA_IGNORE,                                          /* SIG0 */
    SPR_CONTINUE | SPA_KILL    | SPP_ANYTHRD,                           /* SIGHUP    1     /-- POSIX: Hangup */
    SPR_CONTINUE | SPA_KILL    | SPP_ANYTHRD,                           /* SIGINT    2     /-- ANSI: Interrupt (Ctrl-C) */
    SPR_CONTINUE | SPA_CORE    | SPP_ANYTHRD,                           /* SIGQUIT   3     /-- POSIX: Quit */
    SPR_CONTINUE | SPA_NEXT_CORE | SPP_NORESET,                         /* SIGILL    4     /-- ANSI: Illegal instruction */
    SPR_CONTINUE | SPA_NEXT_KILL | SPP_NORESET,                         /* SIGTRAP   5     /-- POSIX: Single step (debugging) */
    SPR_CONTINUE | SPA_CORE,                                            /* SIGABRT   6     /-- ANSI: abort () */
    SPR_CONTINUE | SPA_CORE    | SPP_ANYTHRD,                           /* SIGEMT    7     /-- BSD: EMT instruction */
    SPR_CONTINUE | SPA_NEXT_CORE,                                       /* SIGFPE    8     /-- ANSI: Floating point */
    SPR_KILL     | SPA_KILL | SPP_ANYTHRD | SPP_NOBLOCK | SPP_NOCATCH,  /* SIGKILL   9     /-- POSIX: Kill process */
    SPR_CONTINUE | SPA_CORE,                                            /* SIGBUS   10     /-- BSD 4.2: Bus error */
    SPR_CONTINUE | SPA_NEXT_CORE,                                       /* SIGSEGV  11     /-- ANSI: Segmentation fault */
    SPR_CONTINUE | SPA_CORE,                                            /* SIGSYS   12     /-- Invalid argument to system call */
    SPR_CONTINUE | SPA_KILL    | SPP_ANYTHRD,                           /* SIGPIPE  13     /-- POSIX: Broken pipe. */
    SPR_CONTINUE | SPA_KILL    | SPP_ANYTHRD,                           /* SIGALRM  14     /-- POSIX: Alarm. */
    SPR_CONTINUE | SPA_NEXT_KILL | SPP_ANYTHRD,                         /* SIGTERM  15     /-- ANSI: Termination, process killed */
    SPR_CONTINUE | SPA_IGNORE,                                          /* SIGURG   16     /-- POSIX/BSD: urgent condition on IO channel */
    SPR_CONTINUE | SPA_STOP | SPP_THRDONE | SPP_NOBLOCK | SPP_NOCATCH,  /* SIGSTOP  17     /-- POSIX: Sendable stop signal not from tty. unblockable. */
    SPR_CONTINUE | SPA_STOPTTY | SPP_THRDONE,                           /* SIGTSTP  18     /-- POSIX: Stop signal from tty. */
    SPR_CONTINUE | SPA_RESUME  | SPP_THRDONE,                           /* SIGCONT  19     /-- POSIX: Continue a stopped process. */
    SPR_CONTINUE | SPA_IGNORE  | SPP_ANYTHRD | SPP_QUEUED,              /* SIGCHLD  20     /-- POSIX: Death or stop of a child process. (EMX: 18) */
    SPR_CONTINUE | SPA_STOPTTY | SPP_THRDONE,                           /* SIGTTIN  21     /-- POSIX: To readers pgrp upon background tty read. */
    SPR_CONTINUE | SPA_STOPTTY | SPP_THRDONE,                           /* SIGTTOU  22     /-- POSIX: To readers pgrp upon background tty write. */
    SPR_CONTINUE | SPA_IGNORE  | SPP_ANYTHRD,                           /* SIGIO    23     /-- BSD: Input/output possible signal. */
    SPR_CONTINUE | SPA_KILL,                                            /* SIGXCPU  24     /-- BSD 4.2: Exceeded CPU time limit. */
    SPR_CONTINUE | SPA_KILL,                                            /* SIGXFSZ  25     /-- BSD 4.2: Exceeded file size limit. */
    SPR_CONTINUE | SPA_KILL    | SPP_ANYTHRD,                           /* SIGVTALRM 26    /-- BSD 4.2: Virtual time alarm. */
    SPR_CONTINUE | SPA_KILL    | SPP_ANYTHRD,                           /* SIGPROF  27     /-- BSD 4.2: Profiling time alarm. */
    SPR_CONTINUE | SPA_IGNORE  | SPP_ANYTHRD,                           /* SIGWINCH 28     /-- BSD 4.3: Window size change (not implemented). */
    SPR_CONTINUE | SPA_NEXT_KILL | SPP_ANYTHRD,                         /* SIGBREAK 29     /-- OS/2: Break (Ctrl-Break). (EMX: 21) */
    SPR_CONTINUE | SPA_KILL    | SPP_ANYTHRD,                           /* SIGUSR1  30     /-- POSIX: User-defined signal #1 */
    SPR_CONTINUE | SPA_KILL    | SPP_ANYTHRD,                           /* SIGUSR2  31     /-- POSIX: User-defined signal #2 */
    SPR_CONTINUE | SPA_NEXT_KILL | SPP_ANYTHRD,                         /* SIGBREAK 32     /-- OS/2: Break (Ctrl-Break). (EMX: 21) */
    /* real time signals */
    SPR_CONTINUE | SPA_CORE | SPP_ANYTHRD | SPP_QUEUED,                 /* SIGRTMIN +  0 */
    SPR_CONTINUE | SPA_CORE | SPP_ANYTHRD | SPP_QUEUED,                 /* SIGRTMIN +  1 */
    SPR_CONTINUE | SPA_CORE | SPP_ANYTHRD | SPP_QUEUED,                 /* SIGRTMIN +  2 */
    SPR_CONTINUE | SPA_CORE | SPP_ANYTHRD | SPP_QUEUED,                 /* SIGRTMIN +  3 */
    SPR_CONTINUE | SPA_CORE | SPP_ANYTHRD | SPP_QUEUED,                 /* SIGRTMIN +  4 */
    SPR_CONTINUE | SPA_CORE | SPP_ANYTHRD | SPP_QUEUED,                 /* SIGRTMIN +  5 */
    SPR_CONTINUE | SPA_CORE | SPP_ANYTHRD | SPP_QUEUED,                 /* SIGRTMIN +  6 */
    SPR_CONTINUE | SPA_CORE | SPP_ANYTHRD | SPP_QUEUED,                 /* SIGRTMIN +  7 */
    SPR_CONTINUE | SPA_CORE | SPP_ANYTHRD | SPP_QUEUED,                 /* SIGRTMIN +  8 */
    SPR_CONTINUE | SPA_CORE | SPP_ANYTHRD | SPP_QUEUED,                 /* SIGRTMIN +  9 */
    SPR_CONTINUE | SPA_CORE | SPP_ANYTHRD | SPP_QUEUED,                 /* SIGRTMIN + 10 */
    SPR_CONTINUE | SPA_CORE | SPP_ANYTHRD | SPP_QUEUED,                 /* SIGRTMIN + 11 */
    SPR_CONTINUE | SPA_CORE | SPP_ANYTHRD | SPP_QUEUED,                 /* SIGRTMIN + 12 */
    SPR_CONTINUE | SPA_CORE | SPP_ANYTHRD | SPP_QUEUED,                 /* SIGRTMIN + 13 */
    SPR_CONTINUE | SPA_CORE | SPP_ANYTHRD | SPP_QUEUED,                 /* SIGRTMIN + 14 */
    SPR_CONTINUE | SPA_CORE | SPP_ANYTHRD | SPP_QUEUED,                 /* SIGRTMIN + 15 */
    SPR_CONTINUE | SPA_CORE | SPP_ANYTHRD | SPP_QUEUED,                 /* SIGRTMIN + 16 */
    SPR_CONTINUE | SPA_CORE | SPP_ANYTHRD | SPP_QUEUED,                 /* SIGRTMIN + 17 */
    SPR_CONTINUE | SPA_CORE | SPP_ANYTHRD | SPP_QUEUED,                 /* SIGRTMIN + 18 */
    SPR_CONTINUE | SPA_CORE | SPP_ANYTHRD | SPP_QUEUED,                 /* SIGRTMIN + 19 */
    SPR_CONTINUE | SPA_CORE | SPP_ANYTHRD | SPP_QUEUED,                 /* SIGRTMIN + 20 */
    SPR_CONTINUE | SPA_CORE | SPP_ANYTHRD | SPP_QUEUED,                 /* SIGRTMIN + 21 */
    SPR_CONTINUE | SPA_CORE | SPP_ANYTHRD | SPP_QUEUED,                 /* SIGRTMIN + 22 */
    SPR_CONTINUE | SPA_CORE | SPP_ANYTHRD | SPP_QUEUED,                 /* SIGRTMIN + 23 */
    SPR_CONTINUE | SPA_CORE | SPP_ANYTHRD | SPP_QUEUED,                 /* SIGRTMIN + 24 */
    SPR_CONTINUE | SPA_CORE | SPP_ANYTHRD | SPP_QUEUED,                 /* SIGRTMIN + 25 */
    SPR_CONTINUE | SPA_CORE | SPP_ANYTHRD | SPP_QUEUED,                 /* SIGRTMIN + 26 */
    SPR_CONTINUE | SPA_CORE | SPP_ANYTHRD | SPP_QUEUED,                 /* SIGRTMIN + 27 */
    SPR_CONTINUE | SPA_CORE | SPP_ANYTHRD | SPP_QUEUED,                 /* SIGRTMIN + 28 */
    SPR_CONTINUE | SPA_CORE | SPP_ANYTHRD | SPP_QUEUED,                 /* SIGRTMIN + 29 */
    SPR_CONTINUE | SPA_CORE | SPP_ANYTHRD | SPP_QUEUED,                 /* SIGRTMIN + 30 == SIGRTMAX */
};


/** The signal actions for all signals in the system.
 * All access to this array is protected by the signal semaphore.
 */
static struct sigaction    gaSignalActions[__SIGSET_MAXSIGNALS] =
{
    { { .__sa_handler = SIG_DFL }, {{0, 0}}, 0 },   /* SIG0 */
    { { .__sa_handler = SIG_DFL }, {{0, 0}}, 0 },   /* SIGHUP    1     /-- POSIX: Hangup */
    { { .__sa_handler = SIG_DFL }, {{0, 0}}, 0 },   /* SIGINT    2     /-- ANSI: Interrupt (Ctrl-C) */
    { { .__sa_handler = SIG_DFL }, {{0, 0}}, 0 },   /* SIGQUIT   3     /-- POSIX: Quit */
    { { .__sa_handler = SIG_DFL }, {{0, 0}}, 0 },   /* SIGILL    4     /-- ANSI: Illegal instruction */
    { { .__sa_handler = SIG_DFL }, {{0, 0}}, 0 },   /* SIGTRAP   5     /-- POSIX: Single step (debugging) */
    { { .__sa_handler = SIG_DFL }, {{0, 0}}, 0 },   /* SIGABRT   6     /-- ANSI: abort () */
    { { .__sa_handler = SIG_DFL }, {{0, 0}}, 0 },   /* SIGEMT    7     /-- BSD: EMT instruction */
    { { .__sa_handler = SIG_DFL }, {{0, 0}}, 0 },   /* SIGFPE    8     /-- ANSI: Floating point */
    { { .__sa_handler = SIG_DFL }, {{0, 0}}, 0 },   /* SIGKILL   9     /-- POSIX: Kill process */
    { { .__sa_handler = SIG_DFL }, {{0, 0}}, 0 },   /* SIGBUS   10     /-- BSD 4.2: Bus error */
    { { .__sa_handler = SIG_DFL }, {{0, 0}}, 0 },   /* SIGSEGV  11     /-- ANSI: Segmentation fault */
    { { .__sa_handler = SIG_DFL }, {{0, 0}}, 0 },   /* SIGSYS   12     /-- Invalid argument to system call */
    { { .__sa_handler = SIG_DFL }, {{0, 0}}, 0 },   /* SIGPIPE  13     /-- POSIX: Broken pipe. */
    { { .__sa_handler = SIG_DFL }, {{0, 0}}, 0 },   /* SIGALRM  14     /-- POSIX: Alarm. */
    { { .__sa_handler = SIG_DFL }, {{0, 0}}, 0 },   /* SIGTERM  15     /-- ANSI: Termination, process killed */
    { { .__sa_handler = SIG_DFL }, {{0, 0}}, 0 },   /* SIGURG   16     /-- POSIX/BSD: urgent condition on IO channel */
    { { .__sa_handler = SIG_DFL }, {{0, 0}}, 0 },   /* SIGSTOP  17     /-- POSIX: Sendable stop signal not from tty. unblockable. */
    { { .__sa_handler = SIG_DFL }, {{0, 0}}, 0 },   /* SIGTSTP  18     /-- POSIX: Stop signal from tty. */
    { { .__sa_handler = SIG_DFL }, {{0, 0}}, 0 },   /* SIGCONT  19     /-- POSIX: Continue a stopped process. */
    { { .__sa_handler = SIG_DFL }, {{0, 0}}, 0 },   /* SIGCHLD  20     /-- POSIX: Death or stop of a child process. (EMX: 18) */
    { { .__sa_handler = SIG_DFL }, {{0, 0}}, 0 },   /* SIGTTIN  21     /-- POSIX: To readers pgrp upon background tty read. */
    { { .__sa_handler = SIG_DFL }, {{0, 0}}, 0 },   /* SIGTTOU  22     /-- POSIX: To readers pgrp upon background tty write. */
    { { .__sa_handler = SIG_DFL }, {{0, 0}}, 0 },   /* SIGIO    23     /-- BSD: Input/output possible signal. */
    { { .__sa_handler = SIG_DFL }, {{0, 0}}, 0 },   /* SIGXCPU  24     /-- BSD 4.2: Exceeded CPU time limit. */
    { { .__sa_handler = SIG_DFL }, {{0, 0}}, 0 },   /* SIGXFSZ  25     /-- BSD 4.2: Exceeded file size limit. */
    { { .__sa_handler = SIG_DFL }, {{0, 0}}, 0 },   /* SIGVTALRM 26    /-- BSD 4.2: Virtual time alarm. */
    { { .__sa_handler = SIG_DFL }, {{0, 0}}, 0 },   /* SIGPROF  27     /-- BSD 4.2: Profiling time alarm. */
    { { .__sa_handler = SIG_DFL }, {{0, 0}}, 0 },   /* SIGWINCH 28     /-- BSD 4.3: Window size change (not implemented). */
    { { .__sa_handler = SIG_DFL }, {{0, 0}}, 0 },   /* SIGINFO  29     /-- BSD 4.3: Information request. */
    { { .__sa_handler = SIG_DFL }, {{0, 0}}, 0 },   /* SIGUSR1  30     /-- POSIX: User-defined signal #1 */
    { { .__sa_handler = SIG_DFL }, {{0, 0}}, 0 },   /* SIGUSR2  31     /-- POSIX: User-defined signal #2 */
    { { .__sa_handler = SIG_DFL }, {{0, 0}}, 0 },   /* SIGBREAK 32     /-- OS/2: Break (Ctrl-Break). (EMX: 21) */
    /* realtime signals */
    { { .__sa_handler = SIG_DFL }, {{0, 0}}, 0 },   /* SIGRTMIN +  0 */
    { { .__sa_handler = SIG_DFL }, {{0, 0}}, 0 },   /* SIGRTMIN +  1 */
    { { .__sa_handler = SIG_DFL }, {{0, 0}}, 0 },   /* SIGRTMIN +  2 */
    { { .__sa_handler = SIG_DFL }, {{0, 0}}, 0 },   /* SIGRTMIN +  3 */
    { { .__sa_handler = SIG_DFL }, {{0, 0}}, 0 },   /* SIGRTMIN +  4 */
    { { .__sa_handler = SIG_DFL }, {{0, 0}}, 0 },   /* SIGRTMIN +  5 */
    { { .__sa_handler = SIG_DFL }, {{0, 0}}, 0 },   /* SIGRTMIN +  6 */
    { { .__sa_handler = SIG_DFL }, {{0, 0}}, 0 },   /* SIGRTMIN +  7 */
    { { .__sa_handler = SIG_DFL }, {{0, 0}}, 0 },   /* SIGRTMIN +  8 */
    { { .__sa_handler = SIG_DFL }, {{0, 0}}, 0 },   /* SIGRTMIN +  9 */
    { { .__sa_handler = SIG_DFL }, {{0, 0}}, 0 },   /* SIGRTMIN + 10 */
    { { .__sa_handler = SIG_DFL }, {{0, 0}}, 0 },   /* SIGRTMIN + 11 */
    { { .__sa_handler = SIG_DFL }, {{0, 0}}, 0 },   /* SIGRTMIN + 12 */
    { { .__sa_handler = SIG_DFL }, {{0, 0}}, 0 },   /* SIGRTMIN + 13 */
    { { .__sa_handler = SIG_DFL }, {{0, 0}}, 0 },   /* SIGRTMIN + 14 */
    { { .__sa_handler = SIG_DFL }, {{0, 0}}, 0 },   /* SIGRTMIN + 15 */
    { { .__sa_handler = SIG_DFL }, {{0, 0}}, 0 },   /* SIGRTMIN + 16 */
    { { .__sa_handler = SIG_DFL }, {{0, 0}}, 0 },   /* SIGRTMIN + 17 */
    { { .__sa_handler = SIG_DFL }, {{0, 0}}, 0 },   /* SIGRTMIN + 18 */
    { { .__sa_handler = SIG_DFL }, {{0, 0}}, 0 },   /* SIGRTMIN + 19 */
    { { .__sa_handler = SIG_DFL }, {{0, 0}}, 0 },   /* SIGRTMIN + 20 */
    { { .__sa_handler = SIG_DFL }, {{0, 0}}, 0 },   /* SIGRTMIN + 21 */
    { { .__sa_handler = SIG_DFL }, {{0, 0}}, 0 },   /* SIGRTMIN + 22 */
    { { .__sa_handler = SIG_DFL }, {{0, 0}}, 0 },   /* SIGRTMIN + 23 */
    { { .__sa_handler = SIG_DFL }, {{0, 0}}, 0 },   /* SIGRTMIN + 24 */
    { { .__sa_handler = SIG_DFL }, {{0, 0}}, 0 },   /* SIGRTMIN + 25 */
    { { .__sa_handler = SIG_DFL }, {{0, 0}}, 0 },   /* SIGRTMIN + 26 */
    { { .__sa_handler = SIG_DFL }, {{0, 0}}, 0 },   /* SIGRTMIN + 27 */
    { { .__sa_handler = SIG_DFL }, {{0, 0}}, 0 },   /* SIGRTMIN + 28 */
    { { .__sa_handler = SIG_DFL }, {{0, 0}}, 0 },   /* SIGRTMIN + 29 */
    { { .__sa_handler = SIG_DFL }, {{0, 0}}, 0 },   /* SIGRTMIN + 30 == SIGRTMAX */
};


/** Set of signals pending on this process.
 * All access to this set is protected by the signal semaphore.
 */
sigset_t            __libc_gSignalPending = {{0,0}};

/** Head of the queue of pending signals.
 * All access to this set is protected by the signal semaphore.
 */
static PSIGQUEUED   gpSigQueueHead = NULL;
/** Tail of the queue of pending signals.
 * All access to this set is protected by the signal semaphore.
 */
static PSIGQUEUED   gpSigQueueTail = NULL;

/** Array of statically allocated queued signals. */
static SIGQUEUED    gpaSigQueuedPreAlloced[64] =
{
    { .pNext = &gpaSigQueuedPreAlloced[ 1], .enmHowFree = enmHowFree_PreAllocFree },    /* 0 */
    { .pNext = &gpaSigQueuedPreAlloced[ 2], .enmHowFree = enmHowFree_PreAllocFree },    /* 1 */
    { .pNext = &gpaSigQueuedPreAlloced[ 3], .enmHowFree = enmHowFree_PreAllocFree },    /* 2 */
    { .pNext = &gpaSigQueuedPreAlloced[ 4], .enmHowFree = enmHowFree_PreAllocFree },    /* 3 */
    { .pNext = &gpaSigQueuedPreAlloced[ 5], .enmHowFree = enmHowFree_PreAllocFree },    /* 4 */
    { .pNext = &gpaSigQueuedPreAlloced[ 6], .enmHowFree = enmHowFree_PreAllocFree },    /* 5 */
    { .pNext = &gpaSigQueuedPreAlloced[ 7], .enmHowFree = enmHowFree_PreAllocFree },    /* 6 */
    { .pNext = &gpaSigQueuedPreAlloced[ 8], .enmHowFree = enmHowFree_PreAllocFree },    /* 7 */
    { .pNext = &gpaSigQueuedPreAlloced[ 9], .enmHowFree = enmHowFree_PreAllocFree },    /* 8 */
    { .pNext = &gpaSigQueuedPreAlloced[10], .enmHowFree = enmHowFree_PreAllocFree },    /* 9 */
    { .pNext = &gpaSigQueuedPreAlloced[11], .enmHowFree = enmHowFree_PreAllocFree },    /* 10 */
    { .pNext = &gpaSigQueuedPreAlloced[12], .enmHowFree = enmHowFree_PreAllocFree },    /* 11 */
    { .pNext = &gpaSigQueuedPreAlloced[13], .enmHowFree = enmHowFree_PreAllocFree },    /* 12 */
    { .pNext = &gpaSigQueuedPreAlloced[14], .enmHowFree = enmHowFree_PreAllocFree },    /* 13 */
    { .pNext = &gpaSigQueuedPreAlloced[15], .enmHowFree = enmHowFree_PreAllocFree },    /* 14 */
    { .pNext = &gpaSigQueuedPreAlloced[16], .enmHowFree = enmHowFree_PreAllocFree },    /* 15 */
    { .pNext = &gpaSigQueuedPreAlloced[17], .enmHowFree = enmHowFree_PreAllocFree },    /* 16 */
    { .pNext = &gpaSigQueuedPreAlloced[18], .enmHowFree = enmHowFree_PreAllocFree },    /* 17 */
    { .pNext = &gpaSigQueuedPreAlloced[19], .enmHowFree = enmHowFree_PreAllocFree },    /* 18 */
    { .pNext = &gpaSigQueuedPreAlloced[20], .enmHowFree = enmHowFree_PreAllocFree },    /* 19 */
    { .pNext = &gpaSigQueuedPreAlloced[21], .enmHowFree = enmHowFree_PreAllocFree },    /* 20 */
    { .pNext = &gpaSigQueuedPreAlloced[22], .enmHowFree = enmHowFree_PreAllocFree },    /* 21 */
    { .pNext = &gpaSigQueuedPreAlloced[23], .enmHowFree = enmHowFree_PreAllocFree },    /* 22 */
    { .pNext = &gpaSigQueuedPreAlloced[24], .enmHowFree = enmHowFree_PreAllocFree },    /* 23 */
    { .pNext = &gpaSigQueuedPreAlloced[25], .enmHowFree = enmHowFree_PreAllocFree },    /* 24 */
    { .pNext = &gpaSigQueuedPreAlloced[26], .enmHowFree = enmHowFree_PreAllocFree },    /* 25 */
    { .pNext = &gpaSigQueuedPreAlloced[27], .enmHowFree = enmHowFree_PreAllocFree },    /* 26 */
    { .pNext = &gpaSigQueuedPreAlloced[28], .enmHowFree = enmHowFree_PreAllocFree },    /* 27 */
    { .pNext = &gpaSigQueuedPreAlloced[29], .enmHowFree = enmHowFree_PreAllocFree },    /* 28 */
    { .pNext = &gpaSigQueuedPreAlloced[30], .enmHowFree = enmHowFree_PreAllocFree },    /* 29 */
    { .pNext = &gpaSigQueuedPreAlloced[31], .enmHowFree = enmHowFree_PreAllocFree },    /* 30 */
    { .pNext = &gpaSigQueuedPreAlloced[32], .enmHowFree = enmHowFree_PreAllocFree },    /* 31 */
    { .pNext = &gpaSigQueuedPreAlloced[33], .enmHowFree = enmHowFree_PreAllocFree },    /* 32 */
    { .pNext = &gpaSigQueuedPreAlloced[34], .enmHowFree = enmHowFree_PreAllocFree },    /* 33 */
    { .pNext = &gpaSigQueuedPreAlloced[35], .enmHowFree = enmHowFree_PreAllocFree },    /* 34 */
    { .pNext = &gpaSigQueuedPreAlloced[36], .enmHowFree = enmHowFree_PreAllocFree },    /* 35 */
    { .pNext = &gpaSigQueuedPreAlloced[37], .enmHowFree = enmHowFree_PreAllocFree },    /* 36 */
    { .pNext = &gpaSigQueuedPreAlloced[38], .enmHowFree = enmHowFree_PreAllocFree },    /* 37 */
    { .pNext = &gpaSigQueuedPreAlloced[39], .enmHowFree = enmHowFree_PreAllocFree },    /* 38 */
    { .pNext = &gpaSigQueuedPreAlloced[40], .enmHowFree = enmHowFree_PreAllocFree },    /* 39 */
    { .pNext = &gpaSigQueuedPreAlloced[41], .enmHowFree = enmHowFree_PreAllocFree },    /* 40 */
    { .pNext = &gpaSigQueuedPreAlloced[42], .enmHowFree = enmHowFree_PreAllocFree },    /* 41 */
    { .pNext = &gpaSigQueuedPreAlloced[43], .enmHowFree = enmHowFree_PreAllocFree },    /* 42 */
    { .pNext = &gpaSigQueuedPreAlloced[44], .enmHowFree = enmHowFree_PreAllocFree },    /* 43 */
    { .pNext = &gpaSigQueuedPreAlloced[45], .enmHowFree = enmHowFree_PreAllocFree },    /* 44 */
    { .pNext = &gpaSigQueuedPreAlloced[46], .enmHowFree = enmHowFree_PreAllocFree },    /* 45 */
    { .pNext = &gpaSigQueuedPreAlloced[47], .enmHowFree = enmHowFree_PreAllocFree },    /* 46 */
    { .pNext = &gpaSigQueuedPreAlloced[48], .enmHowFree = enmHowFree_PreAllocFree },    /* 47 */
    { .pNext = &gpaSigQueuedPreAlloced[49], .enmHowFree = enmHowFree_PreAllocFree },    /* 48 */
    { .pNext = &gpaSigQueuedPreAlloced[50], .enmHowFree = enmHowFree_PreAllocFree },    /* 49 */
    { .pNext = &gpaSigQueuedPreAlloced[51], .enmHowFree = enmHowFree_PreAllocFree },    /* 50 */
    { .pNext = &gpaSigQueuedPreAlloced[52], .enmHowFree = enmHowFree_PreAllocFree },    /* 51 */
    { .pNext = &gpaSigQueuedPreAlloced[53], .enmHowFree = enmHowFree_PreAllocFree },    /* 52 */
    { .pNext = &gpaSigQueuedPreAlloced[54], .enmHowFree = enmHowFree_PreAllocFree },    /* 53 */
    { .pNext = &gpaSigQueuedPreAlloced[55], .enmHowFree = enmHowFree_PreAllocFree },    /* 54 */
    { .pNext = &gpaSigQueuedPreAlloced[56], .enmHowFree = enmHowFree_PreAllocFree },    /* 55 */
    { .pNext = &gpaSigQueuedPreAlloced[57], .enmHowFree = enmHowFree_PreAllocFree },    /* 56 */
    { .pNext = &gpaSigQueuedPreAlloced[58], .enmHowFree = enmHowFree_PreAllocFree },    /* 57 */
    { .pNext = &gpaSigQueuedPreAlloced[59], .enmHowFree = enmHowFree_PreAllocFree },    /* 58 */
    { .pNext = &gpaSigQueuedPreAlloced[60], .enmHowFree = enmHowFree_PreAllocFree },    /* 59 */
    { .pNext = &gpaSigQueuedPreAlloced[61], .enmHowFree = enmHowFree_PreAllocFree },    /* 60 */
    { .pNext = &gpaSigQueuedPreAlloced[62], .enmHowFree = enmHowFree_PreAllocFree },    /* 61 */
    { .pNext = &gpaSigQueuedPreAlloced[63], .enmHowFree = enmHowFree_PreAllocFree },    /* 62 */
    { .pNext = NULL,                        .enmHowFree = enmHowFree_PreAllocFree },    /* 63 */
};

/** LIFO of free queued signal structures.
 * Queued by the pNext pointer. (These of course belongs to this process.)
 * All access to this array is protected by the signal semaphore.
 */
static PSIGQUEUED           gpSigQueuedFree = &gpaSigQueuedPreAlloced[0];
/** Number of structures in the free queue. */
static volatile unsigned    gcSigQueuedFree = sizeof(gpaSigQueuedPreAlloced) / sizeof(gpaSigQueuedPreAlloced[0]);

/** Flag indicating that this LIBC instance have installed the signal handler.
 * This is used during fork so that the child will behave like the parent. */
static int gfOS2V1Handler = 0;


/*******************************************************************************
*   Internal Functions                                                         *
*******************************************************************************/
static int              signalSchedule(__LIBC_PTHREAD pThrd, int iSignalNo, siginfo_t *pSigInfo, unsigned fFlags, PSIGQUEUED pSigQueued);
static void             signalScheduleSPM(__LIBC_PTHREAD pThrd);
static void             signalScheduleProcess(__LIBC_PTHREAD pThrd);
static void             signalScheduleToThread(__LIBC_PTHREAD pThrd, __LIBC_PTHREAD pThrdSig, int iSignalNo, PSIGQUEUED pSig);
static __LIBC_PTHREAD   signalScheduleThread(int iSignalNo, __LIBC_PTHREAD pThrdCur);
static int              signalScheduleThreadWorker(__LIBC_PTHREAD pCur, __LIBC_PTHREAD pBest, void *pvParam);
static void             signalScheduleKickThreads(__LIBC_PTHREAD pThrd);
static int              signalScheduleKickThreadsWorker(__LIBC_PTHREAD pCur, void *pvParam);
static int              signalDeliver(__LIBC_PTHREAD pThrd, int iSignalNo, void *pvXcptParams);
static void             signalTerminate(int iSignalNo) __dead2;
static void             signalTerminateAbnormal(int iSignalNo, void *pvXcptParams) __dead2;
static int              signalJobStop(int iSignalNo);
static int              signalJobResume(void);
static int              signalSendPGrpCallback(int iSignalNo, const __LIBC_SPMPROCESS *pProcess, void *pvUser);
static int              signalActionWorker(__LIBC_PTHREAD pCur, void *pvParam);
static unsigned         signalTimestamp(void);
static int              signalForkChild(__LIBC_PFORKHANDLE pForkHandle, __LIBC_FORKOP enmOperation);


/**
 * Initializes the signal handling for this process.
 *
 * This is called at DLL/LIBC init.
 *
 * @returns 0 on success.
 * @returns -1 on failure.
 */
int __libc_back_signalInit(void)
{
    LIBCLOG_ENTER("\n");
    int rc;

    /*
     * Create the semaphore.
     */
    rc = __libc_Back_safesemMtxCreate(&gmtxSignals, FALSE);
    if (!rc)
    {
        /*
         * Create the wait semaphore.
         */
        rc = DosCreateEventSemEx(NULL, &__libc_back_ghevWait, 0, 0);
        if (!rc)
        {
            /*
             * Process inherited signal setup.
             */
            __LIBC_PSPMINHERIT pInherit = __libc_spmInheritRequest();
            if (pInherit)
            {
                if (    pInherit->pSig
                    &&  pInherit->pSig->cb >= sizeof(__LIBC_SPMINHSIG)
                    &&  !__SIGSET_ISEMPTY(&pInherit->pSig->SigSetIGN))
                {
                    /* Ignored signals. */
                    int iSignalNo;
                    for (iSignalNo = 1; iSignalNo < __SIGSET_MAXSIGNALS; iSignalNo++)
                        if (__SIGSET_ISSET(&pInherit->pSig->SigSetIGN, iSignalNo) && iSignalNo != SIGCHLD)
                            gaSignalActions[iSignalNo].__sigaction_u.__sa_handler = SIG_IGN;
                }
                __libc_spmInheritRelease();
            }

            LIBCLOG_RETURN_INT(0);
        }
        else
            LIBC_ASSERTM_FAILED("DosCreateEventSemEx failed with rc=%d\n", rc);

        __libc_Back_safesemMtxClose(&gmtxSignals);
        gmtxSignals.hmtx = NULLHANDLE;
    }
    else
        LIBC_ASSERTM_FAILED("__libc_Back_safesemMtxCreate failed with rc=%d\n", rc);

    LIBCLOG_RETURN_INT(-1);
}


/**
 * Main/exe init - install exception and signals handlers, and set signal focus.
 *
 * @returns 0 on success.
 * @returns Negative error code (errno.h) on failure.
 * @param   pvRegRec    Pointer to the exception registartion record to use.
 */
int __libc_back_signalInitExe(void *pvRegRec)
{
    LIBCLOG_ENTER("\n");

    /*
     * Exception handler.
     */
    PEXCEPTIONREGISTRATIONRECORD pRegRec = (PEXCEPTIONREGISTRATIONRECORD)pvRegRec;
    pRegRec->prev_structure   = END_OF_CHAIN;
    pRegRec->ExceptionHandler = __libc_Back_exceptionHandler;
    int rc = DosSetExceptionHandler(pRegRec);

    /*
     * Signal handling.
     */
    rc = DosSetSigHandler((PFNSIGHANDLER)__libc_back_signalOS2V1Handler16bit, NULL, NULL, SIGA_ACCEPT, SIG_PFLG_A);
    if (!rc)
    {
        gfOS2V1Handler = 1;
        ULONG cTimes = 0;
        DosSetSignalExceptionFocus(SIG_SETFOCUS, &cTimes); /* (Will fail with rc 303 for PM apps - which we ignore.) */
        LIBCLOG_RETURN_INT(0);
    }
    LIBC_ASSERTM_FAILED("DosSetSigHandler failed with rc=%d\n", rc);
    rc = -__libc_native2errno(rc);
    LIBCLOG_ERROR_RETURN_INT(rc);
}


/**
 * Checks if the caller is the current semaphore owner.
 *
 * @returns Nesting level if owner.
 * @returns 0 if not owner.
 */
unsigned    __libc_back_signalSemIsOwner(void)
{
    FS_VAR()
    PTIB    pTib;
    PPIB    pPib;
    int     rc;
    PID     pid = 0;
    TID     tid = 0;
    ULONG   cNesting = 1;

    FS_SAVE_LOAD();
    DosGetInfoBlocks(&pTib, &pPib);

    rc = DosQueryMutexSem(gmtxSignals.hmtx, &pid, &tid, &cNesting);
    FS_RESTORE();
    LIBC_ASSERTM(!rc, "DosQueryMutexSem(%#lx,,,,) -> rc=%d\n", gmtxSignals.hmtx, rc);
    if (!rc && pTib->tib_ptib2->tib2_ultid == tid && pPib->pib_ulpid == pid)
        return cNesting ? cNesting : 1; /* paranoia */
    return 0;
}


/**
 * Requests the signal semaphore and enters a must complete section.
 *
 * @returns 0 if we got the semaphore.
 * @returns -1 if we didn't get it (pretty fatal).
 */
int __libc_back_signalSemRequest(void)
{
    LIBCLOG_ENTER("\n");
    int rc = __libc_Back_safesemMtxLock(&gmtxSignals);
    LIBCLOG_RETURN_INT(rc);
}


/**
 * Releases the signal semaphore and the must complete section.
 */
void __libc_back_signalSemRelease(void)
{
    LIBCLOG_ENTER("\n");
    __libc_Back_safesemMtxUnlock(&gmtxSignals);
    LIBCLOG_RETURN_VOID();
}


/**
 * Reschedule signals.
 * Typically called after a block mask have been updated.
 *
 * The caller must own the signal semaphore, this function will release the ownership!
 *
 * @returns On success a flag mask out of the __LIBC_BSRR_* #defines is returned.
 * @returns On failure a negative error code (errno.h) is returned.
 * @param   pThrd       Current thread.
 */
int __libc_back_signalReschedule(__LIBC_PTHREAD pThrd)
{
    LIBC_ASSERT(__libc_back_signalSemIsOwner());
    signalScheduleSPM(pThrd);
    signalScheduleProcess(pThrd);
    return signalDeliver(pThrd, 0, NULL);
}


/**
 * Raises a signal in the current process.
 *
 * @returns On success a flag mask out of the __LIBC_BSRR_* #defines is returned.
 * @returns On failure a negative error code (errno.h) is returned.
 * @param   iSignalNo           Signal to raise.
 * @param   pSigInfo            Pointer to signal info for this signal.
 *                              NULL is allowed.
 * @param   pvXcptOrQueued      Exception handler parameter list.
 *                              Or if __LIBC_BSRF_QUEUED is set, a pointer to locally malloced
 *                              SIGQUEUED node.
 * @param   fFlags              Flags of the #defines __LIBC_BSRF_* describing how to
 *                              deliver the signal.
 *
 * @remark  This Backend Signal API does NOT require the caller to own the signal semaphore.
 */
int __libc_back_signalQueueSelf(int iSignalNo, siginfo_t *pSigInfo)
{
    LIBCLOG_ENTER("iSignalNo=%d pSigInfo=%p\n", iSignalNo, (void *)pSigInfo);

    /*
     * Preallocate a queue entry to avoid entering and leaving the semaphore unecessary.
     */
    PSIGQUEUED pSigToFree = NULL;
    PSIGQUEUED pSig = _hmalloc(sizeof(*pSig));

    /*
     * Take sempahore.
     */
    int rc = __libc_back_signalSemRequest();
    if (!rc)
    {
        /*
         * Release heap memory accumulated in node
         */
        if (gcSigQueuedFree > sizeof(gpaSigQueuedPreAlloced) * 2 / sizeof(gpaSigQueuedPreAlloced[0]))
        {
            /*
             * No need for the preallocated one!
             */
            if (pSig)
                pSig->pNext = NULL;
            pSigToFree = pSig;

            /*
             * Unlink heap allocated nodes until we're below the waterline again
             */
            PSIGQUEUED pSigPrev = NULL;
            pSig = gpSigQueuedFree;
            int cToMany = gcSigQueuedFree - sizeof(gpaSigQueuedPreAlloced) * 2 / sizeof(gpaSigQueuedPreAlloced[0]);
            while (cToMany > 0)
            {
                if (pSig->enmHowFree == enmHowFree_HeapFree)
                {
                    /* unlink */
                    PSIGQUEUED pSigNext = pSig->pNext;
                    if (pSigPrev)
                        pSigPrev->pNext = pSigNext;
                    else
                        gpSigQueuedFree = pSigNext;

                    /* queue for free */
                    pSig->pNext = pSigToFree;
                    pSigToFree = pSig;

                    /* next */
                    pSig = pSigNext;
                }
                else
                {
                    pSigPrev = pSig;
                    pSig = pSig->pNext;
                }
            }

            pSig = NULL;
        }

        /*
         * Raise the signal the normal way.
         */
        rc = __libc_Back_signalRaise(iSignalNo, pSigInfo, pSig, __LIBC_BSRF_QUEUED);
        __libc_back_signalSemRelease();
        if (rc > 0)
            rc = 0;
    }

    /*
     * Cleanup and return.
     */
    while (pSigToFree)
    {
        void *pvToFree = pSigToFree;
        pSigToFree = pSigToFree->pNext;
        free(pvToFree);
    }
    if (rc >= 0)
        LIBCLOG_RETURN_INT(rc);
    LIBCLOG_ERROR_RETURN_INT(rc);
}


/**
 * Raises a signal in the current process.
 *
 * @returns On success a flag mask out of the __LIBC_BSRR_* #defines is returned.
 * @returns On failure a negative error code (errno.h) is returned.
 * @param   iSignalNo           Signal to raise.
 * @param   pSigInfo            Pointer to signal info for this signal.
 *                              NULL is allowed.
 * @param   pvXcptOrQueued      Exception handler parameter list.
 *                              Or if __LIBC_BSRF_QUEUED is set, a pointer to locally malloced
 *                              SIGQUEUED node.
 * @param   fFlags              Flags of the #defines __LIBC_BSRF_* describing how to
 *                              deliver the signal.
 */
int __libc_Back_signalRaise(int iSignalNo, const siginfo_t *pSigInfo, void *pvXcptOrQueued, unsigned fFlags)
{
    LIBCLOG_ENTER("iSignalNo=%d pSigInfo=%p{.si_signo=%d, .si_errno=%d, .si_code=%#x, .si_timestamp=%#x, .si_flags=%#x .si_pid=%#x, .si_pgrp=%#x, .si_tid=%#x, .si_uid=%d, .si_status=%d, .si_addr=%p, .si_value=%p, .si_band=%ld, .si_fd=%d} pvXcptOrQueued=%p fFlags=%#x\n",
                  iSignalNo, (void *)pSigInfo,
                  pSigInfo ? pSigInfo->si_signo : 0,
                  pSigInfo ? pSigInfo->si_errno : 0,
                  pSigInfo ? pSigInfo->si_code : 0,
                  pSigInfo ? pSigInfo->si_timestamp : 0,
                  pSigInfo ? pSigInfo->si_flags : 0,
                  pSigInfo ? pSigInfo->si_pid : 0,
                  pSigInfo ? pSigInfo->si_pgrp : 0,
                  pSigInfo ? pSigInfo->si_tid : 0,
                  pSigInfo ? pSigInfo->si_uid : 0,
                  pSigInfo ? pSigInfo->si_status : 0,
                  pSigInfo ? pSigInfo->si_addr : 0,
                  pSigInfo ? pSigInfo->si_value.sigval_ptr : 0,
                  pSigInfo ? pSigInfo->si_band : 0,
                  pSigInfo ? pSigInfo->si_fd : 0,
                  (void *)pvXcptOrQueued, fFlags);

    /*
     * Can't be too careful!
     */
    LIBC_ASSERTM(!__libc_back_signalSemIsOwner(), "Thread owns the signal semaphore!!! Bad boy!!\n");
    if (!__SIGSET_SIG_VALID(iSignalNo) || iSignalNo == 0)
        LIBCLOG_ERROR_RETURN(-EINVAL, "ret -EINVAL - Invalid signal %d\n", iSignalNo);

    /*
     * Get the current thread and update the last signal timestamp.
     * There *MUST* be a current thread, if not, we're toast!
     */
    __LIBC_PTHREAD  pThrd = __libc_threadCurrentNoAuto();
    if (!pThrd)
    {
        if (fFlags & (__LIBC_BSRF_EXTERNAL | __LIBC_BSRF_HARDWARE))
        {
            LIBC_ASSERTM_FAILED("No thread structure and we cannot safely create one!\n");
            int rc = __LIBC_BSRR_PASSITON | (__SIGSET_ISSET(&__libc_gSignalRestartMask, iSignalNo) ? __LIBC_BSRR_RESTART : __LIBC_BSRR_INTERRUPT);
            LIBCLOG_ERROR_RETURN_INT(rc);
        }
        pThrd = __libc_threadCurrent();
        if (!pThrd)
        {
            LIBC_ASSERTM_FAILED("Failed to get thread structure!\n");
            LIBCLOG_ERROR_RETURN_INT(-ENOMEM);
        }
    }
    pThrd->ulSigLastTS = fibGetMsCount();

    /*
     * Copy the siginfo structure and fill in the rest of
     * the SigInfo packet (if we've got one).
     */
    siginfo_t SigInfo;
    if (pSigInfo)
    {
        SigInfo = *pSigInfo;
        if (!SigInfo.si_timestamp)
            SigInfo.si_timestamp = signalTimestamp();
        if (fFlags & __LIBC_BSRF_QUEUED)
            SigInfo.si_flags |= __LIBC_SI_QUEUED;
        if (!SigInfo.si_pid)
            SigInfo.si_pid = _sys_pid;
        if (!SigInfo.si_tid && pThrd)
            SigInfo.si_tid = pThrd->tid;
    }

    /*
     * Take the semaphore.
     */
    if (__libc_back_signalSemRequest())
    {
        /* we're toast. */
        static const char szMsg[] = "\r\nLIBC: internal error!! signal sem is dead/deadlocked!!!\n\r";
        ULONG   cb;
        LIBCLOG_MSG("we're toast!\n");
        DosWrite(HFILE_STDERR, szMsg, sizeof(szMsg) - 1, &cb);
        signalTerminateAbnormal(iSignalNo, fFlags & __LIBC_BSRF_QUEUED ? NULL : pvXcptOrQueued);
    }

    /*
     * Does this thread have a notification callback?
     */
    if (pThrd->pfnSigCallback)
        pThrd->pfnSigCallback(iSignalNo, pThrd->pvSigCallbackUser);

    /*
     * Schedule the signal.
     */
    int rc = signalSchedule(pThrd, iSignalNo, pSigInfo ? &SigInfo : NULL, fFlags, fFlags & __LIBC_BSRF_QUEUED ? pvXcptOrQueued : NULL);
    if (rc >= 0)
    {
        /*
         * Paranoia scheduling of the process.
         */
        signalScheduleSPM(pThrd);
        signalScheduleProcess(pThrd);

        /*
         * Deliver signals pending on this thread.
         */
        int rc = signalDeliver(pThrd, fFlags & __LIBC_BSRF_HARDWARE ? iSignalNo : 0,  fFlags & __LIBC_BSRF_QUEUED ? NULL : pvXcptOrQueued);
        if (rc >= 0 && __SIGSET_ISSET(&__libc_gSignalRestartMask, iSignalNo))
            rc = (rc & ~(__LIBC_BSRR_INTERRUPT)) | __LIBC_BSRR_RESTART;
    }
    else
        __libc_back_signalSemRelease();

    return rc;
}


/**
 * Schedules a signal generated by an internal worker thread.
 *
 * @returns 0 on success.
 * @returns On failure a negative error code (errno.h) is returned.
 * @param   pThrd               The current thread.
 * @param   iSignalNo           Signal to raise.
 * @param   pSigInfo            Pointer to signal info for this signal.
 *                              NULL is allowed.
 * @param   pvQueued            If __LIBC_BSRF_QUEUED is set, can optionally point to
 *                              locally malloced SIGQUEUED node.
 * @param   fFlags              Flags of the #defines __LIBC_BSRF_* describing how to
 *                              deliver the signal.
 */
int __libc_back_signalRaiseInternal(__LIBC_PTHREAD pThrd, int iSignalNo, const siginfo_t *pSigInfo, void *pvQueued, unsigned fFlags)
{
    LIBCLOG_ENTER("pThrd=%p{.tid=%#x} iSignalNo=%d pSigInfo=%p{.si_signo=%d, .si_errno=%d, .si_code=%#x, .si_timestamp=%#x, .si_flags=%#x .si_pid=%#x, .si_pgrp=%#x, .si_tid=%#x, .si_uid=%d, .si_status=%d, .si_addr=%p, .si_value=%p, .si_band=%ld, .si_fd=%d} pvQueued=%p fFlags=%#x\n",
                  (void *)pThrd, pThrd->tid,
                  iSignalNo, (void *)pSigInfo,
                  pSigInfo ? pSigInfo->si_signo : 0,
                  pSigInfo ? pSigInfo->si_errno : 0,
                  pSigInfo ? pSigInfo->si_code : 0,
                  pSigInfo ? pSigInfo->si_timestamp : 0,
                  pSigInfo ? pSigInfo->si_flags : 0,
                  pSigInfo ? pSigInfo->si_pid : 0,
                  pSigInfo ? pSigInfo->si_pgrp : 0,
                  pSigInfo ? pSigInfo->si_tid : 0,
                  pSigInfo ? pSigInfo->si_uid : 0,
                  pSigInfo ? pSigInfo->si_status : 0,
                  pSigInfo ? pSigInfo->si_addr : 0,
                  pSigInfo ? pSigInfo->si_value.sigval_ptr : 0,
                  pSigInfo ? pSigInfo->si_band : 0,
                  pSigInfo ? pSigInfo->si_fd : 0,
                  (void *)pvQueued, fFlags);

    /*
     * Can't be to careful!
     */
    LIBC_ASSERT(__libc_back_signalSemIsOwner());
    LIBC_ASSERT(pThrd->fInternalThread);
    if (!__SIGSET_SIG_VALID(iSignalNo) || iSignalNo == 0)
    {
        LIBC_ASSERTM_FAILED("Invalid signal %d\n", iSignalNo);
        LIBCLOG_ERROR_RETURN_INT(-EINVAL);
    }

    /*
     * Copy the siginfo structure and fill in the rest of
     * the SigInfo packet (if we've got one).
     */
    siginfo_t SigInfo;
    SigInfo = *pSigInfo;
    if (!SigInfo.si_timestamp)
        SigInfo.si_timestamp = signalTimestamp();
    if (fFlags & __LIBC_BSRF_QUEUED)
        SigInfo.si_flags |= __LIBC_SI_QUEUED;
    if (!SigInfo.si_pid)
        SigInfo.si_pid = _sys_pid;
    if (!SigInfo.si_tid)
        SigInfo.si_tid = pThrd->tid;

    /*
     * Schedule the signal.
     */
    int rc = signalSchedule(pThrd, iSignalNo, &SigInfo, fFlags, fFlags & __LIBC_BSRF_QUEUED ? pvQueued : NULL);

    if (rc >= 0)
        LIBCLOG_RETURN_INT(rc);
    LIBCLOG_ERROR_RETURN_INT(rc);
}



/**
 * Schedules a signal to the appropriate thread or leave it pending on the process.
 *
 * @returns 0 on success
 * @returns Negative error code (errno.h) on failure.
 * @param   pThrd               Current thread.
 * @param   iSignalNo           Signal number of the signal to be scheduled.
 * @param   pSigInfo            Pointer to signal info for this signal.
 *                              NULL is allowed.
 * @param   fFlags              Flags of the #defines __LIBC_BSRF_* describing how to
 *                              deliver the signal.
 * @param   pSigQueued          A pointer to locally malloced SIGQUEUED node. NULL is allowed and common.
 */
static int signalSchedule(__LIBC_PTHREAD pThrd, int iSignalNo, siginfo_t *pSigInfo, unsigned fFlags, PSIGQUEUED pSigQueued)
{
    LIBCLOG_ENTER("pThrd=%p iSignalNo=%d pSigInfo=%p fFlags=%#x pSigQueued=%p\n",
                  (void *)pThrd, iSignalNo, (void *)pSigInfo, fFlags, (void *)pSigQueued);

    LIBC_ASSERTM(__libc_back_signalSemIsOwner(), "Thread doesn't own the signal semaphore!!! Bad boy!!\n");

    /*
     * If we're ignoring the signal we just eat it up and return immediately.
     */
    if (    gaSignalActions[iSignalNo].__sigaction_u.__sa_handler == SIG_IGN
        ||  (   gaSignalActions[iSignalNo].__sigaction_u.__sa_handler == SIG_DFL
             && (gafSignalProperties[iSignalNo] & SPA_MASK) == SPA_IGNORE)
        ||  (   iSignalNo == SIGCHLD
             && pSigInfo
             && (gaSignalActions[SIGCHLD].sa_flags & SA_NOCLDSTOP)
             && (pSigInfo->si_flags & __LIBC_SI_INTERNAL)
             && (pSigInfo->si_code == CLD_STOPPED || pSigInfo->si_code == CLD_CONTINUED))
        )
    {
        /* free any preallocated queued node. */
        if (pSigQueued)
        {
            pSigQueued->pNext = gpSigQueuedFree;
            gpSigQueuedFree = pSigQueued->pNext;
            gcSigQueuedFree++;
        }

        LIBCLOG_MSG("Ignoring signal %d\n", iSignalNo);
        LIBCLOG_RETURN_INT(0);
    }


    /*
     * Try schedule the signal for a thread.
     */
    __LIBC_PTHREAD  pThrdSig = NULL;
    if (    (fFlags & (__LIBC_BSRF_THREAD | __LIBC_BSRF_HARDWARE))
        || !(gafSignalProperties[iSignalNo] & SPP_ANYTHRD))
    {
        /*
         * Must be scheduled for the current thread.
         */
        pThrdSig = pThrd;
    }
    else if (gafSignalProperties[iSignalNo] & SPP_THRDONE)
    {
        /*
         * Must schedule this signal on thread one.
         */
        pThrdSig = __libc_threadLookup(1);
    }
    else
    {
        /*
         * Schedule for any thread in the process (but obviously
         * prefering the current thread when ever possible).
         */
         pThrdSig = signalScheduleThread(iSignalNo, pThrd);
    }

    int rc = 0;
    if (pThrdSig)
    {
        /*
         * Was it waited for?
         */
        if (   pThrdSig->enmStatus == enmLIBCThreadStatus_sigwait
            && __SIGSET_ISSET(&pThrdSig->u.pSigWait->SigSetWait, iSignalNo))
        {
            /*
             * Wake up a sigwait call.
             */
            if (pSigInfo)
                pThrdSig->u.pSigWait->SigInfo = *pSigInfo;
            else
                bzero((void *)&pThrdSig->u.pSigWait->SigInfo, sizeof(pThrdSig->u.pSigWait->SigInfo));
            pThrdSig->u.pSigWait->SigInfo.si_signo = iSignalNo;
            pThrdSig->u.pSigWait->fDone = 1;
            pThrdSig->enmStatus = enmLIBCThreadStatus_unknown;

            /*
             * Check if there is an unqueued pending signal which we should clear.
             */
            PSIGQUEUED pSig = pThrdSig->SigQueue.pHead;
            while (pSig && pSig->SigInfo.si_signo != iSignalNo)
                pSig = pSig->pNext;
            if (!pSig)
                __SIGSET_CLEAR(&pThrdSig->SigSetPending, iSignalNo);
            else
                __SIGSET_SET(&pThrdSig->SigSetPending, iSignalNo);

            /*
             * Poke the thread.
             */
            if (pThrdSig != pThrd)
                __libc_back_signalPokeThread(pThrdSig);
            LIBCLOG_MSG("wokeup sigwait in thread %#x on signal %d.\n", pThrdSig->tid, iSignalNo);
        }
        else
        {
            /*
             * Not waited for, so just queue it if possible and mark it pending.
             */
            if (    pSigInfo
                ||  (fFlags & __LIBC_BSRF_QUEUED)
                ||  (gafSignalProperties[iSignalNo] & SPP_QUEUED))
            {
                /*
                 * Queue the signal.
                 */
                if (!pSigQueued)
                {   /* 'allocate' it. */
                    pSigQueued = gpSigQueuedFree;
                    if (pSigQueued)
                        gpSigQueuedFree = pSigQueued->pNext;
                }

                /* got a sigqueued node? */
                if (pSigQueued)
                {
                    if (pSigInfo)
                        pSigQueued->SigInfo = *pSigInfo;
                    else
                        bzero(pSigQueued, sizeof(*pSigQueued));
                    pSigQueued->SigInfo.si_signo = iSignalNo;

                    /* insert into the queue, give hardware and kill priority (see signalDeliver()). */
                    if (    (fFlags & __LIBC_BSRF_HARDWARE)
                        ||  iSignalNo == SIGKILL)
                    {   /* insert at head */
                        pSigQueued->pPrev = NULL;
                        pSigQueued->pNext = pThrdSig->SigQueue.pHead;
                        if (pThrdSig->SigQueue.pHead)
                            pThrdSig->SigQueue.pHead->pPrev = pSigQueued;
                        else
                            pThrdSig->SigQueue.pTail = pSigQueued;
                        pThrdSig->SigQueue.pHead = pSigQueued;
                    }
                    else
                    {   /* insert at tail */
                        pSigQueued->pNext = NULL;
                        pSigQueued->pPrev = pThrdSig->SigQueue.pTail;
                        if (pThrdSig->SigQueue.pTail)
                            pThrdSig->SigQueue.pTail->pNext = pSigQueued;
                        else
                            pThrdSig->SigQueue.pHead = pSigQueued;
                        pThrdSig->SigQueue.pTail = pSigQueued;
                    }
                    LIBCLOG_MSG("enqueued signal %d.\n", iSignalNo);
                }
                /*
                 * No available nodes - ignore this situation if queueing isn't compulsory.
                 */
                else if (   (fFlags & __LIBC_BSRF_QUEUED)
                         || (gafSignalProperties[iSignalNo] & SPP_QUEUED))
                {
                    LIBC_ASSERTM_FAILED("Out of queue nodes! iSignalNo=%d\n", iSignalNo);
                    rc = -EAGAIN;
                }
            }

            /*
             * Set pending and poke the thread (if it's not ourself).
             */
            if (!rc)
            {
                __SIGSET_SET(&pThrdSig->SigSetPending, iSignalNo);
                if (pThrdSig != pThrd)
                    __libc_back_signalPokeThread(pThrdSig);
                LIBCLOG_MSG("setting %d pending on thread %#x and poking it.\n", iSignalNo, pThrdSig->tid);
            }
        }
    }
    else
    {
        /*
         * No thread ready for it, leave the signal on process level.
         * Queue it if required and then mark it pending.
         */
        if (    pSigInfo
            ||  (fFlags & __LIBC_BSRF_QUEUED)
            ||  (gafSignalProperties[iSignalNo] & SPP_QUEUED))
        {
            /*
             * Queue the signal.
             */
            if (!pSigQueued)
            {   /* 'allocate' it. */
                pSigQueued = gpSigQueuedFree;
                if (pSigQueued)
                    gpSigQueuedFree = pSigQueued->pNext;
            }

            /* got a sigqueued node? */
            if (pSigQueued)
            {
                if (pSigInfo)
                    pSigQueued->SigInfo = *pSigInfo;
                else
                    bzero(pSigQueued, sizeof(*pSigQueued));
                pSigQueued->SigInfo.si_signo = iSignalNo;

                /* insert into the queue */
                pSigQueued->pNext = NULL;
                pSigQueued->pPrev = gpSigQueueTail;
                if (gpSigQueueTail)
                    gpSigQueueTail = gpSigQueueTail->pNext = pSigQueued;
                else
                    gpSigQueueTail = gpSigQueueHead = pSigQueued;
                LIBCLOG_MSG("enqueued signal %d.\n", iSignalNo);
            }
            /*
             * No available nodes - ignore this situation if queueing isn't compulsory.
             */
            else if (   (fFlags & __LIBC_BSRF_QUEUED)
                     || (gafSignalProperties[iSignalNo] & SPP_QUEUED))
            {
                rc = -EAGAIN;
                LIBC_ASSERTM_FAILED("Out of queue nodes! iSignalNo=%d\n", iSignalNo);
            }
        }

        /*
         * Add to the pending signals of this process.
         */
        if (rc >= 0)
        {
            __SIGSET_SET(&__libc_gSignalPending, iSignalNo);
            LIBCLOG_MSG("adding %d to pending signals of the current process.\n", iSignalNo);
        }
    }

    return rc;
}


/**
 * Checks if we can schedule any of the signals queued on the process in SPM.
 *
 * This means discarding (when ignored) or moving signals from pending at 1st
 * level to 3rd level (thread). signalSchedule() is used to do the actual
 * thead scheduling of the signals, which means that the caller must check
 * the current thread for any deliverable signals upon return.
 *
 * @param   pThrd   Current thread.
 */
static void signalScheduleSPM(__LIBC_PTHREAD pThrd)
{
    LIBCLOG_ENTER("pThrd=%p\n", (void *)pThrd);
    LIBC_ASSERTM(__libc_back_signalSemIsOwner(), "Thread doesn't own the signal semaphore!!! Bad boy!!\n");

    /*
     * Get the mask of pending signals.
     */
    sigset_t    SigSetPending;
    int         cSignals = __libc_spmSigPending(&SigSetPending);

    /*
     * Iterate thru the pending signals and see if any of
     * them can be delivered.
     */
    int         iSignalNo = 1;
    while (cSignals > 0 && iSignalNo < __SIGSET_MAXSIGNALS)
    {
        if (__SIGSET_ISSET(&SigSetPending, iSignalNo))
        {
            /*
             * Let's see if we can ignore it or successfully schedule it to a thread,
             * or if it's SIGCHLD which might be required to get wait*() working right.
             */
            __LIBC_PTHREAD pThrdSig = NULL;
            if (    gaSignalActions[iSignalNo].__sigaction_u.__sa_handler == SIG_IGN
                ||  (   gaSignalActions[iSignalNo].__sigaction_u.__sa_handler == SIG_DFL
                     && (gafSignalProperties[iSignalNo] & SPA_MASK) == SPA_IGNORE)
                || iSignalNo == SIGCHLD
                || (pThrdSig = signalScheduleThread(iSignalNo, pThrd)))
            {
                /*
                 * Fetch signals of this type.
                 */
                siginfo_t SigInfo;
                while (__libc_spmSigDequeue(iSignalNo, &SigInfo, 1, sizeof(SigInfo)) > 0)
                {
                    int rc = signalSchedule(pThrd, iSignalNo, &SigInfo,
                                            __LIBC_BSRF_EXTERNAL | ((SigInfo.si_flags & __LIBC_SI_QUEUED) ? __LIBC_BSRF_QUEUED : 0),
                                            NULL);
                    if (rc >= 0)
                    {
                        /*
                         * Special handling of child notifications.
                         */
                        if (    iSignalNo == SIGCHLD
                            &&  (SigInfo.si_flags & (__LIBC_SI_INTERNAL | __LIBC_SI_NO_NOTIFY_CHILD)) == __LIBC_SI_INTERNAL)
                            __libc_back_processWaitNotifyChild(&SigInfo);

                        cSignals--;
                    }
                    else
                    {
                        /* Shit! We failed to queue it on a thread. Try put it back in the 1st level queue. */
                        LIBCLOG_MSG("failed to schedule signal %d for any thread!\n", iSignalNo);
                        __libc_spmSigQueue(SigInfo.si_signo, &SigInfo, _sys_pid, !!(SigInfo.si_flags & __LIBC_SI_QUEUED));
                        break;
                    }
                }
            }
        }

        /* next */
        iSignalNo++;
    }
    LIBCLOG_RETURN_VOID();
}


/**
 * Schedules all signals currently pending on the process (2nd level).
 *
 * @param   pThrd   Current Thread.
 */
static void signalScheduleProcess(__LIBC_PTHREAD pThrd)
{
    LIBCLOG_ENTER("pThrd=%p\n", (void *)pThrd);
    LIBC_ASSERTM(__libc_back_signalSemIsOwner(), "Thread doesn't own the signal semaphore!!! Bad boy!!\n");

    /*
     * Process queued signals first (to get a more correct order).
     */
    sigset_t    SigSetDone;
    __SIGSET_EMPTY(&SigSetDone);
    PSIGQUEUED pSig = gpSigQueueHead;
    while (pSig)
    {
        int iSignalNo = pSig->SigInfo.si_signo;
        if (!__SIGSET_ISSET(&SigSetDone, iSignalNo))
        {
            __LIBC_PTHREAD pThrdSig = signalScheduleThread(iSignalNo, pThrd);
            if (!pThrdSig)
                __SIGSET_SET(&SigSetDone, iSignalNo);
            else
            {
                /* unlink */
                PSIGQUEUED pSigNext = pSig->pNext;
                if (pSig->pPrev)
                    pSig->pPrev->pNext = pSigNext;
                else
                    gpSigQueueHead     = pSigNext;
                if (pSigNext)
                    pSig->pNext->pPrev = pSig->pPrev;
                else
                    gpSigQueueTail     = pSig->pPrev;

                /* schedule */
                signalScheduleToThread(pThrd, pThrdSig, iSignalNo, pSig);
                __SIGSET_CLEAR(&__libc_gSignalPending, iSignalNo);
                __libc_threadDereference(pThrdSig);

                /* next */
                pSig = pSigNext;
                continue;
            }
        }

        /* next */
        pSig = pSig->pNext;
    }

    /*
     * Process pending signals (which was not queued).
     */
    sigset_t    SigSetTodo = __libc_gSignalPending;
    if (!__SIGSET_ISEMPTY(&SigSetTodo))
    {
        __SIGSET_NOT(&SigSetDone);
        __SIGSET_AND(&SigSetTodo, &SigSetTodo, &SigSetDone);
        if (!__SIGSET_ISEMPTY(&SigSetTodo))
        {
            int iSignalNo = 0;
            while (++iSignalNo < __SIGSET_MAXSIGNALS)
            {
                if (__SIGSET_ISSET(&SigSetTodo, iSignalNo))
                {
                    __LIBC_PTHREAD pThrdSig = signalScheduleThread(iSignalNo, pThrd);
                    if (pThrdSig)
                    {
                        signalScheduleToThread(pThrd, pThrdSig, iSignalNo, NULL);
                        __SIGSET_CLEAR(&__libc_gSignalPending, iSignalNo);
                        __libc_threadDereference(pThrdSig);
                    }
                }
            }
        }
    }

    /*
     * Now kick all thread with pending signals which can be delivered.
     */
    signalScheduleKickThreads(pThrd);
    LIBCLOG_RETURN_VOID();
}


/**
 * Schedules a signal to a thread and notifies the thread.
 *
 * @param   pThrd       Current thread.
 * @param   pThrdSig    Thread to schedule the signal to.
 * @param   iSignalNo   Signal to schedule.
 * @param   pSig        Signal queue node containing more info about the signal. (Optional)
 */
static void signalScheduleToThread(__LIBC_PTHREAD pThrd, __LIBC_PTHREAD pThrdSig, int iSignalNo, PSIGQUEUED pSig)
{
    LIBCLOG_ENTER("pThrd=%p pThrdSig=%p {.tid=%#x} iSignalNo=%d pSig=%p\n", (void *)pThrd, (void *)pThrdSig, pThrdSig->tid, iSignalNo, (void *)pSig);

    /*
     * Check if this was a sig*wait* case.
     */
    if (   pThrdSig->enmStatus == enmLIBCThreadStatus_sigwait
        && __SIGSET_ISSET(&pThrdSig->u.pSigWait->SigSetWait, iSignalNo))
    {
        /*
         * Store result with the thread.
         */
        if (pSig)
            pThrdSig->u.pSigWait->SigInfo = pSig->SigInfo;
        else
            bzero((void *)&pThrdSig->u.pSigWait->SigInfo, sizeof(pThrdSig->u.pSigWait->SigInfo));
        pThrdSig->u.pSigWait->SigInfo.si_signo = iSignalNo;
        pThrdSig->u.pSigWait->fDone = 1;
        pThrdSig->enmStatus = enmLIBCThreadStatus_unknown;

        /*
         * Free the signal node.
         */
        pSig->pNext = gpSigQueuedFree;
        gpSigQueuedFree = pSig->pNext;
        gcSigQueuedFree++;

        /*
         * Check if there is an unqueued pending signal which we should clear.
         */
        pSig = pThrdSig->SigQueue.pHead;
        while (pSig && pSig->SigInfo.si_signo != iSignalNo)
            pSig = pSig->pNext;
        if (!pSig)
            __SIGSET_CLEAR(&pThrdSig->SigSetPending, iSignalNo);
        else
            __SIGSET_SET(&pThrdSig->SigSetPending, iSignalNo);

        /*
         * Poke the thread.
         */
        if (pThrdSig != pThrd)
            __libc_back_signalPokeThread(pThrdSig);
        LIBCLOG_MSG("wokeup sigwait in thread %#x on signal %d.\n", pThrdSig->tid, iSignalNo);
    }
    else
    {
        /*
         * If we've got a queue node, queue it.
         */
        if (pSig)
        {
            pSig->pNext = NULL;
            pSig->pPrev = pThrdSig->SigQueue.pTail;
            if (pThrdSig->SigQueue.pTail)
                pThrdSig->SigQueue.pTail->pNext = pSig;
            else
                pThrdSig->SigQueue.pHead = pSig;
            pThrdSig->SigQueue.pTail = pSig;
        }

        /*
         * And in any case set it pending.
         */
        __SIGSET_SET(&pThrd->SigSetPending, iSignalNo);

        /*
         * Poke the thread.
         */
        if (pThrdSig != pThrd)
            __libc_back_signalPokeThread(pThrdSig);
        LIBCLOG_MSG("setting %d pending on thread %#x and poking it.\n", iSignalNo, pThrdSig->tid);
    }

    LIBCLOG_RETURN_VOID();
}


/**
 * Schedules the signal on a suitable thread.
 *
 * @returns Pointer to a thread structure (referenced).
 * @returns NULL if no thread can handle the signal.
 * @param   iSignalNo   Signal number in question.
 * @param   pThrdCur    Pointer to the current thread.
 */
__LIBC_PTHREAD  signalScheduleThread(int iSignalNo, __LIBC_PTHREAD pThrdCur)
{
    LIBC_ASSERTM(__libc_back_signalSemIsOwner(), "Thread does not own the signal semaphore!!!\n");

    /*
     * Check for sigwait in the current thread.
     */
    if (   pThrdCur->enmStatus == enmLIBCThreadStatus_sigwait
        && __SIGSET_ISSET(&pThrdCur->u.pSigWait->SigSetWait, iSignalNo))
    {
        pThrdCur->cRefs++;
        return pThrdCur;
    }

    /*
     * Enumerate all thread in the process and figure out which on
     * is the best suited to receive the signal.
     */
    SIGSCHEDENUMPARAM   EnumParam;
    EnumParam.iSignalNo = iSignalNo;
    EnumParam.pThrd     = pThrdCur;
    return __libc_threadLookup2(signalScheduleThreadWorker, &EnumParam);
}


/**
 * Thread enumerator which selects the best thread for a given signal.
 *
 * @returns 1 if the current thread should be returned.
 * @returns 2 if the current thread should be returned immediately.
 * @returns 0 if the current best thread should remain unchanged.
 * @returns -1 if the enumeration should fail.
 * @param   pCur        The current thread.
 * @param   pBest       The current best thread.
 * @param   pvParam     User parameters (PSIGSCHEDENUMPARAM in our case).
 */
static int signalScheduleThreadWorker(__LIBC_PTHREAD pCur, __LIBC_PTHREAD pBest, void *pvParam)
{
    PSIGSCHEDENUMPARAM pParam = (PSIGSCHEDENUMPARAM)pvParam;

    /*
     * Skip internal threads.
     */
    if (pCur->fInternalThread)
        return 0;

    /*
     * Look for sig*wait*().
     */
    if (   pCur->enmStatus == enmLIBCThreadStatus_sigwait
        && __SIGSET_ISSET(&pCur->u.pSigWait->SigSetWait, pParam->iSignalNo))
    {
        if (pCur == pParam->pThrd)
            return 2;                   /* we're done. */
        return 1;                       /* the last wait thread gets it... */
    }

    /*
     * Skip anyone who blocks the signal.
     */
    if (__SIGSET_ISSET(&pCur->SigSetBlocked, pParam->iSignalNo))
        return 0;

    /*
     * Not blocked, check if we've got a better choice.
     * (Better means cur thread, or a sig*wait*() thread.)
     */
    if (    pParam->pThrd == pBest
        ||  (   pBest
             && pBest->enmStatus == enmLIBCThreadStatus_sigwait
             && __SIGSET_ISSET(&pBest->u.pSigWait->SigSetWait, pParam->iSignalNo)))
        return 0;
    /* ok, it's not blocking it so it's ok to use it. */
    return 1;
}


/**
 * Enumerates all the threads in the process and poke the ones
 * which have signals that can be process.
 *
 * @param   pThrd   Current thread.
 */
static void signalScheduleKickThreads(__LIBC_PTHREAD pThrd)
{
    __libc_threadEnum(signalScheduleKickThreadsWorker, (void *)pThrd);
}


/**
 * Thread enumeration worker for signalScheduleKickThreads.
 *
 * It poke threads which have signals that can be delivered.
 *
 * @returns 0.
 * @param   pCur        The current thread in the enumration.
 * @param   pvParam     This thread.
 */
static int signalScheduleKickThreadsWorker(__LIBC_PTHREAD pCur, void *pvParam)
{
    /*
     * Skip internal threads.
     */
    if (pCur->fInternalThread)
        return 0;

    /*
     * Walk the queued signals and make sure they're pending.
     * (Self check.)
     */
    PSIGQUEUED pSig = pCur->SigQueue.pHead;
    while (pSig)
    {
        if (!__SIGSET_ISSET(&pCur->SigSetPending, pSig->SigInfo.si_signo))
        {
            LIBC_ASSERTM_FAILED("Signal %d is queued but not marked pending on thread %#x!\n", pSig->SigInfo.si_signo, pCur->tid);
            __SIGSET_SET(&pCur->SigSetPending, pSig->SigInfo.si_signo);
        }

        /* next */
        pSig = pSig->pNext;
    }

    /*
     * Skip further processing of threads which have been poked
     * or if if it's the current thread
     */
    if (    pCur->fSigBeingPoked
        ||  pCur == (__LIBC_PTHREAD)pvParam)
        return 0;

    /*
     * See if there are pending signals that can be delivered.
     */
    sigset_t    SigSetPending = pCur->SigSetBlocked;
    __SIGSET_NOT(&SigSetPending);
    __SIGSET_AND(&SigSetPending, &SigSetPending, &pCur->SigSetPending);
    if (!__SIGSET_ISEMPTY(&SigSetPending))
        __libc_back_signalPokeThread(pCur);

    return 0;
}


/**
 * Deliver signals pending on the current thread.
 *
 * Caller must own the semaphore before calling, this function
 * will release the semaphore no matter what happens!
 *
 * @returns On success a flag mask out of the __LIBC_BSRR_* #defines is returned.
 * @returns On failure a negative error code (errno.h) is returned.
 * @param   pThrd               Current thread.
 * @param   iSignalNo           Deliver the first signal of this type no matter if it's
 *                              blocked or not. This is for use with hardware signals only!
 * @param   pvXcptParams        Pointer to exception parameter list is any.
 */
static int signalDeliver(__LIBC_PTHREAD pThrd, int iSignalNo, void *pvXcptParams)
{
    LIBCLOG_ENTER("pThrd=%p pvXcptParams=%p\n", (void *)pThrd, pvXcptParams);
    int rcRet = __LIBC_BSRR_CONTINUE | __LIBC_BSRR_INTERRUPT;

    /*
     * Assert that we do own the semaphore upon entry.
     */
    LIBC_ASSERTM(__libc_back_signalSemIsOwner(), "Thread does not own the signal semaphore!!!\n");

    for (;;)
    {
        /*
         * Anything pending?
         */
        sigset_t    SigDeliver = pThrd->SigSetBlocked;
        __SIGSET_NOT(&SigDeliver);
        __SIGSET_AND(&SigDeliver, &SigDeliver, &pThrd->SigSetPending);
        if (__SIGSET_ISEMPTY(&SigDeliver))
            break;

        /*
         * Accept the signal.
         */
        siginfo_t SigInfo;
        iSignalNo = __libc_back_signalAccept(pThrd, iSignalNo, &SigDeliver, &SigInfo);
        if (!__SIGSET_SIG_VALID(iSignalNo))
        {
            LIBC_ASSERTM_FAILED("iSignalNo=%d\n", iSignalNo);
            break;
        }
        struct sigaction SigAction = gaSignalActions[iSignalNo];

        /*
         * Check if the thread is waiting in a sig*wait*, because in that
         * case we'll have to notify it that it have to return EINTR.
         */
        if (pThrd->enmStatus == enmLIBCThreadStatus_sigwait)
        {
            bzero((void *)&pThrd->u.pSigWait->SigInfo, sizeof(pThrd->u.pSigWait->SigInfo));
            pThrd->u.pSigWait->fDone = 1;
            pThrd->u.pSigWait = NULL;
            pThrd->enmStatus = enmLIBCThreadStatus_unknown;
        }
        /*
         * Check if the thread is waiting in a sigsuspend, because in that
         * case we'll have to tell it that the waiting is over.
         */
        else if (   pThrd->enmStatus == enmLIBCThreadStatus_sigsuspend
                 && SigAction.__sigaction_u.__sa_handler != SIG_IGN
                 && (gafSignalProperties[iSignalNo] & SPA_MASK) != SPA_STOP
                 && (gafSignalProperties[iSignalNo] & SPA_MASK) != SPA_STOPTTY
                 /*&& (gafSignalProperties[iSignalNo] & SPA_MASK) != SPA_NEXT ??*/
                 /*&& (gafSignalProperties[iSignalNo] & SPA_MASK) != SPA_RESUME ??*/
                 )
        {
            if (pThrd->u.pSigSuspend)
            {
                pThrd->u.pSigSuspend->fDone = 1;
                pThrd->u.pSigSuspend = NULL;
            }
            pThrd->enmStatus = enmLIBCThreadStatus_unknown;
        }

        /*
         * Can we ignore this signal?
         */
        if (    SigAction.__sigaction_u.__sa_handler == SIG_IGN
            ||  (   SigAction.__sigaction_u.__sa_handler == SIG_DFL
                 && (gafSignalProperties[iSignalNo] & SPA_MASK) == SPA_IGNORE)
            )
        {
            /* Ignore the signal. */
            iSignalNo = 0;
            continue;
        }

        /*
         * What to do for this signal?
         */
        if (SigAction.__sigaction_u.__sa_handler == SIG_DFL)
        {
            /*
             * Perform default action.
             */
            switch (gafSignalProperties[iSignalNo] & SPA_MASK)
            {
                /*
                 * Ignore the signal in one of another way.
                 */
                case SPA_NEXT:
                case SPA_IGNORE:
                    break;

                /*
                 * Kill process / dump core.
                 */
                case SPA_NEXT_KILL:
                case SPA_KILL:
                    signalTerminate(iSignalNo);
                case SPA_CORE:
                case SPA_NEXT_CORE:
                    signalTerminateAbnormal(iSignalNo, pvXcptParams);
                    break;

                /*
                 * Stop the current process.
                 */
                case SPA_STOP:
                case SPA_STOPTTY:
                    signalJobStop(iSignalNo);
                    break;

                /*
                 * Resume the current process.
                 */
                case SPA_RESUME:
                    signalJobResume();
                    break;

                default:
                    LIBC_ASSERTM_FAILED("Invalid signal action property! iSignalNo=%d\n", iSignalNo);
                    break;
            }
        }
        else
        {
            /*
             * Prepare call.
             */
            /* reset action in SysV fashion? */
            if (    (SigAction.sa_flags & SA_RESETHAND)
                && !(gafSignalProperties[iSignalNo] & SPP_NORESET))
            {
                LIBCLOG_MSG("Performing SA_RESETHAND on %d.\n", iSignalNo);
                gaSignalActions[iSignalNo].__sigaction_u.__sa_handler = SIG_DFL;
                gaSignalActions[iSignalNo].sa_flags &= ~SA_SIGINFO;
                SigAction.sa_flags |= SA_NODEFER;
            }

            /* adjust and apply the signal mask. */
            if ((SigAction.sa_flags & (SA_ACK | SA_NODEFER)) != SA_NODEFER)
                __SIGSET_SET(&SigAction.sa_mask, iSignalNo);
            if (gafSignalProperties[iSignalNo] & SPP_NOBLOCK)
                __SIGSET_CLEAR(&SigAction.sa_mask, iSignalNo);
            sigset_t SigSetOld = pThrd->SigSetBlocked;
            if (pThrd->fSigSetBlockedOld)
            {
                SigSetOld = pThrd->SigSetBlockedOld;
                pThrd->fSigSetBlockedOld = 0;
            }
            __SIGSET_OR(&pThrd->SigSetBlocked, &pThrd->SigSetBlocked, &SigAction.sa_mask);

            /*
             * Setup ucontext.
             */
            void *pvCtx = NULL;
            if (SigAction.sa_flags & SA_SIGINFO)
            {
                /** @todo Implement x86 signal context. */
            }

            /*
             * Call the signal handler.
             */
            if (    !(SigAction.sa_flags & SA_ONSTACK)
                ||  pThrd->fSigStackActive
                ||  !pThrd->pvSigStack
                ||  !pThrd->cbSigStack)
            {
                LIBCLOG_MSG("Calling %p(%d, %p:{.si_signo=%d, .si_errno=%d, .si_code=%#x, .si_timestamp=%#x, .si_flags=%#x .si_pid=%#x, .si_pgrp=%#x, .si_tid=%#x, .si_uid=%d, .si_status=%d, .si_addr=%p, .si_value=%p, .si_band=%ld, .si_fd=%d}, %p)\n",
                            (void *)SigAction.__sigaction_u.__sa_sigaction, iSignalNo, (void *)&SigInfo, SigInfo.si_signo, SigInfo.si_errno,
                            SigInfo.si_code, SigInfo.si_timestamp, SigInfo.si_flags, SigInfo.si_pid, SigInfo.si_pgrp, SigInfo.si_tid,
                            SigInfo.si_uid, SigInfo.si_status, SigInfo.si_addr, SigInfo.si_value.sigval_ptr, SigInfo.si_band, SigInfo.si_fd, pvCtx);
                __libc_back_signalSemRelease();
                SigAction.__sigaction_u.__sa_sigaction(iSignalNo, &SigInfo, pvCtx);
            }
            else
            {
                /*
                 * Deliver on an alternative stack.
                 */

                /* get and save the tib stack pointer entries. */
                PTIB pTib;
                PPIB pPib;
                FS_VAR();
                FS_SAVE_LOAD();
                DosGetInfoBlocks(&pTib, &pPib);
                FS_RESTORE();
                PVOID pvOldStack      = pTib->tib_pstack;
                PVOID pvOldStackLimit = pTib->tib_pstacklimit;

                /* Mark active before we touch the stack so we don't make the same mistake twice... */
                pThrd->fSigStackActive = 1;
                /* Setup stack frame for the call. */
                uintptr_t *pStack = (uintptr_t *)((char *)pThrd->pvSigStack + pThrd->cbSigStack - sizeof(uintptr_t));
                *pStack-- = 0; /* where we save esp */
                *pStack-- = (uintptr_t)pvCtx;
                *pStack-- = (uintptr_t)&SigInfo;
                *pStack   = iSignalNo;
                pTib->tib_pstack      = pThrd->pvSigStack;
                pTib->tib_pstacklimit = (char *)pThrd->pvSigStack + pThrd->cbSigStack;

                LIBCLOG_MSG("Calling %p(%d, %p:{.si_signo=%d, .si_errno=%d, .si_code=%#x, .si_timestamp=%#x, .si_flags=%#x .si_pid=%#x, .si_pgrp=%#x, .si_tid=%#x, .si_uid=%d, .si_status=%d, .si_addr=%p, .si_value=%p, .si_band=%ld, .si_fd=%d}, %p) on stack %p-%p (old stack %p-%p)\n",
                            (void *)SigAction.__sigaction_u.__sa_sigaction, iSignalNo, (void *)&SigInfo, SigInfo.si_signo, SigInfo.si_errno,
                            SigInfo.si_code, SigInfo.si_timestamp, SigInfo.si_flags, SigInfo.si_pid, SigInfo.si_pgrp, SigInfo.si_tid,
                            SigInfo.si_uid, SigInfo.si_status, SigInfo.si_addr, SigInfo.si_value.sigval_ptr, SigInfo.si_band, SigInfo.si_fd,
                            pvCtx, pTib->tib_pstack, pTib->tib_pstacklimit, pvOldStack, pvOldStackLimit);

                /* release, switch and call. */
                __libc_back_signalSemRelease();
                __asm__ __volatile__ (
                    "mov  %%esp, %0\n\t"
                    "movl %1, %%esp\n\t"
                    "call *%2\n\t"
                    "mov 12(%%esp), %%esp\n\t"
                    : "=m" (*(uintptr_t *)((char *)pThrd->pvSigStack + pThrd->cbSigStack - sizeof(uintptr_t)))
                    : "c" (pStack),
                      "d" (SigAction.__sigaction_u.__sa_sigaction)
                    : "eax" );

                /* Restore tib and release the stack. */
                pTib->tib_pstack      = pvOldStack;
                pTib->tib_pstacklimit = pvOldStackLimit;
                pThrd->fSigStackActive = 0;
            }

            /*
             * Handler returned, what to do now?
             */
            switch (gafSignalProperties[iSignalNo] & SPR_MASK)
            {
                /*
                 * Kill the process.
                 */
                case SPR_KILL:
                    signalTerminate(iSignalNo);
                    break;

                /*
                 * Execution should continue on return, which is our default return.
                 */
                case SPR_CONTINUE:
                    break;

                default:
                    LIBC_ASSERTM_FAILED("Invalid signal return action property! iSignalNo=%d\n", iSignalNo);
                    break;
            }

            /*
             * Re-take the signal semaphore and restore the signal mask.
             * We'll have to reschedule signals here unless the two masks are 100% equal.
             */
            if (__libc_back_signalSemRequest())
                LIBCLOG_ERROR_RETURN_INT(rcRet);
            pThrd->SigSetBlocked = SigSetOld;
            signalScheduleSPM(pThrd);
            signalScheduleProcess(pThrd);
        }

        iSignalNo = 0;
    } /* forever */

    LIBC_ASSERT(__libc_back_signalSemIsOwner());
    __libc_back_signalSemRelease();
    if (rcRet >= 0)
        LIBCLOG_RETURN_INT(rcRet);
    LIBCLOG_ERROR_RETURN_INT(rcRet);
}


/**
 * Cause immediate process termination because of the given signal.
 *
 * @param   iSignalNo   Signal causing the termination.
 */
static void signalTerminate(int iSignalNo)
{
    LIBCLOG_ENTER("iSignalNo=%d\n", iSignalNo);

    /*
     * Set the exit reason in SPM and exit process.
     */
    __libc_spmTerm(__LIBC_EXIT_REASON_SIGNAL_BASE + iSignalNo, 127);
    LIBCLOG_MSG("Calling DosKillProcess()\n");
    for (;;)
    {
        DosKillProcess(DKP_PROCESS, fibGetPid());
        DosExit(EXIT_PROCESS, 127);
    }
}


/**
 * Cause immediate process termination because of the given signal.
 *
 * @param   iSignalNo       Signal causing the termination.
 * @param   pvXcptParams    Pointer to the argument list of the OS/2 exception handler.
 *                          NULL if not available.
 */
static void signalTerminateAbnormal(int iSignalNo, void *pvXcptParams)
{
    LIBCLOG_ENTER("iSignalNo=%d\n", iSignalNo);

    /*
     * Set the exit reason in SPM.
     */
    __libc_spmTerm(__LIBC_EXIT_REASON_SIGNAL_BASE + iSignalNo, 127);

    /*
     * Panic
     */
    PCONTEXTRECORD pCtx = pvXcptParams ? pCtx = ((PEXCPTPARAMS)pvXcptParams)->pCtx : NULL;
    if (iSignalNo < sizeof(gaszSignalNames) / sizeof(gaszSignalNames[0]))
        __libc_Back_panic(__LIBC_PANIC_SIGNAL | __LIBC_PANIC_NO_SPM_TERM, pCtx, "Killed by %s\n", gaszSignalNames[iSignalNo]);
    else
        __libc_Back_panic(__LIBC_PANIC_SIGNAL | __LIBC_PANIC_NO_SPM_TERM, pCtx, "Killed by unknown signal %d\n", iSignalNo);
    LIBCLOG_MSG("panic failed\n"); /* shuts up gcc */
    asm("int3");
}


/**
 * Stop the process (job control emulation).
 *
 * @param   iSignalNo   Signal causing the stop.
 */
static int signalJobStop(int iSignalNo)
{
    LIBC_ASSERTM(__libc_threadCurrent()->tid == 1, "Invalid thread %#x\n", __libc_threadCurrent()->tid);

    /*
     * Signal parent.
     */
    siginfo_t   SigInfo = {0};
    SigInfo.si_signo    = SIGCHLD;
    SigInfo.si_pid      = _sys_pid;
    SigInfo.si_code     = CLD_STOPPED;
    SigInfo.si_status   = iSignalNo;
    SigInfo.si_flags    = __LIBC_SI_INTERNAL;
    __libc_back_signalSendPidOther(_sys_ppid, SIGCHLD, &SigInfo);

    /*
     * Wait for interruption (outside semaphores of course).
     */
    DosSleep(0);                        /** @todo rewrite to call the signalWait worker and change the thread state while here! */
    __libc_back_signalSemRelease();
    int rc;
    do rc = DosWaitEventSem(__libc_back_ghevWait, 24*3600*1000); while (rc == ERROR_SEM_TIMEOUT || rc == ERROR_TIMEOUT);
    return __libc_back_signalSemRequest();
}


/**
 * Resume the process (job control emulation).
 */
static int signalJobResume(void)
{
    LIBC_ASSERTM(__libc_threadCurrent()->tid == 1, "Invalid thread %#x\n", __libc_threadCurrent()->tid);

    /*
     * Signal parent.
     */
    siginfo_t   SigInfo = {0};
    SigInfo.si_signo    = SIGCHLD;
    SigInfo.si_pid      = _sys_pid;
    SigInfo.si_code     = CLD_CONTINUED;
    SigInfo.si_status   = SIGCONT;
    SigInfo.si_flags    = __LIBC_SI_INTERNAL;
    __libc_back_signalSendPidOther(_sys_ppid, SIGCHLD, &SigInfo);
    return 0;
}


/**
 * Send signal to other process.
 *
 * Practically speaking a kill() implementation, but only for a single process.
 *
 * @return 0 on success.
 * @return -errno on failure.
 * @param   pid         Target pid.
 * @param   iSignalNo   Signal number to send.
 * @param   pSigInfo    Signal info (optional).
 */
int __libc_back_signalSendPidOther(pid_t pid, int iSignalNo, const siginfo_t *pSigInfo)
{
    LIBCLOG_ENTER("pid=%d iSignalNo=%d pSigInfo=%p\n", pid, iSignalNo, (void *)pSigInfo);
    int     rc;
    int     rcOS2;
    FS_VAR();

    /*
     * Attempt queue it assuming it's a LIBC process first.
     * If not a LIBC process (rc is -ESRCH) we'll use alternative
     * methods. First resorting to OS/2 signal exceptions and the to EMX signals.
     */
    siginfo_t SigInfo = {0};
    if (pSigInfo)
        SigInfo = *pSigInfo;
    SigInfo.si_signo        = iSignalNo;
    if (!pSigInfo)
        SigInfo.si_code     = SI_USER;
    if (SigInfo.si_pid)
        SigInfo.si_pid      = _sys_pid;
    __LIBC_PTHREAD pThrd    = __libc_threadCurrentNoAuto();
    if (pThrd)
        SigInfo.si_tid      = pThrd->tid;
    if (SigInfo.si_timestamp)
        SigInfo.si_timestamp= signalTimestamp();
    if (gafSignalProperties[iSignalNo] & SPP_QUEUED)
        SigInfo.si_flags   |= __LIBC_SI_QUEUED;

    rc = __libc_spmSigQueue(iSignalNo, &SigInfo, pid, SigInfo.si_flags & __LIBC_SI_QUEUED);
    if (!rc)
    {
        if (!iSignalNo)
            LIBCLOG_RETURN_MSG(0, "0 (LIBC permission check ok)\n");

        /* poke it... */
        FS_SAVE_LOAD();
        rc = DosFlagProcess(pid, FLGP_PID, PFLG_A, 0);
        if (rc == ERROR_SIGNAL_REFUSED)
        {
            /* Try again... no sure what would cause this condition... */
            DosSleep(0);
            DosSleep(1);
            rc = DosFlagProcess(pid, FLGP_PID, PFLG_A, 0);
        }
        FS_RESTORE();
    }
    else if (rc == -ESRCH)
    {
        /*
         * Check that the process exists first.
         * Only root users are allowed to do this. (Mainly to fix some testcases :-)
         */
        if (__libc_spmGetId(__LIBC_SPMID_EUID))
            LIBCLOG_ERROR_RETURN_MSG(-EPERM, "%d (-EPERM) - Only root can signal OS/2 processes. euid=%d\n", -EPERM, __libc_spmGetId(__LIBC_SPMID_EUID));
        FS_SAVE_LOAD();
        rc = DosVerifyPidTid(pid, 1);
        FS_RESTORE();
        if (rc)
            LIBCLOG_ERROR_RETURN_MSG(-ESRCH, "%d (-ESRCH) - DosVerifyPidTid(%d, 1) -> %d\n", -ESRCH, pid, rc);
        if (!iSignalNo)
            LIBCLOG_RETURN_MSG(0, "0 (OS/2 permission check ok)\n");

        /*
         * Send the signal.
         */
        switch (iSignalNo)
        {
            /*
             * SIGKILL means kill process.
             */
            case SIGKILL:
                FS_SAVE_LOAD();
                rcOS2 = DosKillProcess(DKP_PROCESS, pid);
                FS_RESTORE();
                switch (rcOS2)
                {
                    case NO_ERROR:                  rc = 0; break;
                    case ERROR_INVALID_PROCID:      rc = -ESRCH; break;
                    default:                        rc = -EPERM; break;
                }
                if (rc >= 0)
                    LIBCLOG_RETURN_MSG(rc, "ret %d (DosKillProcess rcOS2=%d)\n", rc, rcOS2);
                LIBCLOG_ERROR_RETURN_MSG(rc, "ret %d (DosKillProcess rcOS2=%d)\n", rc, rcOS2);

            /*
             * We can send these the normal way to decentants.
             */
            case SIGINT:
            case SIGBREAK:
                FS_SAVE_LOAD();
                rcOS2 = DosSendSignalException(pid, iSignalNo == SIGINT ? XCPT_SIGNAL_INTR : XCPT_SIGNAL_BREAK);
                FS_RESTORE();
                switch (rcOS2)
                {
                    case NO_ERROR:
                        LIBCLOG_RETURN_INT(0);
                    case ERROR_INVALID_PROCID:
                        LIBCLOG_ERROR_RETURN_INT(-ESRCH);
                }
                /* try flag it */
                break;

            default:
                break;
        }

        /*
         * EMX style signal.
         *
         * Doesn't work for all types of signals and only if process is EMX.
         * The latter we cannot easily check...
         */
        unsigned uSignalEMX = 0;
        switch (iSignalNo)
        {
            case SIGHUP  :  uSignalEMX = EMX_SIGHUP;   break;
            case SIGINT  :  uSignalEMX = EMX_SIGINT;   break;
            case SIGQUIT :  uSignalEMX = EMX_SIGQUIT;  break;
            case SIGILL  :  uSignalEMX = EMX_SIGILL;   break;
            case SIGTRAP :  uSignalEMX = EMX_SIGTRAP;  break;
            case SIGABRT :  uSignalEMX = EMX_SIGABRT;  break;
            case SIGEMT  :  uSignalEMX = EMX_SIGEMT;   break;
            case SIGFPE  :  uSignalEMX = EMX_SIGFPE;   break;
            case SIGKILL :  uSignalEMX = EMX_SIGKILL;  break;
            case SIGBUS  :  uSignalEMX = EMX_SIGBUS;   break;
            case SIGSEGV :  uSignalEMX = EMX_SIGSEGV;  break;
            case SIGSYS  :  uSignalEMX = EMX_SIGSYS;   break;
            case SIGPIPE :  uSignalEMX = EMX_SIGPIPE;  break;
            case SIGALRM :  uSignalEMX = EMX_SIGALRM;  break;
            case SIGTERM :  uSignalEMX = EMX_SIGTERM;  break;
            case SIGUSR1 :  uSignalEMX = EMX_SIGUSR1;  break;
            case SIGUSR2 :  uSignalEMX = EMX_SIGUSR2;  break;
            case SIGCHLD :  uSignalEMX = EMX_SIGCHLD;  break;
            case SIGBREAK:  uSignalEMX = EMX_SIGBREAK; break;
            case SIGWINCH:  uSignalEMX = EMX_SIGWINCH; break;

        }
        if (uSignalEMX)
        {
            FS_SAVE_LOAD();
            rc = DosFlagProcess(pid, FLGP_PID, PFLG_A, uSignalEMX);
            if (rc == ERROR_SIGNAL_PENDING || rc == ERROR_SIGNAL_REFUSED)
            {
                DosSleep(0);
                DosSleep(1);
                rc = DosFlagProcess(pid, FLGP_PID, PFLG_A, uSignalEMX);
            }
            FS_RESTORE();
        }
        else
            LIBCLOG_ERROR_RETURN_INT(-EPERM);
    }
    else
        LIBCLOG_ERROR_RETURN_INT(rc);

    /*
     * Check for DosFlagProcess error.
     */
    if (!rc)
        LIBCLOG_RETURN_INT(0);
    rc = -__libc_native2errno(rc);
    LIBCLOG_ERROR_RETURN_INT(rc);
}

/**
 * Sends a signal to a process group.
 *
 * Special case for iSignalNo equal to 0, where no signal is sent but permissions to
 * do so is checked.
 *
 * @returns 0 on if signal sent.
 * @returns -errno on failure.
 *
 * @param   pgrp        Process group (positive).
 *                      0 means the process group of this process.
 *                      1 means all process in the system. (not implemented!)
 * @param   iSignalNo   Signal to send to all the processes in the group.
 *                      If 0 no signal is sent, but error handling is done as if.
 */
int         __libc_Back_signalSendPGrp(pid_t pgrp, int iSignalNo)
{
    LIBCLOG_ENTER("pgrp=%#x (%d) iSignalNo=%d\n", pgrp, pgrp, iSignalNo);
    int     rc;

    /*
     * Validate input.
     */
    if (!__SIGSET_SIG_VALID(iSignalNo) && iSignalNo != 0)
        LIBCLOG_ERROR_RETURN(-EINVAL, "ret -EINVAL - Invalid signal no. %d\n", iSignalNo);
    if (pgrp < 0 || pgrp == 1)
        LIBCLOG_ERROR_RETURN(-EINVAL, "ret -EINVAL - Invalid pgrp %d\n", pgrp);

    /*
     * Create signal packet.
     */
    siginfo_t SigInfo = {0};
    SigInfo.si_signo        = iSignalNo;
    SigInfo.si_code         = SI_USER;
    if (SigInfo.si_pid)
        SigInfo.si_pid      = _sys_pid;
    __LIBC_PTHREAD pThrd    = __libc_threadCurrent();
    if (pThrd)
        SigInfo.si_tid      = pThrd->tid;
    if (SigInfo.si_timestamp)
        SigInfo.si_timestamp= signalTimestamp();
    if (gafSignalProperties[iSignalNo] & SPP_QUEUED)
        SigInfo.si_flags   |= __LIBC_SI_QUEUED;

    /*
     * Take the signal semaphore.
     */
    rc = __libc_back_signalSemRequest();
    if (rc)
        LIBCLOG_ERROR_RETURN_INT(rc);

    /*
     * Call SPM to do the enumeration and queueing.
     */
    rc = __libc_spmSigQueuePGrp(iSignalNo, &SigInfo, pgrp, SigInfo.si_flags & __LIBC_SI_QUEUED, signalSendPGrpCallback, NULL);

    /*
     * Just in case the signal were for our selves too, we'll do the
     * schedule & deliver stuff.
     */
    signalScheduleSPM(pThrd);
    signalScheduleProcess(pThrd);
    signalDeliver(pThrd, 0, NULL);

    if (!rc)
        LIBCLOG_RETURN_INT(rc);
    LIBCLOG_ERROR_RETURN_INT(rc);
}


/**
 * Callback for poking a process which was signaled as part of a group signaling.
 * @returns 0 on success.
 */
static int signalSendPGrpCallback(int iSignalNo, const __LIBC_SPMPROCESS *pProcess, void *pvUser)
{
    LIBCLOG_ENTER("iSignalNo=%d pProcess=%p {pid=%#x} pvUser=%p\n", iSignalNo, (void *)pProcess, pProcess->pid, pvUser);
    if (!iSignalNo)
        LIBCLOG_RETURN_MSG(0, "0 (LIBC permission check ok)\n");

    /* poke it... */
    FS_VAR();
    FS_SAVE_LOAD();
    int rc = DosFlagProcess(pProcess->pid, FLGP_PID, PFLG_A, 0);
    if (rc == ERROR_SIGNAL_REFUSED)
    {
        /* Try again... no sure what would cause this condition... */
        DosSleep(0);
        DosSleep(1);
        rc = DosFlagProcess(pProcess->pid, FLGP_PID, PFLG_A, 0);
    }
    FS_RESTORE();
    if (!rc)
        LIBCLOG_RETURN_INT(0);
    rc = -__libc_native2errno(rc);
    LIBCLOG_ERROR_RETURN_INT(rc);
}



/**
 * OS/2 V1 signal handler used for interprocess signaling.
 * This is called by the 16-bit thunker.
 *
 * @param   uOS2Signal  Signal number - SIG_PFLG_A.
 * @param   uArg        LIBC sets this to 0 when the target is a LIBC process.
 *                      If the target is a non-LIBC process it's an EMX signal
 *                      number.
 */
void __libc_back_signalOS2V1Handler32bit(unsigned uOS2Signal, unsigned uArg)
{
    LIBCLOG_ENTER("uOS2Signal=%u uArg=%u\n", uOS2Signal, uArg);

    /*
     * Can't be to careful!
     */
    LIBC_ASSERTM(!__libc_back_signalSemIsOwner(), "Thread owns the signal semaphore!!! That should be impossible...\n");

    /*
     * Get the current thread (1st thread, so that should be safe)
     * and set the last signal TS.
     */
    __LIBC_PTHREAD  pThrd = __libc_threadCurrentNoAuto();
    if (!pThrd)
    {
        LIBC_ASSERTM_FAILED("Failed to get thread structure!\n");
        LIBCLOG_ERROR_RETURN_VOID();
    }
    pThrd->ulSigLastTS = fibGetMsCount();

    /*
     * Take the signal semaphore.
     */
    int rc = __libc_back_signalSemRequest();
    if (rc)
    {
        LIBC_ASSERTM_FAILED("Can't aquire signal semaphore!\n");
        LIBCLOG_ERROR_RETURN_VOID(); /* Just return since we're probably close to death anyway now. */
    }

    /*
     * Does this thread have a notification callback?
     */
    if (pThrd->pfnSigCallback)
        pThrd->pfnSigCallback(0, pThrd->pvSigCallbackUser);

    /*
     * Try schedule signals pending on 1st (SPM) and 2nd level (private).
     */
    signalScheduleSPM(pThrd);
    signalScheduleProcess(pThrd);

    /*
     * This could be an EMX signal.
     */
    if (uArg)
    {
        unsigned iSignalNo = 0;
        switch (uArg)
        {
            case EMX_SIGHUP  :  iSignalNo = SIGHUP;   break;
            case EMX_SIGINT  :  iSignalNo = SIGINT;   break;
            case EMX_SIGQUIT :  iSignalNo = SIGQUIT;  break;
            case EMX_SIGILL  :  iSignalNo = SIGILL;   break;
            case EMX_SIGTRAP :  iSignalNo = SIGTRAP;  break;
            case EMX_SIGABRT :  iSignalNo = SIGABRT;  break;
            case EMX_SIGEMT  :  iSignalNo = SIGEMT;   break;
            case EMX_SIGFPE  :  iSignalNo = SIGFPE;   break;
            case EMX_SIGKILL :  iSignalNo = SIGKILL;  break;
            case EMX_SIGBUS  :  iSignalNo = SIGBUS;   break;
            case EMX_SIGSEGV :  iSignalNo = SIGSEGV;  break;
            case EMX_SIGSYS  :  iSignalNo = SIGSYS;   break;
            case EMX_SIGPIPE :  iSignalNo = SIGPIPE;  break;
            case EMX_SIGALRM :  iSignalNo = SIGALRM;  break;
            case EMX_SIGTERM :  iSignalNo = SIGTERM;  break;
            case EMX_SIGUSR1 :  iSignalNo = SIGUSR1;  break;
            case EMX_SIGUSR2 :  iSignalNo = SIGUSR2;  break;
            case EMX_SIGCHLD :  iSignalNo = SIGCHLD;  break;
            case EMX_SIGBREAK:  iSignalNo = SIGBREAK; break;
            case EMX_SIGWINCH:  iSignalNo = SIGWINCH; break;
        }
        if (iSignalNo)
        {
            /*
             * Shcedule the signal, ignore return code.
             */
            signalSchedule(pThrd, iSignalNo, NULL, __LIBC_BSRF_EXTERNAL, NULL);
        }
    }

    /*
     * Acknowledge the signal.
     */
    DosSetSigHandler(NULL, NULL, NULL, SIGA_ACKNOWLEDGE, uOS2Signal);

    /*
     * Deliver signals and release the semaphore.
     */
    signalDeliver(pThrd, 0, NULL);
    LIBCLOG_RETURN_VOID();
}


/**
 * Pokes a thread in the current process, forcing it to
 * evaluate pending signals.
 *
 * @param   pThrdPoke       Thread to poke.
 */
void     __libc_back_signalPokeThread(__LIBC_PTHREAD pThrdPoke)
{
    LIBCLOG_ENTER("pThrdPoke=%p {.tid=%#x}\n", (void *)pThrdPoke, pThrdPoke->tid);
    LIBC_ASSERTM(__libc_back_signalSemIsOwner(), "Thread must own signal sem!\n");

    /*
     * Kick it if it's not already poked.
     */
    if (!pThrdPoke->fSigBeingPoked)
    {
        DosSuspendThread(pThrdPoke->tid);
        __atomic_xchg(&pThrdPoke->fSigBeingPoked, 1);
        int rc = DosKillThread(pThrdPoke->tid);
        DosResumeThread(pThrdPoke->tid);
        if (rc)
        {
            __atomic_xchg(&pThrdPoke->fSigBeingPoked, 0);
            LIBC_ASSERTM_FAILED("DosKillThread(%d) -> rc=%d\n", pThrdPoke->tid, rc);
        }
    }

    LIBCLOG_RETURN_VOID();
}



/**
 * Thread was poked, deliver signals for this thread.
 */
int         __libc_back_signalRaisePoked(void *pvXcptParams, int tidPoker)
{
    LIBC_ASSERTM(!__libc_back_signalSemIsOwner(), "Thread owns the signal semaphore!!! Bad boy!!\n");

    /*
     * Get thread structure and set the signal TS.
     */
    __LIBC_PTHREAD pThrd = __libc_threadCurrentNoAuto();
    if (pThrd)
    {
        pThrd->ulSigLastTS = fibGetMsCount();

        /*
         * Take signal semaphore.
         */
        int rc = __libc_back_signalSemRequest();
        if (!rc)
        {
            if (pThrd->fSigBeingPoked)
            {
                /*
                 * Clear the indicator and deliver signals (releaseing the semaphore).
                 */
                __atomic_xchg(&pThrd->fSigBeingPoked, 0);
                rc = signalDeliver(pThrd, 0, pvXcptParams);
                if (rc < 0)
                    rc = __LIBC_BSRR_CONTINUE | __LIBC_BSRR_INTERRUPT;
                return rc;
            }

            /*
             * We weren't being poked.
             */
            __libc_back_signalSemRelease();
        }
    }
    else
        LIBC_ASSERTM_FAILED("No thread structure! This is really quite impossible!\n");

    return -EINVAL;
}


/**
 * This is a hack to deal with potentially lost thread pokes.
 * 
 * For some reason or another we loose the async signal in some situations. It's 
 * been observed happening after/when opening files (fopen), but it's not known
 * whether this is really related or not.
 */
void        __libc_Back_signalLostPoke(void)
{
    LIBCLOG_ENTER("\n");
    __LIBC_PTHREAD pThrd = __libc_threadCurrentNoAuto();
    if (pThrd && pThrd->fSigBeingPoked)
    {
        ULONG ulSigLastTS = pThrd->ulSigLastTS;
        DosSleep(0);
        if (    pThrd->fSigBeingPoked
            &&  pThrd->ulSigLastTS == ulSigLastTS)
        {
            DosSleep(0);
            if (    pThrd->fSigBeingPoked
                &&  pThrd->ulSigLastTS == ulSigLastTS)
            {

                int rc = __libc_back_signalSemRequest();
                if (!rc)
                {
                    if (    pThrd->fSigBeingPoked
                        &&  pThrd->ulSigLastTS == ulSigLastTS)
                    {
                        /*
                         * Clear the indicator and deliver signals (releaseing the semaphore).
                         */
                        __atomic_xchg(&pThrd->fSigBeingPoked, 0);
                        signalDeliver(pThrd, 0, NULL);
                        LIBCLOG_RETURN_VOID();
                        return;
                    }
                    __libc_back_signalSemRelease();
                }
            }
        }
    }
    LIBCLOG_RETURN_VOID();
}


/**
 * sigaction worker; queries and/or sets the action for a signal.
 *
 * @returns 0 on success.
 * @returns Negative error code (errno) on failure.
 * @param   iSignalNo   Signal number.
 * @param   pSigAct     Pointer to new signal action.
 *                      If NULL no update is done.
 * @param   pSigActOld  Where to store the old signal action.
 *                      If NULL nothing is attempted stored.
 */
int         __libc_Back_signalAction(int iSignalNo, const struct sigaction *pSigAct, struct sigaction *pSigActOld)
{
    LIBCLOG_ENTER("iSignalNo=%d pSigAct=%p {sa_handler=%p, sa_mask={%#08lx %#08lx}, sa_flags=%#x} pSigActOld=%p\n",
                  iSignalNo, (void *)pSigAct,
                  pSigAct ? (void*)pSigAct->__sigaction_u.__sa_sigaction : NULL,
                  pSigAct ? pSigAct->sa_mask.__bitmap[0] : 0,
                  pSigAct ? pSigAct->sa_mask.__bitmap[1] : 0,
                  pSigAct ? pSigAct->sa_flags : 0,
                  (void *)pSigActOld);

    /*
     * Take the semaphore
     */
    LIBC_ASSERT(!__libc_back_signalSemIsOwner());
    int rc = __libc_back_signalSemRequest();
    if (!rc)
    {
        rc = __libc_back_signalAction(iSignalNo, pSigAct, pSigActOld);
        __libc_back_signalSemRelease();
    }

    if (rc >= 0)
        LIBCLOG_RETURN_INT(rc);
    LIBCLOG_ERROR_RETURN_INT(rc);
}


/**
 * sigaction worker; queries and/or sets the action for a signal.
 *
 * The caller MUST own the signal semaphore when calling this function.
 *
 * @returns 0 on success.
 * @returns Negative error code (errno.h) on failure.
 * @param   iSignalNo   Signal number.
 * @param   pSigAct     Pointer to new signal action.
 *                      If NULL no update is done.
 * @param   pSigActOld  Where to store the old signal action.
 *                      If NULL nothing is attempted stored.
 */
int         __libc_back_signalAction(int iSignalNo, const struct sigaction *pSigAct, struct sigaction *pSigActOld)
{
    LIBCLOG_ENTER("iSignalNo=%d pSigAct=%p {sa_handler=%p, sa_mask={%#08lx%#08lx}, sa_flags=%#x} pSigActOld=%p\n",
                  iSignalNo, (void *)pSigAct,
                  pSigAct ? (void*)pSigAct->__sigaction_u.__sa_sigaction : NULL,
                  pSigAct ? pSigAct->sa_mask.__bitmap[0] : 0,
                  pSigAct ? pSigAct->sa_mask.__bitmap[1] : 0,
                  pSigAct ? pSigAct->sa_flags : 0,
                  (void *)pSigActOld);

    /*
     * Assert state.
     */
    LIBC_ASSERTM(__libc_back_signalSemIsOwner(), "Thread does not own the signal semaphore!!!\n");

    /*
     * Check input.
     */
    if (    !__SIGSET_SIG_VALID(iSignalNo)
        ||  (pSigAct && (gafSignalProperties[iSignalNo] & SPP_NOBLOCK)))
        LIBCLOG_ERROR_RETURN_INT(-EINVAL);

    /*
     * Query.
     */
    if (pSigActOld)
        *pSigActOld = gaSignalActions[iSignalNo];

    /*
     * Set.
     */
    int rc = 0;
    if (pSigAct)
    {
        /*
         * Validate the new action.
         */
        if (!(pSigAct->sa_flags & ~(  SA_ONSTACK | SA_RESTART | SA_RESETHAND | SA_NODEFER | SA_NOCLDWAIT
                                    | SA_SIGINFO | SA_NOCLDSTOP | SA_ACK | SA_SYSV)))
        {
            if (    pSigAct->__sigaction_u.__sa_handler != SIG_ERR
                &&  pSigAct->__sigaction_u.__sa_handler != SIG_ACK
                &&  pSigAct->__sigaction_u.__sa_handler != SIG_HOLD)
            {
                /*
                 * Check if it's SIGCHLD and changes the way wait*() works.
                 */
                if (    iSignalNo == SIGCHLD
                    &&  (   (   gaSignalActions[iSignalNo].__sigaction_u.__sa_handler != pSigAct->__sigaction_u.__sa_handler
                             && (   pSigAct->__sigaction_u.__sa_handler == SIG_IGN
                                 ||  gaSignalActions[iSignalNo].__sigaction_u.__sa_handler == SIG_IGN))
                         ||  ((gaSignalActions[iSignalNo].sa_flags & SA_NOCLDWAIT) ^ (pSigAct->sa_flags & SA_NOCLDWAIT))
                         )
                    )
                    __libc_back_processWaitNotifyNoWait(*pSigAct->__sigaction_u.__sa_handler == SIG_IGN || (pSigAct->sa_flags & SA_NOCLDWAIT));

                /*
                 * Update the handler.
                 */
                gaSignalActions[iSignalNo] = *pSigAct;
                __SIGSET_CLEAR(&gaSignalActions[iSignalNo].sa_mask, SIGKILL);
                __SIGSET_CLEAR(&gaSignalActions[iSignalNo].sa_mask, SIGSTOP);

                /*
                 * If set to ignore, we'll remove any pending signals of this type.
                 */
                if (    gaSignalActions[iSignalNo].__sigaction_u.__sa_handler == SIG_IGN
                    ||  (   gaSignalActions[iSignalNo].__sigaction_u.__sa_handler == SIG_DFL
                         && (gafSignalProperties[iSignalNo] & SPA_MASK) == SPA_IGNORE))
                {
                    __libc_threadEnum(signalActionWorker, (void *)iSignalNo);
                }

            }
            else
            {
                LIBCLOG_ERROR("Invalid sa_handler=%p\n", (void *)pSigAct->__sigaction_u.__sa_handler);
                rc = -EINVAL;
            }
        }
        else
        {
            LIBCLOG_ERROR("Invalid sa_flags=%#x\n", pSigAct->sa_flags);
            rc = -EINVAL;
        }
    }

    if (!rc)
        LIBCLOG_RETURN_MSG(rc, "ret %d (pSigActOld=%p {sa_handler=%p, sa_mask={%#08lx%#08lx}, sa_flags=%#x})\n",
                           rc, (void *)pSigActOld,
                           pSigActOld ? (void*)pSigActOld->__sigaction_u.__sa_sigaction : NULL,
                           pSigActOld ? pSigActOld->sa_mask.__bitmap[0] : 0,
                           pSigActOld ? pSigActOld->sa_mask.__bitmap[1] : 0,
                           pSigActOld ? pSigActOld->sa_flags : 0);
    LIBCLOG_ERROR_RETURN_MSG(rc, "ret %d (pSigActOld=%p {sa_handler=%p, sa_mask={%#08lx%#08lx}, sa_flags=%#x})\n",
                             rc, (void *)pSigActOld,
                             pSigActOld ? (void*)pSigActOld->__sigaction_u.__sa_sigaction : NULL,
                             pSigActOld ? pSigActOld->sa_mask.__bitmap[0] : 0,
                             pSigActOld ? pSigActOld->sa_mask.__bitmap[1] : 0,
                             pSigActOld ? pSigActOld->sa_flags : 0);
}



/**
 * Thread enumeration worker for __libc_back_signalAction.
 *
 * It discards pending and queued signals which now are ignored.
 * @returns 0.
 * @param   pCur        The current thread.
 * @param   pvParam     The signal number.
 */
static int signalActionWorker(__LIBC_PTHREAD pCur, void *pvParam)
{
    /*
     * Skip internal threads.
     */
    if (pCur->fInternalThread)
        return 0;

    /*
     * Check if the thread have iSignalNo pending.
     */
    int iSignalNo = (int)pvParam;
    if (__SIGSET_ISSET(&pCur->SigSetPending, iSignalNo))
    {
        __SIGSET_CLEAR(&pCur->SigSetPending, iSignalNo);

        /*
         * Check if there are any queued signals of the sort,
         * remove them from the queue.
         */
        PSIGQUEUED   pSig = pCur->SigQueue.pHead;
        while (pSig)
        {
            PSIGQUEUED   pSigNext = pSig->pNext;
            if (pSig->SigInfo.si_signo == iSignalNo)
            {
                /* unlink */
                if (pSig->pPrev)
                    pSig->pPrev->pNext = pSig->pNext;
                else
                    pCur->SigQueue.pHead = pSig->pNext;
                if (pSig->pNext)
                    pSig->pNext->pPrev = pSig->pPrev;
                else
                    pCur->SigQueue.pTail = pSig->pPrev;

                /* free it */
                switch (pSig->enmHowFree)
                {
                    case enmHowFree_PreAllocFree:
                        pSig->pNext = gpSigQueuedFree;
                        gpSigQueuedFree = pSig;
                        break;
                    case enmHowFree_HeapFree:
                        free(pSig);
                        break;
                    case enmHowFree_Temporary:
                        /* do nothing */
                        break;
                }
            }

            /* next */
            pSig = pSigNext;
        }
    }

    return 0;
}


/**
 * Removes a pending signal.
 *
 * The sign can be pending on both thread and process.
 *
 * @returns Signal which was removed.
 * @returns -EINVAL if pSigSet didn't contain any pending signals.
 * @param   pThrd       The thread to evaluate signals on.
 * @param   iSignalNo   Deliver the first signal of this type no matter if it's
 *                      blocked or not. This is for use with hardware signals only!
 * @param   pSigSet     Mask of acceptable signals.
 * @param   pSigInfo    Where to store any SigInfo, optional.
 */
int __libc_back_signalAccept(__LIBC_PTHREAD pThrd, int iSignalNo, sigset_t *pSigSet, siginfo_t *pSigInfo)
{
    LIBCLOG_ENTER("pThrd=%p pSigSet=%p{%#08lx%#08lx} pSigInfo=%p\n", (void *)pThrd,
                  (void *)pSigSet, pSigSet->__bitmap[1], pSigSet->__bitmap[0], (void *)pSigInfo);

    /*
     * Which signal.
     * Signal delivery priority:
     *   0. Passed in signal (ignores blocking).
     *   1. SIGKILL
     *   2. Realtime signals. (queued)
     *   3. SIGTERM
     *   4. SIGABRT
     *   5. SIGCHLD
     *   6. SIGCONT
     *   7. Ascending from pending mask.
     */
    if (iSignalNo <= 0)
    {
        LIBC_ASSERT(!__SIGSET_ISEMPTY(pSigSet));
        for (iSignalNo = SIGRTMIN; iSignalNo <= SIGRTMAX; iSignalNo++)
            if (__SIGSET_ISSET(pSigSet, iSignalNo))
                break;
        if (iSignalNo > SIGRTMAX)
        {
            iSignalNo = SIGKILL;
            if (!__SIGSET_ISSET(pSigSet, SIGKILL))
            {
                iSignalNo = SIGTERM;
                if (!__SIGSET_ISSET(pSigSet, SIGTERM))
                {
                    iSignalNo = SIGABRT;
                    if (!__SIGSET_ISSET(pSigSet, SIGABRT))
                    {
                        iSignalNo = SIGCHLD;
                        if (!__SIGSET_ISSET(pSigSet, SIGCHLD))
                        {
                            iSignalNo = SIGCONT;
                            if (!__SIGSET_ISSET(pSigSet, SIGCONT))
                            {
                                for (iSignalNo = 1; iSignalNo < SIGRTMIN; iSignalNo++)
                                    if (__SIGSET_ISSET(pSigSet, iSignalNo))
                                        break;
                            }
                        }
                    }
                }
            }
        }
    }

    /*
     * From thread?
     */
    PSIGQUEUED *ppSigHead;
    PSIGQUEUED *ppSigTail;
    sigset_t   *pSigSetPending;
    if (__SIGSET_ISSET(&pThrd->SigSetPending, iSignalNo))
    {
        ppSigHead = &pThrd->SigQueue.pHead;
        ppSigTail = &pThrd->SigQueue.pTail;
        pSigSetPending = &pThrd->SigSetPending;
    }
    else if (__SIGSET_ISSET(&__libc_gSignalPending, iSignalNo))
    {
        ppSigHead = &gpSigQueueHead;
        ppSigTail = &gpSigQueueTail;
        pSigSetPending = &__libc_gSignalPending;
    }
    else
    {
        LIBC_ASSERTM_FAILED("Couldn't find signal %d which should be pending somewhere...\n", iSignalNo);
        iSignalNo = -EINVAL;
        ppSigHead = ppSigTail = NULL;
        pSigSetPending = NULL;
    }

    if (ppSigHead)
    {
        /*
         * Check if there is a queue element for the signal.
         */
        PSIGQUEUED pSig = *ppSigHead;
        while (pSig)
        {
            if (pSig->SigInfo.si_signo == iSignalNo)
            {
                /* unlink */
                if (pSig->pPrev)
                    pSig->pPrev->pNext  = pSig->pNext;
                else
                    *ppSigHead          = pSig->pNext;
                if (pSig->pNext)
                    pSig->pNext->pPrev  = pSig->pPrev;
                else
                    *ppSigTail          = pSig->pPrev;

                /*
                 * Check if more signals of this type so we can update the
                 * pending mask correctly.
                 */
                PSIGQUEUED pSigMore = pSig->pNext;
                while (pSigMore && pSigMore->SigInfo.si_signo != iSignalNo)
                    pSigMore = pSigMore->pNext;
                if (!pSigMore)
                    __SIGSET_CLEAR(pSigSetPending, iSignalNo);

                /*
                 * Copy the signal to the SigInfo structure on the stack
                 * and then release the signal queue structure.
                 */
                if (pSigInfo)
                    *pSigInfo = pSig->SigInfo;

                /*
                 * Free it.
                 */
                pSig->pNext = gpSigQueuedFree;
                gpSigQueuedFree = pSig;
                gcSigQueuedFree++;
                break;
            }
            /* next */
            pSig = pSig->pNext;
        }

        /*
         * If no queued signal data, then we can simply clear it in the pending mask.
         */
        if (!pSig)
        {
            __SIGSET_CLEAR(pSigSetPending, iSignalNo);
            if (pSigInfo)
            {
                bzero(pSigInfo, sizeof(*pSigInfo));
                pSigInfo->si_signo = iSignalNo;
                pSigInfo->si_code = SI_USER;
            }
        }
    }

    LIBCLOG_RETURN_INT(iSignalNo);
}

/**
 * Wait for an interrupt to be delivered.
 *
 * @returns -ETIMEOUT if we timed out.
 *          The semaphore is not longer owned.
 * @returns -EINTR if interrupt was received and stat changed.
 *          The semaphore is owned.
 * @returns 0 on failure.
 * @param   pThrd       The current thread.
 * @param   pfDone      Pointer to the indicator that we're done waiting.
 * @param   pTimeout    Timeout specification, optional.
 */
int         __libc_back_signalWait(__LIBC_PTHREAD pThrd, volatile int *pfDone, const struct timespec *pTimeout)
{
    LIBCLOG_ENTER("pThrd=%p pfDone=%p{%d} pTimeout=%p{tv_sec=%d tv_nsec=%ld}\n",
                  (void *)pThrd, (void *)pfDone, *pfDone, (void *)pTimeout, pTimeout ? pTimeout->tv_sec : ~0, pTimeout ? pTimeout->tv_nsec : ~0);
    /*
     * Calc wait period.
     */
    ULONG cMillies = 30*1000;
    if (pTimeout)
    {
        if (pTimeout->tv_nsec >= 100000000 || pTimeout->tv_nsec < 0)
            LIBCLOG_ERROR_RETURN(-EINVAL, "ret -EINVAL - Invalid tv_nsec! tv_sec=%d tv_nsec=%ld\n", pTimeout->tv_sec, pTimeout->tv_nsec);
        if (    pTimeout->tv_sec < 0
            ||  pTimeout->tv_sec >= 4294967)
            LIBCLOG_ERROR_RETURN(-EINVAL, "ret -EINVAL - Invalid tv_sec! tv_sec=%d (max=4294967) tv_nsec=%ld\n", pTimeout->tv_sec, pTimeout->tv_nsec);

        cMillies = pTimeout->tv_sec * 1000 + pTimeout->tv_nsec / 1000000;
        if (!cMillies && pTimeout->tv_nsec)
            cMillies = 1;
    }

    /*
     * Wait for interruption.
     */
    int rc = 0;
    FS_VAR();
    FS_SAVE_LOAD();
    ULONG ulStart = 0;
    DosQuerySysInfo(QSV_MS_COUNT, QSV_MS_COUNT, &ulStart, sizeof(ulStart));
    for (;;)
    {
        /*
         * Release the semaphore and recheck exit condition before
         * engaging in waiting again.
         */
        DosSleep(0);                    /** @todo need better methods for waiting! */
        __libc_back_signalSemRelease();
        if (*pfDone)
        {
            rc = __libc_back_signalSemRequest();
            if (!rc)
                rc = -EINTR;
            break;
        }
        rc = DosWaitEventSem(__libc_back_ghevWait, cMillies);

        /*
         * We returned from the wait, but did we do so for the right reason?
         */
        int rc2 = __libc_back_signalSemRequest();
        if (rc2 < 0)
            rc = rc2;
        else if (*pfDone)
            rc = -EINTR;
        else if (pTimeout && (rc == ERROR_TIMEOUT || rc == ERROR_SEM_TIMEOUT))
            rc = -EAGAIN;
        if (rc < 0)
            break;

        /*
         * Resume waiting.
         */
        if (pTimeout)
        {
            ULONG ulEnd = 0;
            DosQuerySysInfo(QSV_MS_COUNT, QSV_MS_COUNT, &ulEnd, sizeof(ulEnd));
            ulEnd -= ulStart;
            if (ulEnd < cMillies)
                cMillies -= ulEnd;
            else
                cMillies = 1;
            ulStart = ulEnd;
        }
    } /* for ever */

    FS_RESTORE();
    LIBCLOG_RETURN_INT(rc);
}


/**
 * Gets the current timestamp.
 *
 * @returns The current timestamp.
 */
static unsigned signalTimestamp(void)
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
 * Construct the inheritance packet for signals.
 *
 * @returns 0 on success.
 * @param   ppSig       Where to store the allocated data.
 * @param   pcbSig      Size of the packet.
 */
int         __libc_back_signalInheritPack(__LIBC_PSPMINHSIG *ppSig, size_t *pcbSig)
{
    /*
     * Allocate a packet.
     */
    __LIBC_PSPMINHSIG pSig = _hmalloc(sizeof(*pSig));
    if (!pSig)
        return -ENOMEM;

    /* acquire the sem. */
    int rc = __libc_back_signalSemRequest();

    /*
     * Fill the packet.
     */
    pSig->cb = sizeof(*pSig);
    __SIGSET_EMPTY(&pSig->SigSetIGN);
    unsigned iSignalNo;
    for (iSignalNo = 1; iSignalNo < __SIGSET_MAXSIGNALS; iSignalNo++)
        if (    iSignalNo != SIGCHLD
            &&  gaSignalActions[iSignalNo].__sigaction_u.__sa_handler == SIG_IGN)
            __SIGSET_SET(&pSig->SigSetIGN, iSignalNo);

    /*
     * Return
     */
    if (!rc)
        __libc_back_signalSemRelease();
    *ppSig = pSig;
    *pcbSig = sizeof(*pSig);
    return 0;
}



_FORK_CHILD1(0x0000000f, signalForkChild)

/**
 * Fork callback which initializes 16-bit signal handler in the child process.
 *
 * @returns 0 on success.
 * @returns -errno on failure.
 * @param   pForkHandle     Pointer to fork handle.
 * @param   enmOperation    Fork operation.
 */
static int signalForkChild(__LIBC_PFORKHANDLE pForkHandle, __LIBC_FORKOP enmOperation)
{
    LIBCLOG_ENTER("pForkHandle=%p enmOperation=%d\n", (void *)pForkHandle, enmOperation);
    int     rc;
    switch (enmOperation)
    {
        /*
         * Install the 16-bit signal handler if we were the LIBC having that installed
         * in the parent process.
         */
        case __LIBC_FORK_OP_FORK_CHILD:
            rc = 0;
            if (gfOS2V1Handler)
            {
                rc = DosSetSigHandler((PFNSIGHANDLER)__libc_back_signalOS2V1Handler16bit, NULL, NULL, SIGA_ACCEPT, SIG_PFLG_A);
                if (rc)
                {
                    LIBC_ASSERTM_FAILED("DosSetSigHandler failed with rc=%d\n", rc);
                    rc = -__libc_native2errno(rc);
                }
                ULONG cTimes = 0;
                DosSetSignalExceptionFocus(SIG_SETFOCUS, &cTimes);
            }
            break;

        default:
            rc = 0;
            break;
    }

    LIBCLOG_RETURN_INT(rc);
}


