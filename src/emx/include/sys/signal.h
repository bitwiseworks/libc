/* $Id: $ */
/** @file
 *
 * BSD compatible.
 *
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


#ifndef _SYS_SIGNAL_H_
#define _SYS_SIGNAL_H_

/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#include <sys/cdefs.h>
#include <sys/_types.h>
#include <machine/signal.h>


/*******************************************************************************
*   Defined Constants And Macros                                               *
*******************************************************************************/
/** @defgroup sys_signal_h_signo   Signal numbers.
 * @{
 */
#if __XSI_VISIBLE || __POSIX_VISIBLE
#define SIGHUP    1     /** POSIX: Hangup */
#endif
#define SIGINT    2     /** ANSI: Interrupt (Ctrl-C) */
#if __XSI_VISIBLE || __POSIX_VISIBLE
#define SIGQUIT   3     /** POSIX: Quit */
#endif
#define SIGILL    4     /** ANSI: Illegal instruction */
#if __XSI_VISIBLE  || __POSIX_VISIBLE >= 200112
#define SIGTRAP   5     /** POSIX: Single step (debugging) */
#endif
#define SIGABRT   6     /** ANSI: abort () */
#if __BSD_VISIBLE
#define SIGIOT SIGABRT  /** BSD 4.2 */
#define SIGEMT    7     /** BSD: EMT instruction */
#endif
#define SIGFPE    8     /** ANSI: Floating point */
#if __XSI_VISIBLE || __POSIX_VISIBLE
#define SIGKILL   9     /** POSIX: Kill process */
#endif
#if __XSI_VISIBLE || __POSIX_VISIBLE >= 200112
#define SIGBUS   10     /** BSD 4.2: Bus error */
#endif
#define SIGSEGV  11     /** ANSI: Segmentation fault */
#if __BSD_VISIBLE || __POSIX_VISIBLE >= 200112
#define SIGSYS   12     /** Invalid argument to system call */
#endif
#if __XSI_VISIBLE || __POSIX_VISIBLE
#define SIGPIPE  13     /** POSIX: Broken pipe. */
#define SIGALRM  14     /** POSIX: Alarm. */
#endif
#define SIGTERM  15     /** ANSI: Termination, process killed */
#if __XSI_VISIBLE || __POSIX_VISIBLE >= 200112
#define SIGURG   16     /** POSIX/BSD: urgent condition on IO channel */
#endif
#if __XSI_VISIBLE || __POSIX_VISIBLE
#define SIGSTOP  17     /** POSIX: Sendable stop signal not from tty. unblockable. */
#define SIGTSTP  18     /** POSIX: Stop signal from tty. */
#define SIGCONT  19     /** POSIX: Continue a stopped process. */
#define SIGCHLD  20     /** POSIX: Death or stop of a child process. (EMX: 18) */
#define SIGCLD SIGCHLD  /** System V */
#define SIGTTIN  21     /** POSIX: To readers pgrp upon background tty read. */
#define SIGTTOU  22     /** POSIX: To readers pgrp upon background tty write. */
#endif
#if __BSD_VISIBLE
#define SIGIO    23     /** BSD: Input/output possible signal. */
#endif
#if __XSI_VISIBLE
#define SIGPOLL  23     /** ??: Input/output possible signal. (Same as SIGIO.) */
#endif
#if __XSI_VISIBLE || __POSIX_VISIBLE >= 200112
#define SIGXCPU  24     /** BSD 4.2: Exceeded CPU time limit. */
#define SIGXFSZ  25     /** BSD 4.2: Exceeded file size limit. */
#endif
#if __XSI_VISIBLE
#define SIGVTALRM 26    /** BSD 4.2: Virtual time alarm. */
#endif
#if __XSI_VISIBLE || __POSIX_VISIBLE >= 200112
#define SIGPROF  27     /** BSD 4.2: Profiling time alarm. */
#endif
#if __BSD_VISIBLE
#define SIGWINCH 28     /** BSD 4.3: Window size change (not implemented). */
#define SIGINFO  29     /** BSD 4.3: Information request. */
#endif
#if __XSI_VISIBLE || __POSIX_VISIBLE
#define SIGUSR1  30     /** POSIX: User-defined signal #1 */
#define SIGUSR2  31     /** POSIX: User-defined signal #2 */
#endif
#define SIGBREAK 32     /** OS/2: Break (Ctrl-Break). (EMX: 21) */

#define SIGRTMIN 33     /** First real time signal. */
#define SIGRTMAX 63     /** Last real time signal (inclusive) */
#if __BSD_VISIBLE || defined __USE_MISC
#define _NSIG    (SIGRTMAX + 1)     /** Number of signals (exclusive). Valid range is 1 to (_NSIG-1). */
#define NSIG     _NSIG              /** See _NSIG. */
#define MAXSIG   SIGRTMAX           /** Max signal number, includsive. (Sun) */
#endif
/** @} */


/** @defgroup sys_signal_h_sighnd  Fake Signal Functions
 * @{
 */
#define SIG_ERR ((__sighandler_t *)-1)  /** Failure. */
#define SIG_DFL ((__sighandler_t *)0)   /** Default action. */
#define SIG_IGN ((__sighandler_t *)1)   /** Ignore signal. */
/* 2 is reserved for BSD internal use */
#if __BSD_VISIBLE || __POSIX_VISIBLE >= 200112 /** @todo check specs on SIG_HOLD. */
#define SIG_HOLD ((__sighandler_t *)3)
#endif
#ifdef __BSD_VISIBLE
#define SIG_ACK ((__sighandler_t *)4)   /** OS/2: Acknowledge a signal. */
#endif
/** @} */


/** @defgroup sys_signal_h_sigev    Signal Event Notification Types.
 * See struct sigevent.
 * @{
 */
#define SIGEV_NONE      0               /* No async notification */
#define SIGEV_SIGNAL    1               /* Generate a queued signal */
#define SIGEV_THREAD    2               /* Deliver via thread creation. */
/** @} */


/** @defgroup sys_signal_h_sigaction_flags  Signal Action Flags.
 * @{
 */
#if __XSI_VISIBLE
/** Take signal on a registerd stack_t. */
#define SA_ONSTACK          0x00000001
/** Restart system call on signal return. */
#define SA_RESTART          0x00000002
/** Reset signal handler to SIG_DFL when deliving the signal. */
#define SA_RESETHAND        0x00000004
/** Don't mask the signal which is being delivered. */
#define SA_NODEFER          0x00000010
/** Don't keep zombies around for wait*(). */
#define SA_NOCLDWAIT        0x00000020
/** Signal the handler with the full set of arguments. */
#define SA_SIGINFO          0x00000040
#endif
#if __XSI_VISIBLE || __POSIX_VISIBLE
/** Do not generate SIGCHLD on child stop. */
#define SA_NOCLDSTOP        0x00000008
#endif
/** Block the signal (OS/2 legacy mode). */
#define SA_ACK              0x10000000
/** EMX compatibility. */
#define SA_SYSV             SA_RESETHAND
/** @} */


#if __BSD_VISIBLE
/** #defgroup sys_signal_h_sigval_flags     Signal Value Flags.
 * BSD 4.3 compatibility.
 * @{
 */
#define SV_ONSTACK          SA_ONSTACK
/** opposite sense. */
#define SV_INTERRUPT        SA_RESTART
#define SV_RESETHAND        SA_RESETHAND
#define SV_NODEFER          SA_NODEFER
#define SV_NOCLDSTOP        SA_NOCLDSTOP
#define SV_SIGINFO          SA_SIGINFO
/** @} */
#endif


/** @defgroup   sys_signal_h_stack_t_flags  Signal Stack Flags. (ss_flags)
 * @{
 */
/** Current on alternative stack. */
#define SS_ONSTACK          0x00000001
/** Do not take signal on alternate stack. */
#define SS_DISABLE          0x00000004
/** @} */
/** Recommended size for an alternate signal stack. (See stack_t) */
#define SIGSTKSZ            (0x10000)



#if __BSD_VISIBLE || __POSIX_VISIBLE > 0 && __POSIX_VISIBLE <= 200112
/** Convert signo to a signal mask for use with sigblock(). */
#define sigmask(signo)          (1 << ((signo) - 1))
#endif

#if __BSD_VISIBLE
#define BADSIG          SIG_ERR
#endif

#if __POSIX_VISIBLE || __XSI_VISIBLE
/*
 * Flags for sigprocmask:
 */
/** @defgroup   sys_signal_h_sigprocmask_flags  Flags for sigprocmask().
 * @{
 */
/** Block specified signal set. */
#define SIG_BLOCK           1
/** Unblock specified signal set. */
#define SIG_UNBLOCK         2
/** Set specified signal set. */
#define SIG_SETMASK         3
#endif



/** @defgroup sys_signal_h_si_code  siginfo_t.si_code values.
 * @{
 */

/** @defgroup sys_signal_h_si_code_sigill   siginfo_t.si_code values - SIGILL
 * @{ */
/** Illegal opcode. */
#define ILL_ILLOPC      ((int)0x80001001)
/** Illegal operand. */
#define ILL_ILLOPN      ((int)0x80001002)
/** Illegal addressing mode. */
#define ILL_ILLADR      ((int)0x80001003)
/** Illegal trap. */
#define ILL_ILLTRP      ((int)0x80001004)
/** Privileged opcode. */
#define ILL_PRVOPC      ((int)0x80001005)
/** Privileged register. */
#define ILL_PRVREG      ((int)0x80001006)
/** Coprocessor error. */
#define ILL_COPROC      ((int)0x80001007)
/** Internal stack error. */
#define ILL_BADSTK      ((int)0x80001008)
/** @} */


/** @defgroup sys_signal_h_si_code_sigfpe   siginfo_t.si_code values - SIGILL
 * @{ */
/** see machine/trap.h. */
/** @} */


/** @defgroup sys_signal_h_si_code_sigsegv  siginfo_t.si_code values - SIGSEGV
 * @{ */
/** Address not mapped to object. */
#define SEGV_MAPERR     ((int)0x80003001)
/** Invalid permissions for mapped object. */
#define SEGV_ACCERR     ((int)0x80003002)
/** @} */


/** @defgroup sys_signal_h_si_code_sigbus  siginfo_t.si_code values - SIGBUS
 * @{ */
/** Invalid address alignment. */
#define BUS_ADRALN      ((int)0x80004001)
/** Nonexistent physical address. */
#define BUS_ADRERR      ((int)0x80004002)
/** Object-specific hardware error. */
#define BUS_OBJERR      ((int)0x80004003)
/** @} */


/** @defgroup sys_signal_h_si_code_sigtrap siginfo_t.si_code values - SIGTRAP
 * @{ */
/** Process breakpoint. */
#define TRAP_BRKPT      ((int)0x80005001)
/** Process trace trap. */
#define TRAP_TRACE      ((int)0x80005002)
/** @} */


/** @defgroup sys_signal_h_si_code_sigchild siginfo_t.si_code values - SIGCHLD
 * @{ */
/** Child has exited. */
#define CLD_EXITED      ((int)0x80006001)
/** Child has terminated abnormally and did not create a core file. */
#define CLD_KILLED      ((int)0x80006002)
/** Child has terminated abnormally and created a core file. */
#define CLD_DUMPED      ((int)0x80006003)
/** Traced child has trapped. */
#define CLD_TRAPPED     ((int)0x80006004)
/** Child has stopped. */
#define CLD_STOPPED     ((int)0x80006005)
/** Stopped child has continued. */
#define CLD_CONTINUED   ((int)0x80006006)
/** @} */	


/** @defgroup sys_signal_h_si_code_sigpoll siginfo_t.si_code values - SIGPOLL
 * @{ */
/** Data input available. */
#define POLL_IN         ((int)0x80007001)
/** Output buffers available. */
#define POLL_OUT        ((int)0x80007002)
/** Input message available. */
#define POLL_MSG        ((int)0x80007003)
/** I/O error. */
#define POLL_ERR        ((int)0x80007004)
/** High priority input available. */
#define POLL_PRI        ((int)0x80007005)
/** Device disconnected. */
#define POLL_HUP        ((int)0x80007006)
/** @} */


/** @defgroup sys_signal_h_siginfo_codes    Signal Info Codes (si_code).
 * @{ */
#if __POSIX_VISIBLE || __XSI_VISIBLE
/** Sent by kill() or raise(). */
#define SI_USER             ((int)0x00010001)
/** Sent by sigqueue(). */
#define SI_QUEUE            ((int)0x00010002)
/** Sent cause a timer expired. */
#define SI_TIMER            ((int)0x00010003)
/** Sent upon AIO completion. */
#define SI_ASYNCIO          ((int)0x00010004)
/** Sent upon real time mesq state change. */
#define SI_MESGQ            ((int)0x00010005)
#endif
#if __BSD_VISIBLE
/** No idea. */
#define SI_UNDEFINED        ((int)0x00000000)
#endif
/** @} */

/** @} */


/*******************************************************************************
*   Structures and Typedefs                                                    *
*******************************************************************************/
#if __POSIX_VISIBLE || __XSI_VISIBLE
#ifndef _SIGSET_T_DECLARED
#define _SIGSET_T_DECLARED
/** Signal set. */
typedef __sigset_t sigset_t;
#endif
#endif


#if __POSIX_VISIBLE >= 199309 || __XSI_VISIBLE >= 500
/** Type for a value associated with a signal. */
typedef union sigval
{
    /** Interger value. */
    int     sival_int;
    /** Pointer value. */
    void   *sival_ptr;
    /** FreeBSD names - probably a typo in the FreeBSD, but anyway.
     * @{ */
    int     sigval_int;
    void   *sigval_ptr;
    /** @} */
} sigval_t;
#endif


#if __POSIX_VISIBLE >= 199309
/** Signal event. */
typedef struct sigevent
{
    /** Notification type. */
    int             sigev_notify;
    /** Signal number. */
    int             sigev_signo;
    /** Signal value. */
    union sigval    sigev_value;
    /** Thread Function. */
    void          (*sigev_notify_function)(sigval_t);
    /** Thread Attributes. */
    void           *sigev_notify_attributes;
} sigevent_t;
#endif


#if __XSI_VISIBLE || __POSIX_VISIBLE >= 199309
/**
 * Signal info.
 */
typedef struct __siginfo
{
    /** Signal number. */
    int             si_signo;
    /** Associated errno. */
    int             si_errno;
    /** Signal code. (See SI_* and FPE_* macros.) */
    int             si_code;
    /** LIBC Extension: Timestamp when the signal was generated. */
    unsigned        si_timestamp;
    /** LIBC Extension: Flags. __LIBC_SI_* */
    unsigned        si_flags;
    /** Process sending the signal. */
    __pid_t         si_pid;
    /** LIBC Extension: the program group of the sender. */
    unsigned        si_pgrp;
    /** LIBC Extension: Thread sending the signal. */
    unsigned        si_tid;
    /** User sending the signal (real uid). */
    __uid_t         si_uid;
    /** Exit value. (SIGCHLD) */
    int             si_status;
    /** Pointer to the faulting instruction or memory reference. (SIGSEGV, SIGILL, SIGFPE, SIGBUS) */
    void           *si_addr;
    /** Signal value. */
    union sigval    si_value;
    /** Band event for SIGPOLL. */
    long            si_band;
    /** Filehandle for SIGPOLL. */
    int             si_fd;
    /** Reserve a little bit for future usage. */
    unsigned        auReserved[6];
} siginfo_t;

#ifdef __BSD_VISIBLE
/** Signals LIBC flags.
 * @{ */
/** If set the signal was queue. */
#define __LIBC_SI_QUEUED            0x00000001
/** Internal signal generated by LIBC. */
#define __LIBC_SI_INTERNAL          0x00000002
/** Don't notify the child wait facilities. (Signal origins there.) */
#define __LIBC_SI_NO_NOTIFY_CHILD   0x00000004
/** @} */
#endif
#endif

/** ANSI C signals handler. */
typedef void __sighandler_t(int);
/** POSIX Signal info handler. */
typedef	void __siginfohandler_t(int, struct __siginfo *, void *);

#ifdef __BSD_VISIBLE
/** BSD 4.4 type. */
typedef __sighandler_t *sig_t;
#endif
#ifdef __USE_GNU
/** GLIBC makes this symbol available  */
typedef __sighandler_t *sighandler_t;
#endif

#if __POSIX_VISIBLE || __XSI_VISIBLE
struct __siginfo;

/** Signal action structure for sigaction(). */
struct sigaction
{
    /** Two handler prototypes, __sa_handler is the exposed one,
     * while __sa_sigaction is the one used internally. The two extra
     * arguments will not cause any trouble because of the nature of
     * __cdecl on i386.
     */
    union
    {
        /** New style handler. First arg is signal number, second is signal
         * info, third is signal specific (I think). */
        __siginfohandler_t *__sa_sigaction;
        /** Old style handler. First arg is signal number. */
        __sighandler_t     *__sa_handler;
    }  __sigaction_u;
    /** Signal mask to apply when executing the action. */
    sigset_t    sa_mask;
    /** Signal action flags. (See SA_* macros.) */
    int         sa_flags;
};
#define sa_handler      __sigaction_u.__sa_handler
#define sa_sigaction    __sigaction_u.__sa_sigaction
#endif


#if __BSD_VISIBLE
/** BSD 4.3 compatibility.
 * This structure is identical to sigaction.
 */
struct sigvec
{
    /** See sa_sigaction in struct sigaction. */
    __sighandler_t *sv_handler;
    /** See sa_mask in struct sigaction. */
    int             sv_mask;
    /** See sa_flags in struct sigaction. */
    int             sv_flags;
};
#define sv_onstack          sv_flags
#endif


#if __XSI_VISIBLE
/*
 * Structure used in sigaltstack call.
 */
#if __BSD_VISIBLE
typedef struct sigaltstack
#else
typedef struct
#endif
{
    /** Stack base pointer. */
    void       *ss_sp;
    /** Stack size. */
    __size_t    ss_size;
    /** Flags. (SS_DISABLE and/or SS_ONSTACK) */
    int         ss_flags;
} stack_t;
#endif


#if __XSI_VISIBLE
/** Structure for sigstack(). obsolete. */
struct sigstack
{
    /** Pointer to stack base. */
    void       *ss_sp;
    /** On stack indicator. Non zero if executing on this stack. */
    int         ss_onstack;
};
#endif


/*******************************************************************************
*   Functions                                                                  *
*******************************************************************************/

__BEGIN_DECLS
__sighandler_t *signal(int, __sighandler_t *);
__sighandler_t *bsd_signal(int, __sighandler_t *);
__sighandler_t *_signal_sysv(int, __sighandler_t *);
__sighandler_t *_signal_os2(int, __sighandler_t *);

/** @define __LIBC_SIGNAL_SYSV
 * #define __LIBC_SIGNAL_SYSV to use System V style signal. */
#ifdef __LIBC_SIGNAL_SYSV
static inline __sighandler_t *signal(int iSignalNo, __sighandler_t *pfnHandler)
{ return _signal_sysv(iSignalNo, pfnHandler); }
#endif

/** @define __LIBC_SIGNAL_OS2
 * #define __LIBC_SIGNAL_OS2 to use System V style signal. */
#ifdef __LIBC_SIGNAL_OS2
static inline __sighandler_t *signal(int iSignalNo, __sighandler_t *pfnHandler)
{ return _signal_os2(iSignalNo, pfnHandler); }
#endif

__END_DECLS


#if 0  /** @todo various emx stuff. */
#define SIGPTRACENOTIFY 128     /* Notification from ptrace() */
#define SIGTY           void
#endif

#endif /* not _SYS_SIGNAL_H */

