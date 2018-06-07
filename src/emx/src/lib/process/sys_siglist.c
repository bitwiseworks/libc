/* $Id: sys_siglist.c 2304 2005-08-22 05:30:22Z bird $ */
/** @file
 *
 * LIBC - sys_nsig, sys_siglist, and sys_signame.
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
#include <signal.h>

/**
 * Number of signals described by sys_siglist and sys_signame.
 */
const int           sys_nsig = NSIG;

/**
 * Descriptive signal names
 */
const char * const  sys_siglist[NSIG] =
{
    "Signal 0",
    "Hangup",                           /* SIGHUP    1     /-- POSIX: Hangup */
    "Interrupt",                        /* SIGINT    2     /-- ANSI: Interrupt (Ctrl-C) */
    "Quit",                             /* SIGQUIT   3     /-- POSIX: Quit */
    "Illegal instruction",              /* SIGILL    4     /-- ANSI: Illegal instruction */
    "Trace/BPT trap",                   /* SIGTRAP   5     /-- POSIX: Single step (debugging) */
    "Abort trap",                       /* SIGABRT   6     /-- ANSI: abort () */
    "EMT trap",                         /* SIGEMT    7     /-- BSD: EMT instruction */
    "Floating point exception",         /* SIGFPE    8     /-- ANSI: Floating point */
    "Killed",                           /* SIGKILL   9     /-- POSIX: Kill process */
    "Bus error",                        /* SIGBUS   10     /-- BSD 4.2: Bus error */
    "Segmentation fault",               /* SIGSEGV  11     /-- ANSI: Segmentation fault */
    "Bad system call",                  /* SIGSYS   12     /-- Invalid argument to system call */
    "Broken pipe",                      /* SIGPIPE  13     /-- POSIX: Broken pipe. */
    "Alarm clock",                      /* SIGALRM  14     /-- POSIX: Alarm. */
    "Terminated",                       /* SIGTERM  15     /-- ANSI: Termination, process killed */
    "Urgent I/O condition",             /* SIGURG   16     /-- POSIX/BSD: urgent condition on IO channel */
    "Suspended (signal)",               /* SIGSTOP  17     /-- POSIX: Sendable stop signal not from tty. unblockable. */
    "Suspended",                        /* SIGTSTP  18     /-- POSIX: Stop signal from tty. */
    "Continued",                        /* SIGCONT  19     /-- POSIX: Continue a stopped process. */
    "Child exited",                     /* SIGCHLD  20     /-- POSIX: Death or stop of a child process. (EMX: 18) */
    "Stopped (tty input)",              /* SIGTTIN  21     /-- POSIX: To readers pgrp upon background tty read. */
    "Stopped (tty output)",             /* SIGTTOU  22     /-- POSIX: To readers pgrp upon background tty write. */
    "I/O possible",                     /* SIGIO    23     /-- BSD: Input/output possible signal. */
    "Cputime limit exceeded",           /* SIGXCPU  24     /-- BSD 4.2: Exceeded CPU time limit. */
    "Filesize limit exceeded",          /* SIGXFSZ  25     /-- BSD 4.2: Exceeded file size limit. */
    "Virtual timer expired",            /* SIGVTALRM 26    /-- BSD 4.2: Virtual time alarm. */
    "Profiling timer expired",          /* SIGPROF  27     /-- BSD 4.2: Profiling time alarm. */
    "Window size changes",              /* SIGWINCH 28     /-- BSD 4.3: Window size change (not implemented). */
    "Information request",              /* SIGINFO  29     /-- BSD 4.3: Information request. */
    "User defined signal 1",            /* SIGUSR1  30     /-- POSIX: User-defined signal #1 */
    "User defined signal 2",            /* SIGUSR2  31     /-- POSIX: User-defined signal #2 */
    "Ctrl-Break",                       /* SIGBREAK 32     /-- OS/2: Break (Ctrl-Break). (EMX: 21) */
#if NSIG >= 32
    "Real time signal 0",               /* SIGRTMIN +  0 */
    "Real time signal 1",               /* SIGRTMIN +  1 */
    "Real time signal 2",               /* SIGRTMIN +  2 */
    "Real time signal 3",               /* SIGRTMIN +  3 */
    "Real time signal 4",               /* SIGRTMIN +  4 */
    "Real time signal 5",               /* SIGRTMIN +  5 */
    "Real time signal 6",               /* SIGRTMIN +  6 */
    "Real time signal 7",               /* SIGRTMIN +  7 */
    "Real time signal 8",               /* SIGRTMIN +  8 */
    "Real time signal 9",               /* SIGRTMIN +  9 */
    "Real time signal 10",              /* SIGRTMIN + 10 */
    "Real time signal 11",              /* SIGRTMIN + 11 */
    "Real time signal 12",              /* SIGRTMIN + 12 */
    "Real time signal 13",              /* SIGRTMIN + 13 */
    "Real time signal 14",              /* SIGRTMIN + 14 */
    "Real time signal 15",              /* SIGRTMIN + 15 */
    "Real time signal 16",              /* SIGRTMIN + 16 */
    "Real time signal 17",              /* SIGRTMIN + 17 */
    "Real time signal 18",              /* SIGRTMIN + 18 */
    "Real time signal 19",              /* SIGRTMIN + 19 */
    "Real time signal 20",              /* SIGRTMIN + 20 */
    "Real time signal 21",              /* SIGRTMIN + 21 */
    "Real time signal 22",              /* SIGRTMIN + 22 */
    "Real time signal 23",              /* SIGRTMIN + 23 */
    "Real time signal 24",              /* SIGRTMIN + 24 */
    "Real time signal 25",              /* SIGRTMIN + 25 */
    "Real time signal 26",              /* SIGRTMIN + 26 */
    "Real time signal 27",              /* SIGRTMIN + 27 */
    "Real time signal 28",              /* SIGRTMIN + 28 */
    "Real time signal 29",              /* SIGRTMIN + 29 */
    "Real time signal 30",              /* SIGRTMIN + 30 == SIGRTMAX */
#endif
};


/**
 * Short signal names.
 */
const char * const  sys_signame[NSIG] =
{
    "Signal 0",                         /*           0 */
    "hup",                              /* SIGHUP    1     /-- POSIX: Hangup */
    "int",                              /* SIGINT    2     /-- ANSI: Interrupt (Ctrl-C) */
    "quit",                             /* SIGQUIT   3     /-- POSIX: Quit */
    "ill",                              /* SIGILL    4     /-- ANSI: Illegal instruction */
    "trap",                             /* SIGTRAP   5     /-- POSIX: Single step (debugging) */
    "abrt",                             /* SIGABRT   6     /-- ANSI: abort () */
    "emt",                              /* SIGEMT    7     /-- BSD: EMT instruction */
    "fpe",                              /* SIGFPE    8     /-- ANSI: Floating point */
    "kill",                             /* SIGKILL   9     /-- POSIX: Kill process */
    "bus",                              /* SIGBUS   10     /-- BSD 4.2: Bus error */
    "segv",                             /* SIGSEGV  11     /-- ANSI: Segmentation fault */
    "sys",                              /* SIGSYS   12     /-- Invalid argument to system call */
    "pipe",                             /* SIGPIPE  13     /-- POSIX: Broken pipe. */
    "alrm",                             /* SIGALRM  14     /-- POSIX: Alarm. */
    "term",                             /* SIGTERM  15     /-- ANSI: Termination, process killed */
    "urg",                              /* SIGURG   16     /-- POSIX/BSD: urgent condition on IO channel */
    "stop",                             /* SIGSTOP  17     /-- POSIX: Sendable stop signal not from tty. unblockable. */
    "tstp",                             /* SIGTSTP  18     /-- POSIX: Stop signal from tty. */
    "cont",                             /* SIGCONT  19     /-- POSIX: Continue a stopped process. */
    "chld",                             /* SIGCHLD  20     /-- POSIX: Death or stop of a child process. (EMX: 18) */
    "ttin",                             /* SIGTTIN  21     /-- POSIX: To readers pgrp upon background tty read. */
    "ttou",                             /* SIGTTOU  22     /-- POSIX: To readers pgrp upon background tty write. */
    "io",                               /* SIGIO    23     /-- BSD: Input/output possible signal. */
    "xcpu",                             /* SIGXCPU  24     /-- BSD 4.2: Exceeded CPU time limit. */
    "xfsz",                             /* SIGXFSZ  25     /-- BSD 4.2: Exceeded file size limit. */
    "vtalrm",                           /* SIGVTALRM 26    /-- BSD 4.2: Virtual time alarm. */
    "prof",                             /* SIGPROF  27     /-- BSD 4.2: Profiling time alarm. */
    "winch",                            /* SIGWINCH 28     /-- BSD 4.3: Window size change (not implemented). */
    "info",                             /* SIGINFO  29     /-- BSD 4.3: Information request. */
    "usr1",                             /* SIGUSR1  30     /-- POSIX: User-defined signal #1 */
    "usr2",                             /* SIGUSR2  31     /-- POSIX: User-defined signal #2 */
    "break",                            /* SIGBREAK 32     /-- OS/2: Break (Ctrl-Break). (EMX: 21) */
#if NSIG > 33
# if SIGRTMIN != 33
#  error SIGRTMIN != 33
# endif
    "rt0",                              /* SIGRT+ 0 33 */
    "rt1",                              /* SIGRT+ 1 34 */
    "rt2",                              /* SIGRT+ 2 35 */
    "rt3",                              /* SIGRT+ 3 36 */
    "rt4",                              /* SIGRT+ 4 37 */
    "rt5",                              /* SIGRT+ 5 38 */
    "rt6",                              /* SIGRT+ 6 39 */
    "rt7",                              /* SIGRT+ 7 40 */
    "rt8",                              /* SIGRT+ 8 41 */
    "rt9",                              /* SIGRT+ 9 42 */
    "rt10",                             /* SIGRT+10 43 */
    "rt11",                             /* SIGRT+11 44 */
    "rt12",                             /* SIGRT+12 45 */
    "rt13",                             /* SIGRT+13 46 */
    "rt14",                             /* SIGRT+14 47 */
    "rt15",                             /* SIGRT+15 48 */
    "rt16",                             /* SIGRT+16 49 */
    "rt17",                             /* SIGRT+17 50 */
    "rt18",                             /* SIGRT+18 51 */
    "rt19",                             /* SIGRT+19 52 */
    "rt20",                             /* SIGRT+20 53 */
    "rt21",                             /* SIGRT+21 54 */
    "rt22",                             /* SIGRT+22 55 */
    "rt23",                             /* SIGRT+23 56 */
    "rt24",                             /* SIGRT+24 57 */
    "rt25",                             /* SIGRT+25 58 */
    "rt26",                             /* SIGRT+26 59 */
    "rt27",                             /* SIGRT+27 60 */
    "rt28",                             /* SIGRT+28 61 */
    "rt29",                             /* SIGRT+29 62 */
    "rt30",                             /* SIGRT+30 63 == SIGRTMAX */
# if SIGRTMAX != 63
#  error SIGRTMIN != 63
# endif
# if NSIG != 64
#  error NSIG != 64
# endif
#endif /* NSIG > 33 */
};

