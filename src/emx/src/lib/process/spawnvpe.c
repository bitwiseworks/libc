/* $Id: cabsl.c 3889 2014-06-28 16:06:20Z bird $ */
/** @file
 * kLibC - Implementation of spawnvpe().
 *
 * @copyright   Copyright (C) 2014 knut st. osmundsen <bird-klibc-spam-xiv@anduin.net>
 * @licenses    MIT, BSD2, BSD3, BSD4, LGPLv2.1, LGPLv3, LGPLvFuture.
 */


/*******************************************************************************
* Header Files                                                                 *
*******************************************************************************/
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_PROCESS
#include "libc-alias.h"
#include <InnoTekLIBC/logstrict.h>
#include <stdlib.h>
#include <process.h>
#include <sys/syslimits.h>


int _STD(spawnvpe)(int fMode, const char *pszName, char * const *papszArgs, char * const *papszEnv)
{
    LIBCLOG_ENTER("fMode=%#x pszName=%s papszArgs=%p papszEnv=%p\n", fMode, pszName, papszArgs, papszEnv);
    char szResolvedName[PATH_MAX];
    if (_path2(pszName, ".exe", szResolvedName, sizeof(szResolvedName)) == 0)
    {
        int rc = spawnve(fMode, szResolvedName, papszArgs, papszEnv);
        LIBCLOG_MIX_RETURN_INT(rc);
    }
    LIBCLOG_ERROR_RETURN_INT(-1);
}

