/* execlp.c (emx+gcc) -- Copyright (c) 1992-1995 by Eberhard Mattes */

#include "libc-alias.h"
#include <stddef.h>
#include <stdarg.h>
#include <process.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_PROCESS
#include <InnoTekLIBC/logstrict.h>

int _STD(execlp)(const char *name, const char *arg0, ...)
{
    LIBCLOG_ENTER("name=%s arg0=%s ...\n", name, arg0);
    int rc;

    /* Note: Passing `&arg0' to spawnvp() is not portable. */

    rc = spawnvp(P_OVERLAY, name, (char * const *)&arg0);
    LIBCLOG_ERROR_RETURN_INT(rc);
}
