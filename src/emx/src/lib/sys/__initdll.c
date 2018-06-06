/* $Id: __initdll.c 2021 2005-06-13 02:16:10Z bird $ */
/** @file

    Dynamic library low-level initialization routine.

    Copyright (c) 1992-1998 by Eberhard Mattes
    Copyright (c) 2003 InnoTek Systemberatung GmbH

    This routine is called from dll0.o. It should perform
    all kinds of low-level initialization required by a DLL.
*/


/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#include "libc-alias.h"
#define INCL_DOS
#define INCL_FSMACROS
#include <os2emx.h>
#define _osmajor __osminor
#define _osminor __osminor
#include <stdlib.h>
#undef _osmajor
#undef _osminor
#include <string.h>
#include <errno.h>
#include <sys/builtin.h>
#include <sys/fmutex.h>
#include <emx/startup.h>
#include <emx/syscalls.h>
#include <emx/umalloc.h>
#include <alloca.h>
#include <InnoTekLIBC/fork.h>
#include "syscalls.h"
#include "b_fs.h"
#include "b_signal.h"
#include "backend.h"
#include <InnoTekLIBC/thread.h>
#include <InnoTekLIBC/libc.h>
#include <InnoTekLIBC/backend.h>
#include <InnoTekLIBC/FastInfoBlocks.h>
#define __LIBC_LOG_GROUP    __LIBC_LOG_GRP_INITTERM
#include <InnoTekLIBC/logstrict.h>

/* Make this function an weak external. */
#pragma weak __init_largefileio


/*******************************************************************************
*   Structures and Typedefs                                                    *
*******************************************************************************/
/**
 * Argument to initdllForkTLM().
 */
typedef struct TLMARGS_s
{
    /** Pointer to the TLM entry. */
    PULONG          pTLM;
    /** Pointer to the current thread. */
    __LIBC_PTHREAD  pCurThread;
} TLMARGS, *PTLMARGS;


/*******************************************************************************
*   Global Variables                                                           *
*******************************************************************************/
int                 __libc_gfNoUnix;
__LIBC_PPTHREAD     __libc_gpTLS;

extern unsigned char _osminor;
extern unsigned char _osmajor;


/*******************************************************************************
*   Internal Functions                                                         *
*******************************************************************************/
static int initdllForkParent1(__LIBC_PFORKHANDLE pForkHandle, __LIBC_FORKOP enmOperation);
static int initdllForkChild1(__LIBC_PFORKHANDLE pForkHandle, __LIBC_FORKOP enmOperation);
static int initdllForkTLM(__LIBC_PFORKHANDLE pForkHandle, void *pvArg, size_t cbArg);


/**
 * Common init code for crt0 and dll0.
 * This should perhaps be a part of _CRT_init.
 *
 * @param   fFlags  Bit 0: If set the application is open to put the default heap in high memory.
 *                         If clear the application veto against putting the default heap in high memory.
 *                  Bit 1: If set some of the unixness of LIBC is disabled.
 *                         If clear all unix like features are enabled.
 *                  Passed on to __init_dll().
 * @param   hmod    The handle of the calling module. This is 0 when we're called from crt0.s.
 *                  If we're not already initialized, this will be the handle of the libc dll.
 */
int __init_dll(int fFlags, unsigned long hmod)
{
    ULONG               aul[2];
    int                 rc;
    __LIBC_PSPMPROCESS  pSelf;

    static int fInitialized = 0;

    /*
     * Process flags.
     */
    __libc_HeapVote(fFlags & 1);
    if (fFlags & 2)
        __libc_gfNoUnix = 1;


    /*
     * The rest must only be executed once.
     */
    if (fInitialized)
        return 0;
    fInitialized = 1;

    /*
     * Init fast infoblocks.
     */
    rc = __libc_back_fibInit(0);
    if (rc)
        return -1;

    /*
     * Initialize _osmajor and _osminor.
     */
    DosQuerySysInfo(QSV_VERSION_MAJOR, QSV_VERSION_MINOR, &aul[0], sizeof(aul));
    _osminor = (unsigned char)aul[1];
    _osmajor = (unsigned char)aul[0];

    /*
     * Check for high memory (>512MB) support.
     */
    _sys_gcbVirtualAddressLimit = 0;
    if (    !DosQuerySysInfo(QSV_VIRTUALADDRESSLIMIT, QSV_VIRTUALADDRESSLIMIT, &aul[0], sizeof(aul[0]))
        &&  aul[0] > 512
        &&  (   (_osmajor == 20 && _osminor >= 40)
             || _osmajor > 20 /* yeah, sure! */) )
        _sys_gcbVirtualAddressLimit = aul[0] * 1024*1024;

    /*
     * Get the process ID and parent process ID.
     * This is also required for (stand-alone) DLLs. At least, we need ptib
     * and ppib.
     */
    _sys_pid = fibGetPid();
    _sys_ppid = fibGetPPid();

    /*
     * Initialize the heap semaphores.
     */
    if (_fmutex_create2(&_sys_heap_fmutex, 0, "LIBC SYS Heap Mutex") != 0)
        return -1;
    if (_fmutex_create2(&_sys_gmtxHimem,   0, "LIBC SYS Highmem Mutex") != 0)
        return -1;
    if (_fmutex_create2(&__libc_gmtxExec,  0, "LIBC SYS Exec Mutex") != 0)
        return -1;

    /*
     * Initialize TLS.
     */
    rc = DosAllocThreadLocalMemory(1, (PULONG*)(void*)&__libc_gpTLS);
    if (rc)
    {
        LIBC_ASSERTM_FAILED("DosAllocThreadLocalMemory() failed. rc=%d\n", rc);
        return -1;
    }

    /*
     * Setup environment (org_environ and _STD(environ))
     */
    rc = _sys_init_environ(fibGetEnv());
    if (rc)
    {
        LIBC_ASSERTM_FAILED("_sys_init_environ() failed\n");
        return -1;
    }

    /*
     * Get the current process.
     */
    pSelf = __libc_spmSelf();
    if (!pSelf)
    {
        LIBC_ASSERTM_FAILED("__libc_spmSelf() failed\n");
        return -1;
    }

    /*
     * Init the file system department.
     */
    if (__libc_back_fsInit())
    {
        LIBC_ASSERTM_FAILED("__libc_back_fsInit() failed\n");
        return -1;
    }

    /*
     * Init file handles.
     */
    rc = __libc_fhInit();
    if (rc)
    {
        LIBC_ASSERTM_FAILED("__libc_fhInit() failed\n");
        return -1;
    }

    /*
     * Init signals
     */
    rc = __libc_back_signalInit();
    if (rc)
    {
        LIBC_ASSERTM_FAILED("__libc_back_signalInit() failed\n");
        return -1;
    }

    /*
     * Load and call hook DLLs.
     */
    const char *psz = getenv("LIBC_HOOK_DLLS");
    if (psz)
        __libc_back_hooksInit(psz, hmod);

    /*
     * Get current time for clock() for use as process startup time.
     */
    _sys_clock0_ms = fibGetMsCount();
    return 0;
}


/**
 * Get the current MS timestamp.
 * @param   pms     Where to store the current timestamp.
 */
void _sys_get_clock(unsigned long *pms)
{
    *pms = fibGetMsCount();
}



#undef  __LIBC_LOG_GROUP
#define __LIBC_LOG_GROUP    __LIBC_LOG_GRP_FORK

_FORK_PARENT1(0xffffffff, initdllForkParent1)

/**
 * Fork callback which transfere the TLM to the child.
 *
 * @returns 0 on success.
 * @returns -errno on failure.
 * @param   pForkHandle     Pointer to fork handle.
 * @param   enmOperation    Fork operation.
 */
int initdllForkParent1(__LIBC_PFORKHANDLE pForkHandle, __LIBC_FORKOP enmOperation)
{
    LIBCLOG_ENTER("pForkHandle=%p enmOperation=%d\n", (void *)pForkHandle, enmOperation);
    int     rc;
    switch (enmOperation)
    {
        case __LIBC_FORK_OP_EXEC_PARENT:
        {
            TLMARGS Args;
            Args.pTLM       = (PULONG)__libc_gpTLS;
            Args.pCurThread = *__libc_gpTLS;
            rc = pForkHandle->pfnInvoke(pForkHandle, initdllForkTLM, &Args, sizeof(Args));
            break;
        }

        default:
            rc = 0;
            break;
    }

    LIBCLOG_RETURN_INT(rc);
}


_FORK_CHILD1(0xffffffff, initdllForkChild1)

/**
 * Fork callback which updates the thread structure in the child
 * and the global pid & parent pid values.
 *
 * @returns 0 on success.
 * @returns -errno on failure.
 * @param   pForkHandle     Pointer to fork handle.
 * @param   enmOperation    Fork operation.
 */
static int initdllForkChild1(__LIBC_PFORKHANDLE pForkHandle, __LIBC_FORKOP enmOperation)
{
    LIBCLOG_ENTER("pForkHandle=%p enmOperation=%d\n", (void *)pForkHandle, enmOperation);
    int     rc;
    switch (enmOperation)
    {
        case __LIBC_FORK_OP_FORK_CHILD:
        {
            LIBCLOG_MSG("Adjusts thread nesting level. from %d to %d\n", (*__libc_gpTLS)->cDefLoggerDepth,
                        (*__libc_gpTLS)->cDefLoggerDepth - 2);
            (*__libc_gpTLS)->cDefLoggerDepth -= 2;
            _sys_pid = pForkHandle->pidChild;
            _sys_ppid = pForkHandle->pidParent;
            __libc_back_fibInit(1);
            _sys_clock0_ms = fibGetMsCount();
            rc = 0;
            break;
        }

        default:
            rc = 0;
            break;
    }

    LIBCLOG_RETURN_INT(rc);
}


/**
 * Callback which allocates the TLM entry which the parent is using.
 * and initializes it with the pointer to the thread used by the
 * parent. The global variable is not set, we must wait on the
 * data segment and heap duplication there.
 *
 * @returns 0 on success.
 * @returns -errno on failure.
 * @param   pForkHandle     Pointer to fork handle.
 * @param   pvArg           Pointer to a TLMARGS structure.
 * @param   cbArg           Size of argument pointed to by pvArg.
 */
static int initdllForkTLM(__LIBC_PFORKHANDLE pForkHandle, void *pvArg, size_t cbArg)
{
    LIBCLOG_ENTER("pForkHandle=%p pvArg=%p cbArg=%d\n", (void *)pForkHandle, pvArg, cbArg);
    PTLMARGS    pArgs = (PTLMARGS)pvArg;
    int         rc = 0;
    PULONG      apulTLM[64];
    int         iTLM;
    LIBC_ASSERT(cbArg == sizeof(TLMARGS));

    for (iTLM = 0; iTLM < sizeof(apulTLM) / sizeof(apulTLM[0]); iTLM++)
    {
        /*
         * Allocate.
         */
        rc = DosAllocThreadLocalMemory(1, &apulTLM[iTLM]);
        if (rc)
            break;

        /*
         * The right one?
         */
        if (apulTLM[iTLM] == pArgs->pTLM)
        {
            *apulTLM[iTLM] = (ULONG)pArgs->pCurThread;
            break;
        }
    }

    /*
     * Cleanup.
     */
    while (iTLM-- > 0)
        DosFreeThreadLocalMemory(apulTLM[iTLM]);

    if (!rc)
        LIBCLOG_RETURN_INT(0);
    LIBCLOG_RETURN_INT(-ENOMEM);
}

