/* $Id: b_processGetDefaultShell.c 3859 2014-04-14 02:19:42Z bird $ */
/** @file
 * kNIX - get default shell for system() and popen().
 *
 * @copyright   Copyright (C) 2014 knut st. osmundsen <bird-klibc-spam-xiv@anduin.net>
 * @licenses    MIT, BSD2, BSD3, BSD4, LGPLv2.1, LGPLv3.
 */


/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#include "libc-alias.h"
#include "syscalls.h"
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_BACK_PROCESS
#include <InnotekLIBC/logstrict.h>
#include <InnotekLIBC/libc.h>
#include <InnotekLIBC/backend.h>


/**
 * Gets the default shell for functions like system() and popen().
 *
 * @returns 0 on success, negative error number on failure.
 * @param   pszShell        Where to put the path to the shell.
 * @param   cbShell         The size of the buffer @a pszShell points to.
 * @param   poffShellArg    Where to return the offset into @a pszShell of the
 *                          first argument.  The system() and popen() calls has
 *                          traditionally not included the path to /bin/sh.
 * @param   pszCmdLineOpt   Where to put the shell option for specifying a
 *                          command line it should execute.
 * @param   cbCmdLineOpt    The size of the buffer @a pszCmdLineOpt points to.
 */
int __libc_Back_processGetDefaultShell(char *pszShell, size_t cbShell, size_t *poffShellArg,
                                       char *pszCmdLineOpt, size_t cbCmdLineOpt)
{
    const char *pszBase;
    const char *pszEnv;
    size_t cchEnv;
    int fIsUnixShell = -1;

    /*
     * If the process isn't a no-unix process, try look for a shell under
     * the unixroot.
     */
    if (!__libc_gfNoUnix)
    {
        struct stat st;
        fIsUnixShell = 1;
        pszEnv = "/@unixroot/usr/bin/sh.exe";
        if (stat(pszEnv, &st) != 0)
        {
            pszEnv = NULL;
            if (stat("/@unixroot/usr/bin/", &st) == 0)
            {
                pszEnv = "/@unixroot/usr/bin/ash.exe";
                if (stat(pszEnv, &st) < 0)
                {
                    pszEnv = "/@unixroot/usr/bin/bash.exe";
                    if (stat(pszEnv, &st) < 0)
                    {
                        fIsUnixShell = -1;
                        pszEnv = NULL;
                    }
                }
            }
        }
    }
    else
        pszEnv = NULL;

    /*
     * Fall back on various environment variables.
     */
    if (!pszEnv)
    {
        pszEnv = getenv("EMXSHELL");
        if (!pszEnv)
        {
            pszEnv = getenv(!__libc_gfNoUnix ? "SHELL" : "COMSPEC");
            if (!pszEnv)
            {
                pszEnv = getenv(!__libc_gfNoUnix ? "COMSPEC" : "SHELL");
#ifdef __OS2__
                if (!pszEnv)
                    pszEnv = getenv("OS2_SHELL");
#endif
                if (!pszEnv)
                    return -ENOENT;
            }
        }
    }

    /*
     * Copy out the path.
     */
    cchEnv = strlen(pszEnv);
    if (cchEnv >= cbShell)
        return -ENAMETOOLONG;
    memcpy(pszShell, pszEnv, cchEnv);
    pszShell[cchEnv] = '\0';

    if (cbCmdLineOpt < sizeof("-c"))
        return -EOVERFLOW;

    /*
     * What kind of option does it take before a command line?
     */
    pszBase = _getname(pszShell);
    if (   !stricmp(pszBase, "cmd.exe")
#ifdef __OS2__
        || !stricmp(pszBase, "4os2.exe")
#endif
#ifdef __NT__
        || !stricmp(pszBase, "4nt.exe")
        || !stricmp(pszBase, "tcc.exe")
#endif
        || !stricmp(pszBase, "command.com")
        || !stricmp(pszBase, "4dos.com")
       )
    {
        memcpy(pszCmdLineOpt, "/c", sizeof("/c"));
        fIsUnixShell = 0;
    }
    else
        memcpy(pszCmdLineOpt, "-c", sizeof("-c"));

    /*
     * Offset of argv[0] into pszShell.  Only do this for unix shell, at least
     * for now.
     */
    if (fIsUnixShell == -1)
    {
        fIsUnixShell = !stricmp(pszBase, "sh.exe")   || !stricmp(pszBase, "sh")
                    || !stricmp(pszBase, "ash.exe")  || !stricmp(pszBase, "ash")
                    || !stricmp(pszBase, "bash.exe") || !stricmp(pszBase, "bash")
                    || !stricmp(pszBase, "kash.exe") || !stricmp(pszBase, "kash");
    }
    if (fIsUnixShell)
        *poffShellArg = pszBase - pszShell;
    else
        *poffShellArg = 0;

    return 0;
}

