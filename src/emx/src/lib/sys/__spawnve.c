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


/*
 * Resolve the interpreter name.
 * Try the unchanged name first, then the name with .exe suffix,
 * then the PATH.  In the latter case we skip the directory path
 * if given, since we're frequently faced with path differences
 * between OS/2 and the UNIX where the script originated.
 */
static int resolveInterpreter(const char* pszInterpreter, char* szPathBuf)
{
    char *psz;
    int rc = __libc_back_fsResolve(pszInterpreter, BACKFS_FLAGS_RESOLVE_FULL, szPathBuf, NULL);
    if (rc)
    {
        char szPath[PATH_MAX];
        if (   _path2(pszInterpreter, ".exe", szPath, sizeof(szPath)) == 0
            || (   (psz = _getname(pszInterpreter)) != pszInterpreter
                && _path2(psz, ".exe", szPath, sizeof(szPath)) == 0) )
            rc = __libc_back_fsResolve(szPath, BACKFS_FLAGS_RESOLVE_FULL, szPathBuf, NULL);
        if (rc)
            LIBCLOG_MSG2("Failed to find interpreter '%s'! szPath='%s'\n", pszInterpreter, szPath);
    }

    return rc;
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
          if (pShMem) \
            DosFreeMem(pShMem); \
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
    int  i;
    char *psz;
    size_t cch;

#ifdef DEBUG_LOGGING
    LIBCLOG_MSG("fname=\"%s\",arg_off=0x%lx,env_off=0x%lx\n", (char *)np->fname_off, np->arg_off, np->env_off);
    psz = (char *)np->arg_off;
    for (i = 0; i < np->arg_count; ++i)
    {
        /* skip flag byte */
        LIBCLOG_MSG("arg[%d]=\"%s\"\n", i, ++psz);
        psz += strlen(psz) + 1;
    }
    psz = (char *)np->env_off;
    for (i = 0; i < np->env_count; ++i)
    {
        LIBCLOG_MSG("env[%d]=\"%s\"\n", i, psz);
        psz += strlen(psz) + 1;
    }
#endif

    /*
     * Validate mode.
     */
    int fHave32BitSize = np->mode & 0x80000000;
    ULONG ulMode = np->mode & 0x7FFFFFFF;
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
     * Resolve the program name - prefer .exe over stubs.
     * The caller have left enough space for adding an .exe extension.
     */
    char *pszPgmName = (char *)np->fname_off;
    size_t cchFname = strlen((char *)np->fname_off);
    _defext(pszPgmName, "exe");
    char szNativePath[PATH_MAX];
    int rc = __libc_back_fsResolve(pszPgmName, BACKFS_FLAGS_RESOLVE_FULL, &szNativePath[0], NULL);
    if (rc)
    {
        if (pszPgmName[cchFname])
        {
            pszPgmName[cchFname] = '\0'; /* Drop the .exe bit added by _defext(). */
            rc = __libc_back_fsResolve(pszPgmName, BACKFS_FLAGS_RESOLVE_FULL, &szNativePath[0], NULL);
        }
        if (rc)
        {
            errno = -rc;
            LIBCLOG_ERROR_RETURN(-1, "ret -1 - Failed to resolve program name: '%s' rc=%d.\n", pszPgmName, rc);
        }
    }
    if (pszPgmName[cchFname]) /* update the length if .exe was added */
        cchFname = strlen(pszPgmName);
    pszPgmName = &szNativePath[0];

    /*
     * cmd.exe and 4os2.exe needs different argument handling, and
     * starting with kLIBC 0.6.4 we can pass argv directly to LIBC
     * programs.
     * (1 == cmd or 4os2 shell, 0 == anything else)
     */
    enum { args_standard, args_cmd, args_unix } enmMethod = args_standard;

    const char *pszInterpreter = NULL;
    const char *pszInterpreterArgs = NULL;
    size_t      cchInterpreterArgs = 0;

    int cTries = 2;
    while (cTries-- > 0)
    {
        char *psz = _getname(pszPgmName);
        if (   stricmp(psz, "cmd.exe") == 0
            || stricmp(psz, "4os2.exe") == 0)
        {
            enmMethod = args_cmd;
            break;
        }
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
                rc = DosRead(hFile, szLineBuf, sizeof(szLineBuf) - 1, &cbRead);
                DosClose(hFile);
                if (rc)
                    LIBCLOG_ERROR("DosRead - rc=%d (%s)\n", rc, pszPgmName);
                else
                {
                    if (   cbRead >= __KLIBC_STUB_MIN_SIZE
                        && szLineBuf[0] == 'M'
                        && szLineBuf[1] == 'Z'
                        && !memcmp(&szLineBuf[__KLIBC_STUB_SIGNATURE_OFF], __KLIBC_STUB_SIGNATURE_BASE, sizeof(__KLIBC_STUB_SIGNATURE_BASE) - 1)
                    )
                    {
                        char const *pchVer = &szLineBuf[__KLIBC_STUB_SIGNATURE_OFF + sizeof(__KLIBC_STUB_SIGNATURE_BASE) - 1];
                        if (*pchVer >= '0' && *pchVer <= '9')
                            enmMethod = !(ulMode & P_NOUNIXARGV) ? args_unix : args_standard;
                        break;
                    }
                    else if (cTries > 0 && __libc_Back_gfProcessHandleHashBangScripts)
                    {
                        /* Note: this is tried only once thanks to cTries to avoid hash bang recursion */
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

                        if (!pszInterpreter)
                            break;
                        rc = resolveInterpreter(pszInterpreter, &szNativePath[0]);
                        if (rc)
                        {
                            errno = -rc;
                            LIBCLOG_RETURN_INT(-1);
                        }

                        /* Repeat with the interpreter as the program name. */
                        pszPgmName = &szNativePath[0];
                        LIBCLOG_MSG("Trying with hash bang interpreter '%s' (-> '%s'), args '%s'\n", pszInterpreter, pszPgmName, pszInterpreterArgs);

                        /* Copy the strings as the buffer gets reused */
                        cch = strlen(pszInterpreter) + 1;
                        psz = alloca(cch);
                        memcpy(psz, pszInterpreter, cch);
                        pszInterpreter = psz;

                        if (pszInterpreterArgs)
                        {
                            cchInterpreterArgs = strlen(pszInterpreterArgs);
                            cch = cchInterpreterArgs + 1;
                            psz = alloca(cch);
                            memcpy(psz, pszInterpreterArgs, cch);
                            pszInterpreterArgs = psz;
                        }
                    }
                }
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
    } /* if cTries > 0 (a hash bang retry) */

    LIBCLOG_MSG("enmMethod (std=0,cmd=1,unix=2) %d\n", enmMethod);

    /*
     * Construct the commandline.
     */
    const char *pszSrc = (const char *)np->arg_off;
    char       *pszArgsBuf = NULL;
    size_t      cbArgsBuf = 0;
    char       *pszArg = NULL;
    size_t      cbArgs = 0;
    size_t      szArgSize = np->arg_size;
    size_t      szEnvSize = np->env_size;
    PCSZ        pszEnv = (PCSZ)np->env_off;
    PVOID       pShMem = NULL;
    size_t      szArgMax = ARG_MAX;
    size_t      cchArg0;

    if (fHave32BitSize)
    {
        szArgSize |= (size_t)np->arg_size2 << 16;
        szEnvSize |= (size_t)np->env_size2 << 16;
    }

    /* Get the length of argv[0] (note: skip flags byte) */
    cchArg0 = np->arg_count > 0 ? strlen(++pszSrc) + 1 : 0;

    if (pszInterpreter)
    {
        cch = strlen(pszInterpreter) + 1;

        /* Sanity. */
        if (cch > szArgMax)
        {
            LIBCLOG_ERROR("hash bang interpreter '%s' > ARG_MAX (%d > %d)\n", pszInterpreter, cch, szArgMax);
            errno = E2BIG;
            LIBCLOG_RETURN_INT(-1);
        }

        /*
         * In hash bang mode, ignore the original argv[0] but account for the
         * interpreter args and the script name (unresolved).
         */
        pszSrc += cchArg0;
        szArgSize -= cchArg0;
        if (pszInterpreterArgs)
            szArgSize += cchInterpreterArgs + 2 /* term + flags byte */;
        szArgSize += cchFname + 2 /* term + flags byte */;

        /* Make the interpreter a new argv[0]. */
        ADD(cch);
        memcpy(pszArg, pszInterpreter, cch);
        pszArg += cch;

    }
    else if (cchArg0)
    {
        /* Sanity. */
        if (cchArg0 > szArgMax)
        {
            LIBCLOG_ERROR("arg[0] > ARG_MAX (%d > %d)\n", cchArg0, szArgMax);
            errno = E2BIG;
            LIBCLOG_RETURN_INT(-1);
        }

        /* argv[0] */
        ADD(cchArg0);
        memcpy(pszArg, pszSrc, cchArg0);
        pszArg += cchArg0; pszSrc += cchArg0;
        szArgSize -= cchArg0;
        szArgMax -= cchArg0;
    }
    if (enmMethod == args_unix)
    {
        /*
         * Use shared memory to pass environment and arguments if we hit their
         * limits. ARG_MAX is set to 32K - 32 on OS/2 because this is how much
         * DosExecPgm can take for either of them (64K in total). Note that
         * Posix defines ARG_MAX to be the sum of the arguments and environment,
         * but since DosExecPgm doesn't let spend more than 32K for one half
         * even if the other one is smaller, we have to keep it like that.
         */
        char* pShDst = NULL;
        int fEnvExceedsArgMax = szEnvSize > ARG_MAX;
        int fArgsExceedsArgMax = szArgSize > szArgMax;
        int fUseShMem = fEnvExceedsArgMax || fArgsExceedsArgMax;
        if (fUseShMem)
        {
            ULONG fFlags = PAG_READ | PAG_WRITE | PAG_COMMIT | OBJ_GETTABLE;
            ULONG cb = sizeof(size_t) * 2;
            if (fEnvExceedsArgMax)
                cb += szEnvSize;
            if (fArgsExceedsArgMax)
                cb += szArgSize;
            rc = DosAllocSharedMem((PVOID)&pShMem, NULL, cb, fFlags | OBJ_ANY);
            if (rc)
            {
                rc = DosAllocSharedMem((PVOID)&pShMem, NULL, cb, fFlags);
                if (rc)
                {
                    LIBCLOG_ERROR("DosAllocSharedMem - rc=%d (cb=%lu)\n", rc, cb);
                    _tfree(pszArgsBuf);
                    errno = ENOMEM;
                    LIBCLOG_RETURN_INT(-1);
                }
            }
            LIBCLOG_MSG("Using shared mem for env & args %p (env size %d (max %d), args size %d (max %d) -> cb=%lu)\n",
                        pShMem, szEnvSize, ARG_MAX, szArgSize, szArgMax, cb);
            pShDst = pShMem;

            if (fEnvExceedsArgMax)
            {
                /*
                 * Copy environment first (see __init.c where it's processed).
                 * Note that in order to pass environment this way, we need to
                 * pass "\0" to DosExecPgm (when pShMem is not NULL) and then
                 * reconstruct it from scratch in the child using the data from
                 * shared memory.
                 */
                *(size_t*)pShDst = szEnvSize;
                pShDst += sizeof(size_t);
                memcpy(pShDst, pszEnv, szEnvSize);
                pShDst += szEnvSize;
                pszEnv = (PCSZ)"\0";
            }
            else
            {
                /* Use the regular way */
                *(size_t*)pShDst = 0;
                pShDst += sizeof(size_t);
            }
        }

        /*
         * The first arg is the signature (note that we assume that this fits
         * into 32 bytes off the 32K limit in ARG_MAX including what OS/2 also
         * needs for rounding).
         */
        ADD(sizeof(__KLIBC_ARG_SIGNATURE));
        memcpy(pszArg, __KLIBC_ARG_SIGNATURE, sizeof(__KLIBC_ARG_SIGNATURE));
        pszArg += sizeof(__KLIBC_ARG_SIGNATURE);

        /* then comes the argument vector. */
        if (fUseShMem)
        {
            if (fArgsExceedsArgMax)
            {
                *(size_t*)pShDst = szArgSize;
                pShDst += sizeof(size_t);
                if (pszInterpreter)
                {
                    /* Inject interpreter args + script name */
                    if (pszInterpreterArgs)
                    {
                        cch = cchInterpreterArgs + 1;
                        *pShDst++ = __KLIBC_ARG_NONZERO;
                        memcpy(pShDst, pszInterpreterArgs, cch);
                        pShDst += cch;
                    }
                    cch = cchFname + 1;
                    *pShDst++ = __KLIBC_ARG_NONZERO;
                    memcpy(pShDst, (const char*)np->fname_off, cch);
                    pShDst += cch;
                }
                memcpy(pShDst, pszSrc, szArgSize - cchInterpreterArgs - cchFname - 4);
            }
            else
            {
                /* Use the regular way */
                *(size_t*)pShDst = 0;
            }

            /*
             * The second arg is the address of shared memory in hex (XXXXXXXX +
             * \0). Note that we fit that into the 32 bytes off 32K as well.
             */
            ADD(9 + 1);
            *pszArg++ = __KLIBC_ARG_SHMEM;
            _ultoa((ULONG)pShMem, pszArg, 16);
            pszArg += strlen(pszArg) + 1;
        }

        if (!fUseShMem || !fArgsExceedsArgMax)
        {
            if (pszInterpreter)
            {
                /* Inject interpreter args + script name */
                if (pszInterpreterArgs)
                {
                    cch = cchInterpreterArgs + 1;
                    ADD(cch + 1);
                    *pszArg++ = __KLIBC_ARG_NONZERO;
                    memcpy(pszArg, pszInterpreterArgs, cch);
                    pszArg += cch;
                }
                cch = cchFname + 1;
                ADD(cch + 1);
                *pszArg++ = __KLIBC_ARG_NONZERO;
                memcpy(pszArg, (const char*)np->fname_off, cch);
                pszArg += cch;
            }
            for (i = 1; i < np->arg_count; ++i)
            {
                unsigned char chFlags = *pszSrc++;
                chFlags &= ~__KLIBC_ARG_MASK;
                chFlags |= __KLIBC_ARG_ARGV;
                cch = strlen(pszSrc) + 1;
                ADD(cch + 1);
                *pszArg++ = chFlags;
                memcpy(pszArg, pszSrc, cch);
                pszArg += cch;
                pszSrc += cch;
            }
        }
        /* the double termination. */
        ADD(1);
        *pszArg = '\0';
    }
    else
    {
        /* quote the arguments in emx / cmd.exe fashion. */
        for (i = pszInterpreter ? -1 : 1; i < np->arg_count; ++i)
        {
            /* Inject interpreter args + script name */
            const char *pszSrcSave = NULL;
            if (i == -1)
            {
                if (!pszInterpreterArgs)
                    continue;
                pszSrcSave = pszSrc;
                pszSrc = pszInterpreterArgs;
            }
            else if (i == 0)
            {
                pszSrcSave = pszSrc;
                pszSrc = (const char*)np->fname_off;
            }
            else
                ++pszSrc;                    /* skip flags byte */

            if (i > 1 || ((pszInterpreterArgs && i > -1) || i > 0))
            {
                ADD(1);
                *pszArg++ = ' ';
            }

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

            if (pszSrcSave)
                pszSrc = pszSrcSave;
            else
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
            rc = DosExecPgm(szObj, sizeof(szObj), EXEC_ASYNCRESULT, (PCSZ)pszArgsBuf, pszEnv, &resc, (PCSZ)pszPgmName);
            LIBCLOG_MSG("DosExecPgm returned %d\n", rc);

            if (     (   rc == ERROR_INVALID_EXE_SIGNATURE
                      || rc == ERROR_BAD_EXE_FORMAT)
                &&   !pszInterpreter)
            do
            {
                /*
                 * This could be a batch, rexx or hash bang script.
                 * The first two is recognized by the filename extension, the latter
                 * requires inspection of the first line of the file.
                 */
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
                cchInterpreterArgs = pszInterpreterArgs ? strlen(pszInterpreterArgs) : -1;
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

                /* Resolve the interpreter name. */
                rc = resolveInterpreter(pszInterpreter, &szNativePath[0]);
                if (rc)
                    break;

                /*
                 * Try execute it.
                 * Note! pszPgmName = &szNativePath[0], so we're good here.
                 */
                LIBCLOG_MSG("Calling DosExecPgm pgm: %s args: %s\\0%s\\0\\0\n", pszPgmName, pszArgsBuf, pszArgsBuf + strlen(pszArgsBuf) + 1);
                rc = DosExecPgm(szObj, sizeof(szObj), EXEC_ASYNCRESULT, (PCSZ)pszArgsBuf, (PCSZ)np->env_off, &resc, (PCSZ)pszPgmName);
                LIBCLOG_MSG("DosExecPgm returned %d\n", rc);
            }
            while (0);
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
                if (pShMem != NULL)
                    DosFreeMem(pShMem);

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
    if (pShMem != NULL)
        DosFreeMem(pShMem);
    _fmutex_release(&__libc_gmtxExec);
    LIBCLOG_ERROR_RETURN_INT(-1);
}
