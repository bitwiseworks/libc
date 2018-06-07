/* beginth.c (emx+gcc) -- Copyright (c) 1992-1998 by Eberhard Mattes */

#include "libc-alias.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#define INCL_DOSPROCESS
#define INCL_DOSERRORS
#define INCL_DOSEXCEPTIONS
#define INCL_FSMACROS
#include <os2emx.h>
#include <emx/syscalls.h>
#include <InnoTekLIBC/thread.h>
#include <InnoTekLIBC/backend.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_THREAD
#include <InnoTekLIBC/logstrict.h>
#include <386/builtin.h>

/**
 * Thread wrapper routine.
 *
 * @param   pThrd   Pointer to thread structure allocated for this thread.
 */
static void _System threadWrapper(__LIBC_PTHREAD pThrd)
{
    LIBCLOG_ENTER("pThrd=%p\n", (void *)pThrd);
    int                         tid;
    EXCEPTIONREGISTRATIONRECORD reg;
    FS_VAR();

    __libc_threadUse(pThrd);
    __libc_Back_threadStartup(&reg);

    LIBCLOG_MSG("calling thread function %p with arg %p\n", (void *)pThrd->u.startup.pfnStart, pThrd->u.startup.pvArg);
    pThrd->u.startup.pfnStart(pThrd->u.startup.pvArg);
    LIBCLOG_MSG("thread function returned\n");

    FS_SAVE_LOAD();
    __libc_threadTermination(0);
    tid = pThrd->tid;
    __libc_threadDereference(pThrd);
    __libc_Back_threadEnd(&reg);
    FS_SAVE_LOAD();
    LIBCLOG_MSG("calling DosExit(%s, 0)\n", tid == 1 ? "EXIT_PROCESS" : "EXIT_THREAD");
    for (;;)
        DosExit(tid == 1 ? EXIT_PROCESS : EXIT_THREAD, 0);
}


/* move me !! */
int __libc_back_threadCreate(void (*pfnStart)(void *), unsigned cbStack, void *pvArg, int fInternal)
{
    LIBCLOG_ENTER("pfnStart=%p cbStack=%d pvArg=%p fInternal=%d\n", (void *)pfnStart, cbStack, pvArg, fInternal);
    int             rc;
    TID             tid;
    __LIBC_PTHREAD  pThrd;
    FS_VAR();

    /*
     * Adjust the stack size, omit internal threads.
     */
    if (!fInternal)
    {
        /* Initialize the minimum stack mark on the first call run. */
        static size_t   cbStackMin;
        if (!cbStackMin)
        {
            int cb = 4096;
            _getenv_int("LIBC_THREAD_MIN_STACK_SIZE", &cb);
            __atomic_xchg((unsigned volatile *)&cbStackMin, cb);
        }

        /* adjust it */
        if (!cbStack)
            cbStack = 512*1024;             /* Default stack size is 512 KB. */
        if (cbStack < cbStackMin)
            cbStack = cbStackMin;
    }

    /*
     * Allocate a thread structure.
     */
    pThrd = __libc_threadAlloc();
    if (!pThrd)
        LIBCLOG_ERROR_RETURN_INT(-ENOMEM);

    /*
     * Set the startup thread info and create a new thread.
     */
    pThrd->fInternalThread    = fInternal;
    pThrd->u.startup.pfnStart = pfnStart;
    pThrd->u.startup.pvArg    = pvArg;
    FS_SAVE_LOAD();
    rc = DosCreateThread(&tid,
                         (PFNTHREAD)threadWrapper,
                         (ULONG)pThrd,
                         CREATE_READY | STACK_COMMITTED,
                         cbStack);
    FS_RESTORE();
    if (!rc)
        LIBCLOG_RETURN_INT((int)tid);

    /*
     * Set errno and cleanup.
     */
    LIBCLOG_ERROR("DosCreateThread failed with rc=%u cbStack=%u\n", rc, cbStack);
    if (rc == ERROR_NOT_ENOUGH_MEMORY)
        rc = -ENOMEM;
    else if (rc == ERROR_MAX_THRDS_REACHED)
        rc = -EAGAIN;
    else
        rc = -EINVAL;
    __libc_threadDereference(pThrd);
    LIBCLOG_ERROR_RETURN_INT(rc);
}

int _beginthread(void (*pfnStart)(void *), void *pvStack, unsigned cbStack, void *pvArg)
{
    LIBCLOG_ENTER("pfnStart=%p pvStack=%p cbStack=%d pvArg=%p\n", (void *)pfnStart, pvStack, cbStack, pvArg);
    int rc = __libc_back_threadCreate(pfnStart, cbStack, pvArg, 0);
    if (rc >= 0)
        LIBCLOG_RETURN_INT(rc);
    errno = -rc;
    LIBCLOG_ERROR_RETURN_INT(-1);
}


void _endthread(void)
{
    LIBCLOG_ENTER("\n");
    __LIBC_PTHREAD  pThrd = __libc_threadCurrent();
    int             tid = pThrd->tid;
    FS_VAR();

    __libc_threadTermination(0);
    __libc_threadDereference(pThrd);
    FS_SAVE_LOAD();
    LIBCLOG_MSG("calling DosExit(%s, 0)\n", tid == 1 ? "EXIT_PROCESS" : "EXIT_THREAD");
    for (;;)
        DosExit(tid == 1 ? EXIT_PROCESS : EXIT_THREAD, 0);
}
