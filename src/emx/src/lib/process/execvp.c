/* execvp.c (emx+gcc) -- Copyright (c) 1990-1995 by Eberhard Mattes */

#include "libc-alias.h"
#include <process.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_PROCESS
#include <InnoTekLIBC/logstrict.h>

int _STD(execvp)(const char *name, char * const argv[])
{
    LIBCLOG_ENTER("name=%p:{%s} argv=%p\n", (void *)name, name, (void *)argv);
    int rc = spawnvp(P_OVERLAY, name, argv);
    LIBCLOG_ERROR_RETURN_INT(rc);
}
