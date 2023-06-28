/*
    Application low-level initialization routine.

    Copyright (c) 1992-1998 by Eberhard Mattes
    Copyright (c) 2003 InnoTek Systemberatung GmbH

    This routine is called from crt0.o. It should parse
    argv and envp, initialize heap and do all other sorts
    of low-level initialization.
*/


/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#include "libc-alias.h"
#define INCL_DOS
#define INCL_DOSSIGNALS
#define INCL_FSMACROS
#include <os2emx.h>
#include <string.h>
#include <sys/builtin.h>
#include <sys/fmutex.h>
#include <emx/startup.h>
#include <emx/syscalls.h>
#include <emx/umalloc.h>
#include <alloca.h>
#include <InnoTekLIBC/thread.h>
#include <InnoTekLIBC/sharedpm.h>
#include <InnoTekLIBC/backend.h>
#include <InnoTekLIBC/FastInfoBlocks.h>
#define __LIBC_LOG_GROUP    __LIBC_LOG_GRP_INITTERM
#include <InnoTekLIBC/logstrict.h>
#include <klibc/startup.h>
#include "b_signal.h"


/* All globals is to be defined in this object file. */
#define EXTERN
#define INIT(x) = x
#include "syscalls.h"


/*******************************************************************************
*   Internal Functions                                                         *
*******************************************************************************/
static int parse_args(const char *src, char **argv, char *pool);
/*static int verify_end_of_single_quote(const char *src);*/


/*******************************************************************************
*   Global Variables                                                           *
*******************************************************************************/
extern int      __libc_gfkLIBCArgs;
extern void    *__libc_gpTmpEnvArgs;

/** argument count found by parse_args(). */
static int argc;

/*******************************************************************************
*   Defined Constants And Macros                                               *
*******************************************************************************/
/** Helper macros for parse args
 * @{ */
#define PUTC(c) BEGIN ++arg_size; if (pool != NULL) *pool++ = (c); END
#define PUTV    BEGIN ++argc; if (argv != NULL) *argv++ = pool; END
#define WHITE(c) ((c) == ' ' || (c) == '\t')
/** @} */


/**
 * Parses the argument string passed in as src.
 *
 * @returns size of the processed arguments.
 * @param   src     Pointer to sequent zero terminated string containing
 *                  the arguments to be parsed.
 * @param   pool    Pointer to memory pool to put the arguments into.
 *                  NULL allowed.
 * @param   argv    Pointer to argument vector to put argument pointers in.
 *                  NULL allowed.
 * @ingroup startup
 */
static int parse_args(const char *src, char **argv, char *pool)
{
    int   quote;
    char  ch;
    char  flags;
    char *flag_ptr;
    int   arg_size, arg_pos;

    argc = 0; arg_size = 0;

    if (__libc_gfkLIBCArgs != 2)
    {
        /* argv[0] */
        PUTC((char)_ARG_NONZERO);
        PUTV;
        for (;;)
        {
            PUTC(*src);
            if (*src == 0)
                break;
            ++src;
        }
        ++src;
    }

    /* Check for the kLIBC signature used for unix arguments. */
    if (!__libc_gfkLIBCArgs)
    {
        /* Convert from OS/2 command line convention to C/Unix. */
        for (;;)
        {
            while (WHITE(*src))
                ++src;
            if (*src == 0)
                break;
            if (!pool)
            {
                flags = _ARG_NONZERO;
                flag_ptr = &flags;
                arg_size++;
            }
            else
            {
                flag_ptr = pool;
                PUTC((char)_ARG_NONZERO);
            }
            arg_pos = arg_size;
            PUTV;
            quote = 0;
            for (;;)
            {
                /* End quote. */
                if (*src == quote && quote)
                    quote = 0;
                /* Start of double quote. */
                else if (!quote && *src == '"')
                {
                    quote = '"';
                    *flag_ptr |= _ARG_DQUOTE;
                }
                /*
                 * Only permit the single quote to be used at the start of an argument or
                 * within an already quoted one. This restriction is necessary to support
                 * unquoted filenames containing the single quote char.
                 */
                else if (   !quote && *src == '\''
                         && (   arg_pos == arg_size
                             || (*flag_ptr & _ARG_DQUOTE)))
                {
                    quote = '\'';
                    *flag_ptr |= _ARG_DQUOTE;
                }
                /*
                 * The escape character. This is truly magic/weird.
                 * It doesn't escape anything unless it's in front of a
                 * double quote character. EMX weirdness...
                 * (Ok, not escaping more than necessary is ok when using \ as path
                 * separator, but why weird interpretation of \\\\"asdf"?
                 */
                else if (*src == '\\' && quote != '\'')
                {
                    int cSlashes = 0;
                    do
                    {
                        cSlashes++;
                        src++;
                    } while (*src == '\\');

                    if (*src == '"')
                    {
                        /* Treat it is as escapes. */
                        while (cSlashes >= 2)
                        {
                            PUTC('\\');
                            cSlashes -= 2;
                        }
                        if (cSlashes & 1)
                            PUTC(*src);
                        else
                            src--;
                    }
                    else
                    {
                        /* unmodified, no escaping. */
                        while (cSlashes-- > 0)
                            PUTC('\\');
                        src--;
                    }
                }
                /* Check for end of argument. */
                else if (*src == 0 || (WHITE(*src) && !quote))
                    break;
                /* Normal character. */
                else
                    PUTC(*src);
                ++src;
            }
            PUTC(0);
        }
    }
    else
    {
        /* The kLIBC spawn packs exactly what we want, including the flag at [-1]. */
        src += sizeof(__KLIBC_ARG_SIGNATURE);
        if (pool)
        {
            /* copying */
            if (__libc_gpTmpEnvArgs)
            {
                /*
                 * Got a shared memory block with env & args. Skip the
                 * environment (and both size fields).
                 */
                size_t szEnv = *(size_t*)__libc_gpTmpEnvArgs;
                size_t szArgs = *(size_t*)(__libc_gpTmpEnvArgs + sizeof(size_t) + szEnv);
                if (szArgs)
                    src = __libc_gpTmpEnvArgs + szEnv + sizeof(size_t) * 2;
                else
                {
                     /* skip __KLIBC_ARG_SHMEM arg */
                    LIBC_ASSERTM((unsigned)*src & __KLIBC_ARG_SHMEM, "flags=%x", (unsigned)*src);
                    if (*src)
                        while (*src++);
                }
            }
            while (*src)
            {
                LIBC_ASSERTM((unsigned)*src & __KLIBC_ARG_NONZERO, "flags=%x", (unsigned)*src);
                argc++;
                *argv++ = pool + 1;
                do
                {
                    *pool++ = ch = *src++;
                } while (ch);
            }
        }
        else
        {
            /* counting */
            if (__libc_gpTmpEnvArgs)
            {
                /*
                 * Got a shared memory block with env & args. Skip the
                 * environment (and both size fields).
                 */
                size_t szEnv = *(size_t*)__libc_gpTmpEnvArgs;
                size_t szArgs = *(size_t*)(__libc_gpTmpEnvArgs + sizeof(size_t) + szEnv);
                if (szArgs)
                    src = __libc_gpTmpEnvArgs + szEnv + sizeof(size_t) * 2;
                else
                {
                     /* skip __KLIBC_ARG_SHMEM arg */
                    LIBC_ASSERTM((unsigned)*src & __KLIBC_ARG_SHMEM, "flags=%x", (unsigned)*src);
                    if (*src)
                        while (*src++);
                }
#ifdef DEBUG_LOGGING
                LIBCLOG_MSG2("big args (%u bytes):\n", szArgs);
                char tmp[16];
                const char *p = src;
                int n = 1, l;
                while (*p)
                {
                    l = __libc_LogSNPrintf(__LIBC_LOG_INSTANCE, tmp, sizeof(tmp), "arg[%d]=", n);
                    LIBCLOG_RAW(tmp, l);
                    l = strlen(p);
                    LIBCLOG_RAW(p, l);
                    LIBCLOG_RAW("\n", 1);
                    p += l + 1;
                    ++n;
                }
#endif
            }
            while (*src)
            {
                LIBC_ASSERTM((unsigned)*src & __KLIBC_ARG_NONZERO, "flags=%x", (unsigned)*src);
                argc++;
                do
                {
                    arg_size++;
                    ch = *src++;
                } while (ch);
            }
        }
    }
    return arg_size;
}


#if 0
/**
 * This is a hack to prevent us from messing up filenames that
 * include a \' character.
 *
 * @param src
 *
 * @return int
 */
static int verify_end_of_single_quote(const char *src);
#endif


/**
 * Does initialization for thread 1 before main() is called.
 *
 * This function doesn't return, but leaves its stack frame on the stack by some
 * magic done in sys/386/appinit.s. The top of the returned stack have a layout
 * as seen in struct stackframe below - start of struct is main() callframe.
 *
 * @param   fFlags  Bit 0: If set the application is open to put the default heap in high memory.
 *                         If clear the application veto against putting the default heap in high memory.
 *                  Bit 1: If set some of the unixness of LIBC is disabled.
 *                         If clear all unix like features are enabled.
 *                  Passed on to __init_dll().
 *
 * @ingroup startup
 */
void __init(int fFlags)
{
    /** top of stack upon 'return' from this function. */
    struct stackframe
    {
        /** Argument count. */
        int                           argc;
        /** Pointer to argument vector. */
        char **                       argv;
        /** Pointer to environmet vector. */
        char **                       envp;
        /** Exception handler registration record (*_sys_xreg). */
        EXCEPTIONREGISTRATIONRECORD   ExcpRegRec;
        /** Argument vector. */
        char *                        apszArg[1];
        /** somewhere after this comes the string. */
    } * pStackFrame;
    int       rc;
    int       cb;

    /*
     * Do the common initialization in case we're linked statically.
     * Then end the heap voting.
     */
    if (__init_dll(fFlags, 0))
        goto failure;

    __libc_HeapEndVoting();

    /*
     * Copy command line arguments.
     *    Parse it to figure out the size.
     *    Allocate stack frame for args and more.
     *    Redo the parsing, but setup argv now.
     */
    cb = parse_args(fibGetCmdLine(), NULL, NULL);
    cb += (argc + 1) * sizeof (char *) + sizeof (struct stackframe);
    cb = (cb + 15) & ~15;
    pStackFrame = alloca(cb);
    if (!pStackFrame)
    {
        LIBC_ASSERTM_FAILED("alloca(%d) failed\n", cb);
        goto failure;
    }

    pStackFrame->envp = _org_environ;
    pStackFrame->argc = argc;
    pStackFrame->argv = &pStackFrame->apszArg[0];
    parse_args(fibGetCmdLine(), pStackFrame->argv, (char*)&pStackFrame->argv[argc + 1]);
    pStackFrame->argv[argc] = NULL;

    /*
     * Free env & args shared memory block if any.
     */
    if (__libc_gpTmpEnvArgs)
    {
        DosFreeMem(__libc_gpTmpEnvArgs);
        __libc_gpTmpEnvArgs = NULL;
    }

    /*
     * Free the SPM inherit data.
     */
    __libc_spmInheritFree();

    /*
     * Install exception handler, 16-bit signal handler and set signal focus.
     */
    rc = __libc_back_signalInitExe(&pStackFrame->ExcpRegRec);
    if (rc)
        goto failure;

    /*
     * Mark the process as a completely inited LIBC process.
     */
    __libc_spmExeInited();

    /* Return to the program. */
    _sys_init_ret(pStackFrame);

failure:
    DosExit(EXIT_PROCESS, 255);
}

