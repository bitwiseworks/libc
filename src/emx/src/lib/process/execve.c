/* execve.c (emx+gcc) -- Copyright (c) 1990-1995 by Eberhard Mattes */

#include "libc-alias.h"
#include <process.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_PROCESS
#include <InnoTekLIBC/logstrict.h>

int _STD(execve)(const char *name, char * const argv[], char * const envp[])
{
    LIBCLOG_ENTER("name=%s argv=%p envp=%p\n", name, argv, envp);
    int rc = spawnve(P_OVERLAY, name, argv, envp);
    LIBCLOG_ERROR_RETURN_INT(rc);
}
