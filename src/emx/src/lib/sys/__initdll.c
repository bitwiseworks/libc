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
#include <klibc/startup.h>

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

int     __libc_gfkLIBCArgs;
void   *__libc_gpTmpEnvArgs;
char   *__libc_gpBigEnv;

#define __LIBC_KLIBCARGS_ERROR_SHMEMHEX 0x01
#define __LIBC_KLIBCARGS_ERROR_SHMEMGET 0x02

static int __libc_scanenv_fErr;
static void *__libc_scanenv_pErr;


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
    const char         *psz;

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
     * Force big environment and command line processing if not yet done.
     */
    __libc_scanenv(NULL, NULL);
    if (__libc_gfkLIBCArgs)
    {
        LIBCLOG_MSG2("Got kLIBC-style command line arguments\n");
        if (__libc_scanenv_fErr)
        {
            if (__libc_scanenv_fErr & __LIBC_KLIBCARGS_ERROR_SHMEMHEX)
                LIBC_ASSERTM_FAILED("__libc_scanenv: Shared memory addr string is not hex: \"%s\"\n", (char*)__libc_scanenv_pErr);
            if (__libc_scanenv_fErr & __LIBC_KLIBCARGS_ERROR_SHMEMGET)
                LIBC_ASSERTM_FAILED("__libc_scanenv: DosGetSharedMem(%p) returned %d\n", __libc_gpTmpEnvArgs, (int)__libc_scanenv_pErr);
            else
                LIBC_ASSERTM_FAILED("__libc_scanenv: Unknown error %x\n", __libc_scanenv_fErr);
        }
        else if (__libc_gpTmpEnvArgs)
        {
            size_t szEnv = *(size_t*)__libc_gpTmpEnvArgs;
            LIBCLOG_MSG2("Got shared mem for big env & args %p (env size %d, args size %d)\n",
                         __libc_gpTmpEnvArgs, szEnv, *(size_t*)(__libc_gpTmpEnvArgs + sizeof(size_t) + szEnv));
            if (szEnv)
            {
                /* __spawnve passes "\0" as pszEnv to DosExecPgm in this case */
                LIBC_ASSERTM(!*fibGetEnv(), "native OS/2 env doesn't point to '\\0'");

                /*
                * _sys_init_environ does not make copies of environment variables,
                * allocate a private storage for all of them.
                */
                __libc_gpBigEnv = _hmalloc(szEnv);
                if (!__libc_gpBigEnv)
                {
                    LIBC_ASSERTM_FAILED("_hmalloc(%d) failed\n", szEnv);
                    return -1;
                }
                memcpy(__libc_gpBigEnv, __libc_gpTmpEnvArgs + sizeof(size_t), szEnv);
            }
        }
    }

    /*
     * Setup environment (org_environ and _STD(environ))
     */
    rc = _sys_init_environ(__libc_gpBigEnv ? __libc_gpBigEnv : fibGetEnv());
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
    psz = getenv("LIBC_HOOK_DLLS");
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


static int is_klibc_arg_signature(const char *psz)
{
    return (    psz[0] == __KLIBC_ARG_SIGNATURE[0]
            &&  psz[1] == __KLIBC_ARG_SIGNATURE[1]
            &&  psz[2] == __KLIBC_ARG_SIGNATURE[2]
            &&  psz[3] == __KLIBC_ARG_SIGNATURE[3]
            &&  psz[4] == __KLIBC_ARG_SIGNATURE[4]
            &&  psz[5] == __KLIBC_ARG_SIGNATURE[5]
            &&  psz[6] == __KLIBC_ARG_SIGNATURE[6]
            &&  psz[7] == __KLIBC_ARG_SIGNATURE[7]);
}


/**
 * Internal DosScanEnv version that supports scanning a "big" environment string
 * passed by __spawnve via shared memory in kLIBC-style command line arguments
 * or a "normal" OS/2-maintained environment string otherwise. It doesn't use
 * any other LIBC functions so is reentrant.
 */
int __libc_scanenv(const char *pszName, const char **ppszValue)
{
    static int fInitialized = 0;

    const char *pszEnv = __libc_gpBigEnv;
    do
    {
        /* Short route (already got a big env) */
        if (pszEnv)
            break;

        /*
         * We may be called very early when __libc_GpFIBLIS is not yet
         * initialized by __libc_back_fibInit (e.g. from
         * __libc_ForkRegisterModule which is the first LIBC call after the DLL
         * has been loaded and it already wants logging which scans the
         * environment for LIBC_LOGGING). This will make fibGetEnv() and
         * fibGetCmdLine() immediately trap. To work around this, we use
         * DosGetInfoBlocks() directly instead.
         */

        if (fInitialized)
        {
            if (__libc_gpTmpEnvArgs && *(size_t*)__libc_gpTmpEnvArgs)
            {
                /* Short route 2 (got a big env but not copied it yet) */
                pszEnv = __libc_gpTmpEnvArgs + sizeof(size_t);
                break;
            }
            if (__libc_GpFIBPIB)
            {
                /* Short route 3 (no big en but already got fib data) */
                pszEnv = fibGetEnv();
                break;
            }
        }

        PPIB pPib = NULL;
        FS_VAR();
        FS_SAVE_LOAD();
        DosGetInfoBlocks(NULL, &pPib);
        FS_RESTORE();

        if (fInitialized)
        {
            /* Short route 4 (no big env) */
            pszEnv = pPib->pib_pchenv;
            break;
        }

        /*
         * Long route: check if we are given kLIBC-style command line arguments
         * and set up shared memory access to see if there is a big env in it.
         */

        fInitialized = 1;

        const char *pszArgs = pPib->pib_pchcmd;
        if (!*pszArgs)
            break;

        if (is_klibc_arg_signature(pszArgs))
        {
            __libc_gfkLIBCArgs = 2; /* argv[0] is the signature */
        }
        else
        {
            /* Skip argv[0] */
            while (*pszArgs++);

            if (is_klibc_arg_signature(pszArgs))
                __libc_gfkLIBCArgs = 1; /* argv[1] is the signature */
        }

        if (__libc_gfkLIBCArgs)
        {
            /* Got kLIBC-style command line arguments */
            pszArgs += sizeof(__KLIBC_ARG_SIGNATURE);
            if ((unsigned)*pszArgs & __KLIBC_ARG_SHMEM)
            {
                /*
                 * Got a shared memory block with env & args from __spawnve.
                 * Note that __libc_gpTmpEnvArgs will be freed in __init.c.
                 */

                /* Skip the flag byte */
                const char *psz = ++pszArgs;
                unsigned n = 0;

                /* Convert from hex (no prefixes) */
                while (*psz)
                {
                    char ch = *psz;
                    unsigned d;
                    if (ch >= '0' && ch <= '9')
                        d = ch - '0';
                    else if (ch >= 'A' && ch <= 'F')
                        d = ch - 'A' + 10;
                    else if (ch >= 'a' && ch <= 'f')
                        d = ch - 'a' + 10;
                    else
                    {
                        __libc_scanenv_fErr |= __LIBC_KLIBCARGS_ERROR_SHMEMHEX;
                        __libc_scanenv_pErr = (void*)pszArgs;
                        return -EFAULT;
                    }
                    n <<= 4;
                    n |= d;
                    ++psz;
                }

                __libc_gpTmpEnvArgs = (void*)n;
                int rc = DosGetSharedMem(__libc_gpTmpEnvArgs, PAG_READ);
                if (rc)
                {
                    __libc_scanenv_fErr |= __LIBC_KLIBCARGS_ERROR_SHMEMGET;
                    __libc_scanenv_pErr = (void*)rc;
                    return -EFAULT;
                }

                size_t szEnv = *(size_t*)__libc_gpTmpEnvArgs;
                if (szEnv)
                    pszEnv = __libc_gpTmpEnvArgs + sizeof(size_t);
            }
        }

        if (!pszEnv)
        {
            /* either no shared memory block or no big env in it */
            pszEnv = pPib->pib_pchenv;
        }
    }
    while (0);

    if (!pszName || !ppszValue)
        return -EINVAL;

    while (*pszEnv)
    {
        const char *psz = pszName;
        while (*psz != '=' && *pszEnv == *psz)
        {
            ++pszEnv;
            ++psz;
        }

        if (*psz == '=')
            return -EINVAL;
        if (*psz == '\0' && *pszEnv == '=')
        {
            ++pszEnv;
            if (!*pszEnv)
                break;
            *ppszValue = pszEnv;
            return 0;
        }

        while (*pszEnv)
            ++pszEnv;
        ++pszEnv;
    }

    return -ENOENT;
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

