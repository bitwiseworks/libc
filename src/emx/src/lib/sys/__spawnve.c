/* sys/spawnve.c (emx+gcc) -- Copyright (c) 1992-1996 by Eberhard Mattes */

#include "libc-alias.h"
#include <stdlib.h>
#include <string.h>
#include <process.h>
#include <errno.h>
#include <alloca.h>
#include <386/builtin.h>
#include <sys/fmutex.h>
#define INCL_DOSPROCESS
#define INCL_FSMACROS
#define INCL_DOSERRORS
#include <os2emx.h>
#include "b_fs.h"
#include "b_signal.h"
#include "b_process.h"
#include <emx/syscalls.h>
#include <InnoTekLIBC/sharedpm.h>
#include <InnoTekLIBC/backend.h>
#include <klibc/startup.h>
#define __LIBC_LOG_GROUP    __LIBC_LOG_GRP_PROCESS
#include <InnoTekLIBC/logstrict.h>
#include "syscalls.h"


/*******************************************************************************
*   Global Variables                                                           *
*******************************************************************************/
/** When this flag is set, the exec / spawn backend will handle hash bang scripts. */
int __libc_Back_gfProcessHandleHashBangScripts = 1;
/** When this flag is set, the exec / spawn backend will handle PC batch scripts. */
int __libc_Back_gfProcessHandlePCBatchScripts = 1;


/**
 * Collects and creates the inherit stuff we send down to a LIBC child.
 * @returns Pointer to inherit structure on success.
 * @returns NULL and errno on failure.
 */
static __LIBC_PSPMINHERIT doInherit(void)
{
    LIBCLOG_ENTER("\n");
    __LIBC_PSPMINHERIT      pRet = NULL;
    size_t                  cbStrings = 0;
    char                   *pszStrings = NULL;

    /*
     * Get FH stuff.
     */
    size_t                  cbFH;
    __LIBC_PSPMINHFHBHDR    pFH;
    pFH = __libc_fhInheritPack(&cbFH, &pszStrings, &cbStrings);
    if (pFH)
    {
        /*
         * Get FS stuff.
         */
        size_t              cbFS;
        __LIBC_PSPMINHFS    pFS;
        size_t              cbFS2;
        __LIBC_PSPMINHFS2   pFS2;
        if (!__libc_back_fsInheritPack(&pFS, &cbFS, &pFS2, &cbFS2))
        {
            /*
             * Get Signal stuff
             */
            size_t              cbSig;
            __LIBC_PSPMINHSIG   pSig;
            if (!__libc_back_signalInheritPack(&pSig, &cbSig))
            {
                /*
                 * Allocate shared memory.
                 */
                #define ALIGNIT(cb) (((cb) + 3) & ~3)
                size_t  cb = sizeof(__LIBC_SPMINHERIT) + ALIGNIT(cbFH) + ALIGNIT(cbFS) + ALIGNIT(cbFS2) + ALIGNIT(cbSig) + ALIGNIT(cbStrings);
                pRet = __libc_spmAlloc(cb);
                if (pRet)
                {
                    pRet->cb = sizeof(*pRet);
                    char *p = (char *)(pRet + 1);

                    /* fh */
                    pRet->pFHBundles = (__LIBC_PSPMINHFHBHDR)p;
                    p += ALIGNIT(cbFH);
                    memcpy(pRet->pFHBundles, pFH, cbFH);
                    free(pFH);

                    /* fs */
                    if (pFS)
                    {
                        pRet->pFS = (__LIBC_PSPMINHFS)p;
                        p += ALIGNIT(cbFS);
                        memcpy(pRet->pFS, pFS, cbFS);
                        free(pFS);
                    }
                    else
                        pRet->pFS = NULL;

                    /* fs2 */
                    if (pFS2)
                    {
                        pRet->pFS2 = (__LIBC_PSPMINHFS2)p;
                        p += ALIGNIT(cbFS2);
                        memcpy(pRet->pFS2, pFS2, cbFS2);
                        free(pFS2);
                    }
                    else
                        pRet->pFS2 = NULL;

                    /* sig */
                    if (pSig)
                    {
                        pRet->pSig = (__LIBC_PSPMINHSIG)p;
                        p += ALIGNIT(cbSig);
                        memcpy(pRet->pSig, pSig, cbSig);
                        free(pSig);
                    }
                    else
                        pRet->pSig = NULL;

                    /* strings */
                    if (pszStrings)
                    {
                        pRet->pszStrings = p;
                        /*p += ALIGNIT(cbStrings);*/
                        memcpy(pRet->pszStrings, pszStrings, cbStrings);
                        free(pszStrings);
                    }
                    else
                        pRet->pszStrings = NULL;

                    /* done! */
                    LIBCLOG_RETURN_P(pRet);
                }
                free(pSig);
            }
            free(pFS);
            free(pFS2);
        }

        /* cleanup on failure */
        __libc_fhInheritDone();
        free(pFH);
    }
    free(pszStrings);

    LIBCLOG_RETURN_P(pRet);
}


/**
 * Cleans up any globale inherit stuff.
 */
static void doInheritDone(void)
{
    __libc_fhInheritDone();
}

/* Note: We are allowed to call _trealloc() as this module is not used
   in an .a library. */

#define ADD(n) do { \
  while (cbArgs + n > cbArgsBuf) \
    { \
      char *pszOld = pszArgsBuf; \
      cbArgsBuf += 512; \
      pszArgsBuf = _trealloc (pszArgsBuf, cbArgsBuf); \
      if (__predict_false(pszArgsBuf == NULL)) \
        { \
          _tfree (pszOld); \
          errno = ENOMEM; \
          LIBCLOG_RETURN_INT(-1); \
        } \
      pszArg = pszArgsBuf + cbArgs; \
    } \
  cbArgs += n; } while (0)


int __spawnve(struct _new_proc *np)
{
    LIBCLOG_ENTER("np=%p:{mode=%#x}\n", (void *)np, np->mode);
    FS_VAR();
    char szLineBuf[512];

    /*
     * Validate mode.
     */
    ULONG ulMode = np->mode;
    switch (ulMode & 0xff)
    {
        case P_WAIT:
        case P_NOWAIT:
        case P_OVERLAY:
            break;
        default:
            errno = EINVAL;
            LIBCLOG_ERROR_RETURN(-1, "ret -1 - invalid mode 0x%08lx\n", ulMode);
    }

    /*
     * Resolve the program name - prefere .exe over stubs.
     * The caller have left enough space for adding an .exe extension.
     */
    char *pszPgmName = (char *)np->fname_off;
    size_t cch = strlen((char *)np->fname_off);
    _defext(pszPgmName, "exe");
    char szNativePath[PATH_MAX];
    int rc = __libc_back_fsResolve(pszPgmName, BACKFS_FLAGS_RESOLVE_FULL, &szNativePath[0], NULL);
    if (rc)
    {
        if (pszPgmName[cch])
        {
            pszPgmName[cch] = '\0'; /* Drop the .exe bit added by _defext(). */
            rc = __libc_back_fsResolve(pszPgmName, BACKFS_FLAGS_RESOLVE_FULL, &szNativePath[0], NULL);
        }
        if (rc)
        {
            errno = -rc;
            LIBCLOG_ERROR_RETURN(-1, "ret -1 - Failed to resolve program name: '%s' rc=%d.\n", pszPgmName, rc);
        }
    }
    pszPgmName = &szNativePath[0];

    /*
     * cmd.exe and 4os2.exe needs different argument handling, and
     * starting with kLIBC 0.6.4 we can pass argv directly to LIBC
     * programs.
     * (1 == cmd or 4os2 shell, 0 == anything else)
     */
    enum { args_standard, args_cmd, args_unix } enmMethod = args_standard;
    char *psz = _getname(pszPgmName);
    if (   stricmp(psz, "cmd.exe") == 0
        || stricmp(psz, "4os2.exe") == 0)
        enmMethod = args_cmd;
    else
    {
        HFILE hFile = NULLHANDLE;
        ULONG ulAction = 0;
        rc = DosOpen((PCSZ)pszPgmName, &hFile, &ulAction, 0, FILE_NORMAL,
                     OPEN_ACTION_FAIL_IF_NEW | OPEN_ACTION_OPEN_IF_EXISTS,
                     OPEN_FLAGS_SEQUENTIAL | OPEN_FLAGS_NOINHERIT | OPEN_SHARE_DENYNONE | OPEN_ACCESS_READONLY,
                     NULL);
        if (!rc)
        {
            ULONG cbRead = 0;
            rc = DosRead(hFile, szLineBuf, sizeof(szLineBuf), &cbRead);
            DosClose(hFile);
            if (rc)
                LIBCLOG_ERROR("DosRead - rc=%d (%s)\n", rc, pszPgmName);
            else if (   cbRead >= __KLIBC_STUB_MIN_SIZE
                     && szLineBuf[0] == 'M'
                     && szLineBuf[1] == 'Z'
                     && !memcmp(&szLineBuf[__KLIBC_STUB_SIGNATURE_OFF], __KLIBC_STUB_SIGNATURE_BASE, sizeof(__KLIBC_STUB_SIGNATURE_BASE) - 1)
                    )
            {
                char const *pchVer = &szLineBuf[__KLIBC_STUB_SIGNATURE_OFF + sizeof(__KLIBC_STUB_SIGNATURE_BASE) - 1];
                if (*pchVer >= '0' && *pchVer <= '9')
                    enmMethod = !(np->mode & P_NOUNIXARGV) ? args_unix : args_standard;
            }
            /** @todo move the hash bang handling up here. */
            /*else if (!rc && szLineBuf[0] == '#')
            {
            } */
        }
        /* Catch some plain failures here, leave the rest for later. */
        else if (   rc == ERROR_FILE_NOT_FOUND
                 || rc == ERROR_PATH_NOT_FOUND)
        {
            _sys_set_errno(rc);
            LIBCLOG_RETURN_INT(-1);
        }
        else
            LIBCLOG_ERROR("rc=%d (%s)\n", rc, pszPgmName);
    }

    /*
     * Construct the commandline.
     */
    const char *pszSrc = (const char *)np->arg_off;
    char       *pszArgsBuf = NULL;
    size_t      cbArgsBuf = 0;
    char       *pszArg = NULL;
    size_t      cbArgs = 0;
    int         i;

    if (np->arg_count > 0)
    {
        ++pszSrc;                    /* skip flags byte */
        cch = strlen(pszSrc) + 1;
        ADD(cch);
        memcpy(pszArg, pszSrc, cch);
        pszArg += cch; pszSrc += cch;
    }
    if (enmMethod == args_unix)
    {
        /* first arg is the signature. */
        ADD(sizeof(__KLIBC_ARG_SIGNATURE));
        memcpy(pszArg, __KLIBC_ARG_SIGNATURE, sizeof(__KLIBC_ARG_SIGNATURE));
        pszArg += sizeof(__KLIBC_ARG_SIGNATURE);

        /* then comes the argument vector. */
        for (i = 1; i < np->arg_count; ++i)
        {
            unsigned char chFlags = *pszSrc++;
            chFlags &= __KLIBC_ARG_MASK;
            chFlags |= __KLIBC_ARG_ARGV;
            cch = strlen(pszSrc) + 1;
            ADD(cch + 1);
            *pszArg++ = chFlags;
            memcpy(pszArg, pszSrc, cch);
            pszArg += cch;
            pszSrc += cch;
        }

        /* the double termination. */
        ADD(1);
        *pszArg = '\0';
    }
    else
    {
        /* quote the arguments in emx / cmd.exe fashion. */
        for (i = 1; i < np->arg_count; ++i)
        {
            if (i > 1)
            {
                ADD(1);
                *pszArg++ = ' ';
            }
            ++pszSrc;                    /* skip flags byte */
            BOOL fQuote = FALSE;
            if (*pszSrc == 0)
                fQuote = TRUE;
            else if (ulMode & P_QUOTE)
            {
                if (pszSrc[0] == '@' && pszSrc[1] != 0)
                    fQuote = TRUE;
                else
                    for (psz = (char *)pszSrc; *psz != 0; ++psz)
                        if (*psz == '?' || *psz == '*')
                        {
                            fQuote = TRUE;
                            break;
                        }
            }
            if (!fQuote)
            {
                for (psz = (char *)pszSrc; *psz != 0; ++psz)
                    if (*psz == ' ' || *psz == '\t' || (*psz == '"' && enmMethod == args_cmd))
                    {
                        fQuote = TRUE;
                        break;
                    }
            }
            if (fQuote)
            {
                ADD(1);
                *pszArg++ = '"';
            }
            size_t bs = 0;
            while (*pszSrc != 0)
            {
                if (*pszSrc == '"' && enmMethod == args_standard)
                {
                    ++bs;
                    ADD(bs);
                    memset(pszArg, '\\', bs); pszArg += bs;
                    bs = 0;
                }
                else if (*pszSrc == '\\' && enmMethod == args_standard)
                    ++bs;
                else
                    bs = 0;
                ADD(1);
                *pszArg++ = *pszSrc;
                ++pszSrc;
            }
            if (fQuote)
            {
                ADD(1+bs);
                memset(pszArg, '\\', bs); pszArg += bs;
                *pszArg++ = '"';
            }
            ++pszSrc;
        }
        /* The arguments are an array of zero terminated strings, ending with an empty string. */
        ADD(2);
        *pszArg++ = '\0';
        *pszArg++ = '\0';
    }


    /*
     * Now create an embryo process.
     */
    _fmutex_request(&__libc_gmtxExec, 0);
    __LIBC_PSPMPROCESS pEmbryo = __libc_spmCreateEmbryo(getpid());
    if (pEmbryo)
    {
        /*
         * Do inheritance stuff.
         */
        pEmbryo->pInherit = doInherit();
        if (pEmbryo->pInherit)
        {
            RESULTCODES resc;
            char        szObj[40];

            /*
             * Create the process.
             */
            FS_SAVE_LOAD();
            LIBCLOG_MSG("Calling DosExecPgm pgm: %s args: %s\\0%s\\0\\0\n", pszPgmName, pszArgsBuf, pszArgsBuf + strlen(pszArgsBuf) + 1);
            rc = DosExecPgm(szObj, sizeof(szObj), EXEC_ASYNCRESULT, (PCSZ)pszArgsBuf, (PCSZ)np->env_off, &resc, (PCSZ)pszPgmName);
            int cTries = 3;
            while (     (   rc == ERROR_INVALID_EXE_SIGNATURE
                         || rc == ERROR_BAD_EXE_FORMAT)
                   &&   --cTries > 0)
            {
                /*
                 * This could be a batch, rexx or hash bang script.
                 * The first two is recognized by the filename extension, the latter
                 * requires inspection of the first line of the file.
                 */
                const char *pszInterpreter = NULL;
                const char *pszInterpreterArgs = NULL;
                if (    __libc_Back_gfProcessHandlePCBatchScripts
                    &&  (psz = _getext(pszPgmName))
                    &&  (!stricmp(psz, ".cmd") || !stricmp(psz, ".bat") || !stricmp(psz, ".btm")))
                {
                    pszInterpreterArgs = "/C";
                    pszInterpreter = getenv("COMSPEC");
                    if (!pszInterpreter)
                    {
                        pszInterpreter = getenv("OS2_SHELL");
                        if (!pszInterpreter)
                            pszInterpreter = stricmp(psz, ".btm") ? "cmd.exe" : "4os2.exe";
                    }

                    /* make sure the slashes in the script name goes the DOS way. */
                    psz = szNativePath;
                    while ((psz = strchr(szNativePath, '/')) != NULL)
                        *psz++ = '\\';
                }
                else if (__libc_Back_gfProcessHandleHashBangScripts)
                {
                    /*
                     * Read the first line of the file into szLineBuf and terminate
                     * it stripping trailing blanks.
                     */
                    HFILE hFile = NULLHANDLE;
                    ULONG ulAction = 0;
                    int rc2 = DosOpen((PCSZ)pszPgmName, &hFile, &ulAction, 0, FILE_NORMAL,
                                      OPEN_ACTION_FAIL_IF_NEW | OPEN_ACTION_OPEN_IF_EXISTS,
                                      OPEN_FLAGS_SEQUENTIAL | OPEN_FLAGS_NOINHERIT | OPEN_SHARE_DENYNONE | OPEN_ACCESS_READONLY,
                                      NULL);
                    if (!rc2)
                    {
                        ULONG cbRead = 0;
                        rc2 = DosRead(hFile, szLineBuf, sizeof(szLineBuf) - 1, &cbRead);
                        DosClose(hFile);
                        if (!rc2)
                        {
                            szLineBuf[cbRead < sizeof(szLineBuf) ? cbRead : sizeof(szLineBuf) - 1] = '\0';
                            psz = strpbrk(szLineBuf, "\r\n");
                            if (psz)
                            {
                                register char ch;
                                while ((ch = *--psz) == ' ' || ch == '\t')
                                    /* nothing */;
                                psz[1] = '\0';

                                /*
                                 * Check for '#[ \t]*!'
                                 */
                                psz = &szLineBuf[0];
                                if (*psz++ == '#')
                                {
                                    while ((ch = *psz) == ' ' || ch == '\t')
                                        psz++;
                                    if (*psz++ == '!')
                                    {
                                        while ((ch = *psz) == ' ' || ch == '\t')
                                            psz++;
                                        pszInterpreter = psz;

                                        /*
                                         * Find end of interpreter and start of potential arguments.
                                         * I've never seen quoted interpreter names, so we won't bother with that yet.
                                         */
                                        while ((ch = *psz) != ' ' && ch != '\t' && ch != '\0')
                                            psz++;
                                        if (ch)
                                        {
                                            *psz++ = '\0';
                                            while ((ch = *psz) == ' ' || ch == '\t')
                                                psz++;
                                            if (ch)
                                                pszInterpreterArgs = psz;
                                        }
                                    } /* if bang */
                                } /* if hash */
                            } /* if full line */
                        } /* if read */
                    } /* if open */
                }
                if (!pszInterpreter)
                    break;

                /*
                 * The original argv0 is replaced by the "program name".
                 * The interpreter and its optional arguments are then inserted in front of that.
                 *
                 * ASSUMES that the arguments and program name require no escaping.
                 * ASSUMES that enmMethod != args_unix, which is true at the time of writing.
                 */
                size_t offOldArg1         = strlen(pszArgsBuf) + 1;
                size_t cchPgmName         = strlen(pszPgmName);
                size_t cchInterpreter     = strlen(pszInterpreter);
                BOOL   fQuote             = strpbrk(pszPgmName, " \t") != NULL;
                int    cchInterpreterArgs = pszInterpreterArgs ? strlen(pszInterpreterArgs) : -1;
                cch = cchInterpreter + 1 + cchInterpreterArgs + 1 + cchPgmName + 2*fQuote + 1 - offOldArg1;

                /* Grow and shift the argument buffer. */
                size_t cbToMove = cbArgs; /* (ADD modifies cbArgs) */
                ADD(cch);
                memmove(pszArgsBuf + cch + offOldArg1, pszArgsBuf + offOldArg1, cbToMove - offOldArg1);

                /* New argv[0] = Interpreter name. */
                psz = pszArgsBuf;
                memcpy(psz, pszInterpreter, cchInterpreter);
                psz += cchInterpreter;
                *psz++ = '\0';

                /* Add arguments after that (if present). */
                if (pszInterpreterArgs)
                {
                    memcpy(psz, pszInterpreterArgs, cchInterpreterArgs);
                    psz += cchInterpreterArgs;
                    *psz++ = ' ';
                }

                /* Finally add the script name (previous szNativePath actually). */
                if (fQuote)
                    *psz++ = '"';
                memcpy(psz, pszPgmName, cchPgmName);
                psz += cchPgmName;
                if (fQuote)
                    *psz++ = '"';
                *psz++ = ' ';

                /*
                 * Resolve the interpreter name.
                 * Try the unchanged name first, then the name with .exe suffix,
                 * then the PATH.  In the latter case we skip the directory path
                 * if given, since we're frequently faced with path differences
                 * between OS/2 and the UNIX where the script originated.
                 */
                rc = __libc_back_fsResolve(pszInterpreter, BACKFS_FLAGS_RESOLVE_FULL, &szNativePath[0], NULL);
                if (rc)
                {
                    char szPath[PATH_MAX];
                    if (   _path2(pszInterpreter, ".exe", szPath, sizeof(szPath)) == 0
                        || (   (psz = _getname(pszInterpreter)) != pszInterpreter
                            && _path2(psz, ".exe", szPath, sizeof(szPath)) == 0) )
                        rc = __libc_back_fsResolve(szPath, BACKFS_FLAGS_RESOLVE_FULL, &szNativePath[0], NULL);
                    if (rc)
                    {
                        LIBCLOG_MSG("Failed to find interpreter '%s'! szPath='%s'\n", pszInterpreter, szPath);
                        break;
                    }
                }

                /*
                 * Try execute it.
                 * Note! pszPgmName = &szNativePath[0], so we're good here and if this loops.
                 */
                LIBCLOG_MSG("Calling DosExecPgm pgm: %s args: %s\\0%s\\0\\0\n", pszPgmName, pszArgsBuf, pszArgsBuf + strlen(pszArgsBuf) + 1);
                rc = DosExecPgm(szObj, sizeof(szObj), EXEC_ASYNCRESULT, (PCSZ)pszArgsBuf, (PCSZ)np->env_off, &resc, (PCSZ)pszPgmName);
            } /* while */
            FS_RESTORE();
            if (!rc)
            {
                __atomic_cmpxchg32((volatile uint32_t *)(void *)&pEmbryo->pid, (uint32_t)resc.codeTerminate, ~0);
                LIBCLOG_MSG("Spawned pid=%04lx (%ld)\n", resc.codeTerminate, resc.codeTerminate);

                /*
                 * If the service is running, notify it now. Otherwise wait until
                 * we're done waiting for the child finish inheriting us.
                 */
                int fDoneNotifyExec = __libc_back_processWaitNotifyAlreadyStarted();
                if (fDoneNotifyExec)
                    __libc_back_processWaitNotifyExec(resc.codeTerminate);

                /*
                 * Wait for the child to become active and complete the inhertining.
                 */
                int fDoneInherit = __libc_spmWaitForChildToBecomeAlive(pEmbryo);
                LIBCLOG_MSG("fDoneInherit=%d enmState=%d inh=%p,%p\n",
                            fDoneInherit, pEmbryo->enmState, pEmbryo->pInherit, pEmbryo->pInheritLocked);

                /*
                 * Do cleanups unless we're in an exec in which case we wish to
                 * get some more stuff done before we do the clean ups.
                 */
                if ((ulMode & 0xff) != P_OVERLAY)
                {
                    doInheritDone();
                    if (!fDoneNotifyExec)
                        __libc_back_processWaitNotifyExec(resc.codeTerminate);
                    __libc_spmRelease(pEmbryo);
                    _fmutex_release(&__libc_gmtxExec);
                }
                if (pszArgsBuf != NULL)
                    _tfree(pszArgsBuf);

                /*
                 * Exit depends on the mode.
                 */
                switch (ulMode & 0xff)
                {
                    /*
                     * Return the pid.
                     */
                    case P_NOWAIT:
                        LIBCLOG_RETURN_INT((int)resc.codeTerminate);

                    /*
                     * Wait for the child and return the result.
                     */
                    case P_WAIT:
                    {
                        pid_t pid = resc.codeTerminate;
                        LIBCLOG_MSG("Calling wait4(%d,,0,0)\n", pid);
                        int iStatus = 0;
                        pid_t pidEnded = wait4(pid, &iStatus, 0, NULL);
                        while (pidEnded < 0 && errno == EINTR)
                            pidEnded = wait4(pid, &iStatus, 0, NULL);
                        if (pidEnded > 0)
                        {
                            LIBCLOG_MSG("wait4(%d,,0,0) returned pidEnded=%d iStatus=%#x (%d)\n", pid, pidEnded, iStatus, iStatus);
                            LIBC_ASSERTM(pidEnded == pid, "Expected pid %d and got %d!\n", pid, pidEnded);
                            LIBCLOG_RETURN_INT(iStatus >> 8);
                        }

                        LIBC_ASSERTM_FAILED("Calling wait4(%d,,0,0) -> errno=%d\n", pid, errno);
                        LIBCLOG_RETURN_INT(-1);
                        break;
                    }

                    /*
                     * Wait for the child and exit this process with the same result.
                     */
                    case P_OVERLAY:
                    {
                        /*
                         * Ignore SIGCHLD signal.
                         */
                        /** @todo proxy job control */
                        bsd_signal(SIGCHLD, SIG_DFL);
                        bsd_signal(SIGSTOP, SIG_DFL);
                        bsd_signal(SIGTSTP, SIG_DFL);
                        bsd_signal(SIGCONT, SIG_DFL);

                        /*
                         * Wait a bit more for the child to catch on since we're going to
                         * close all file handles next and that we certainly screw up tcpip
                         * handles in the child if they aren't already passed along.
                         */
                        if (!fDoneInherit)
                        {
                            LIBCLOG_MSG("waiting some more...\n");
                            fDoneInherit = __libc_spmWaitForChildToBecomeAlive(pEmbryo);
                            LIBCLOG_MSG("fDoneInherit=%d enmState=%d inh=%p,%p\n",
                                        fDoneInherit, pEmbryo->enmState, pEmbryo->pInherit, pEmbryo->pInheritLocked);
                        }

                        if (!fDoneNotifyExec)
                            __libc_back_processWaitNotifyExec(resc.codeTerminate);
                        __libc_spmRelease(pEmbryo);
                        doInheritDone();
                        _fmutex_release(&__libc_gmtxExec);

                        /*
                         * Shut down the process...
                         */
                        _rmtmp();
                        __libc_fhExecDone();

                        /*
                         * Wait for the child to complete and forward stuff while doing so...
                         */
                        pid_t pid = resc.codeTerminate;
                        LIBCLOG_MSG("Calling __libc_Back_processWait(P_PID,%d,,WEXITED,NULL)\n", pid);
                        for (;;)
                        {
                            siginfo_t SigInfo = {0};
                            do
                                /** @todo proxy job control */
                                rc = __libc_Back_processWait(P_PID, pid, &SigInfo, WEXITED, NULL);
                            while (rc == -EINTR);
                            if (rc < 0)
                                break;
                            LIBCLOG_MSG("__libc_Back_processWait(P_PID,%d,,WEXITED,NULL) returned %d si_code=%d si_status=%#x (%d)\n",
                                        pid, rc, SigInfo.si_code, SigInfo.si_status, SigInfo.si_status);
                            LIBC_ASSERTM(SigInfo.si_pid == pid, "Expected pid %d and got %d!\n", pid, SigInfo.si_pid);
                            if (    SigInfo.si_code == CLD_STOPPED
                                ||  SigInfo.si_code == CLD_CONTINUED)
                            {
                                /* notify parent. */
                                /** @todo proxy job control */
                            }
                            else
                            {
                                /*
                                 * Terminate the process.
                                 */
                                int iStatus = SigInfo.si_status;
                                switch (SigInfo.si_code)
                                {
                                    default:
                                        LIBC_ASSERTM_FAILED("Invalid si_code=%#x si_status=%#x\n", SigInfo.si_code, SigInfo.si_status);
                                    case CLD_EXITED:
                                        __libc_spmTerm(__LIBC_EXIT_REASON_EXIT, iStatus);
                                        break;
                                    case CLD_KILLED:
                                        __libc_spmTerm(__LIBC_EXIT_REASON_SIGNAL_BASE + iStatus, 0);
                                        iStatus = 127;
                                        break;
                                    case CLD_DUMPED:
                                        if (iStatus == SIGSEGV || iStatus > SIGRTMAX || iStatus <= 0)
                                            __libc_spmTerm(__LIBC_EXIT_REASON_XCPT, 0);
                                        else
                                            __libc_spmTerm(__LIBC_EXIT_REASON_SIGNAL_BASE + iStatus, 0);
                                        iStatus = 127;
                                        break;
                                    case CLD_TRAPPED:
                                        if (iStatus <= SIGRTMAX && iStatus > 0)
                                            __libc_spmTerm(__LIBC_EXIT_REASON_SIGNAL_BASE + iStatus, 0);
                                        else
                                            __libc_spmTerm(__LIBC_EXIT_REASON_TRAP, 0);
                                        iStatus = 127;
                                        break;
                                }

                                LIBCLOG_MSG("Calling DosExit(,0)\n");
                                for (;;)
                                    DosExit(EXIT_PROCESS, iStatus);
                                break; /* won't get here */
                            }
                        }

                        LIBC_ASSERTM_FAILED("__libc_Back_processWait(P_PID,%d,,WEXITED,NULL) returned %d\n", pid, rc);
                        __libc_spmTerm(__LIBC_EXIT_REASON_KILL + SIGABRT, 123);
                        for (;;)
                            DosExit(EXIT_PROCESS, 123);
                        break; /* won't get here */
                    }

                        /* this can _NEVER_ happen! */
                    default:
                        errno = EINVAL;
                        LIBCLOG_ERROR_RETURN_INT(-1);
                }
                /* won't ever get here! */
            }
            else if (rc > 0)
                _sys_set_errno(rc);
            else
                errno = -rc;
            doInheritDone();
        }
        /* cleanup embryo */
        __libc_spmRelease(pEmbryo);
    }

    if (pszArgsBuf != NULL)
        _tfree(pszArgsBuf);
    _fmutex_release(&__libc_gmtxExec);
    LIBCLOG_ERROR_RETURN_INT(-1);
}
