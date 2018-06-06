/* spawnl.c (emx+gcc) -- Copyright (c) 1992-1995 by Eberhard Mattes */

#include "libc-alias.h"
#include <stddef.h>
#include <stdarg.h>
#include <process.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_PROCESS
#include <InnoTekLIBC/logstrict.h>

int _STD(spawnl)(int mode, const char *name, const char *arg0, ...)
{
    LIBCLOG_ENTER("mode=%#x name=%s arg0=%s ...\n", mode, name, arg0);
    /* Note: Passing `&arg0' to spawnv() is not portable. */
    int rc = spawnv (mode, name, (char * const *)&arg0);
    LIBCLOG_RETURN_INT(rc);
}
