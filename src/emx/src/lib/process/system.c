/* $Id: system.c 3910 2014-10-24 09:11:08Z bird $ */
/** @file
 * kNIX - system().
 *
 * @copyright   Copyright (C) 2014 knut st. osmundsen <bird-klibc-spam-xiv@anduin.net>
 * @licenses    MIT, BSD2, BSD3, BSD4, LGPLv2.1, LGPLv3.
 */


/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_PROCESS
#include "libc-alias.h"
#include <stdlib.h>

#include <io.h>
#include <errno.h>
#include <process.h>
#include <unistd.h>
#include <InnotekLIBC/backend.h>
#include <InnotekLIBC/logstrict.h>


int _STD(system)(const char *pszCmdLine)
{
    LIBCLOG_ENTER("pszCmdLine=%s\n", pszCmdLine);
    char szCmdLineOpt[8];
    char szShell[260];
    size_t offShellArg;
    int rc = __libc_Back_processGetDefaultShell(szShell, sizeof(szShell), &offShellArg, szCmdLineOpt, sizeof(szCmdLineOpt));
    if (rc == 0)
    {
        LIBCLOG_MSG("using shell: %s\n", pszCmdLine);
        if (pszCmdLine == 0)
            rc = access(szShell, F_OK) == 0;
        else if (!*pszCmdLine) /* EMX speciality? */
            rc = spawnlp(P_WAIT, szShell, &szShell[offShellArg], (void *)NULL);
        else
            rc = spawnlp(P_WAIT, szShell, &szShell[offShellArg], szCmdLineOpt, pszCmdLine, (void *)NULL);
    }
    else
    {
        errno = -rc;
        rc = -1;
    }
    LIBCLOG_RETURN_INT(rc);
}

