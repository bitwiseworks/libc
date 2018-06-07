/* execv.c (emx+gcc) -- Copyright (c) 1990-1995 by Eberhard Mattes */

#include "libc-alias.h"
#include <process.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_PROCESS
#include <InnoTekLIBC/logstrict.h>

int _STD(execv)(const char *name, char * const argv[])
{
    LIBCLOG_ENTER("name=%s argv=%p\n", name, argv);
    int rc = spawnv(P_OVERLAY, name, argv);
    LIBCLOG_ERROR_RETURN_INT(rc);
}
