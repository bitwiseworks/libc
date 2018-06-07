/* abort.c (emx+gcc) -- Copyright (c) 1990-1996 by Eberhard Mattes */

#include "libc-alias.h"
#include <stdlib.h>
#include <signal.h>
#include <emx/startup.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_SIGNAL
#include <InnoTekLIBC/logstrict.h>

static sig_atomic_t abort_flag = 0;

void _STD(abort)(void)
{
    LIBCLOG_ENTER("\n");
    sigset_t set;
    struct sigaction act;

    if (getenv("LIBC_BREAKPOINT_ABORT"))
        __asm__ __volatile__("int $3");


    /* Special handling if there's a signal-catching function installed
       and SIGABRT is not blocked: do not yet call _CRT_term(). */

    if (    sigaction(SIGABRT, NULL, &act) == 0
        &&  act.sa_handler != SIG_IGN && act.sa_handler != SIG_DFL
        &&  sigemptyset(&set) == 0
        &&  sigprocmask(SIG_SETMASK, NULL, &set) == 0
        &&  sigismember(&set, SIGABRT) == 0)
    {
        /* Raise SIGABRT.  Note that this is not guaranteed to terminate
           the process. */

        raise(SIGABRT);
    }

    /* Install SIG_DFL for SIGABRT. */

    act.sa_handler = SIG_DFL;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    sigaction(SIGABRT, &act, NULL);

    /* Clean up the C library only once. */

    LIBCLOG_MSG("abort_flag=%d\n", abort_flag);
    if (abort_flag++ == 0)
        _CRT_term();

    /* Unblock SIGABRT. */

    sigemptyset(&set);
    sigaddset(&set, SIGABRT);
    sigprocmask(SIG_UNBLOCK, &set, NULL);

    /* Raise SIGABRT again. */

    raise(SIGABRT);

    /* This should not get reached. */

    exit(3);
}
