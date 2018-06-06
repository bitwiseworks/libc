/* execvpe.c (emx+gcc) -- Copyright (c) 1990-1995 by Eberhard Mattes */

#include "libc-alias.h"
#include <process.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_PROCESS
#include <InnoTekLIBC/logstrict.h>

int _STD(execvpe)(const char *name, char * const argv[], char * const envp[])
{
    LIBCLOG_ENTER("name=%p:{%s} argv=%p envp=%p\n", (void *)name, name, (void *)argv, (void *)envp);
    int rc = spawnvpe(P_OVERLAY, name, argv, envp);
    LIBCLOG_ERROR_RETURN_INT(rc);
}
