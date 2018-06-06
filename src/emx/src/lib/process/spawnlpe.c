/* spawnlpe.c (emx+gcc) -- Copyright (c) 1992-1995 by Eberhard Mattes */

#include "libc-alias.h"
#include <stddef.h>
#include <stdarg.h>
#include <process.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_PROCESS
#include <InnoTekLIBC/logstrict.h>

int _STD(spawnlpe)(int mode, const char *name, const char *arg0, ...)
{
    LIBCLOG_ENTER("mode=%#x name=%s arg0=%s ...\n", mode, name, arg0);
    va_list arg_ptr;
    char * const *env_ptr;
    int rc;

    va_start(arg_ptr, arg0);
    while (va_arg(arg_ptr, char *) != NULL)
        /* do nothing */;
    env_ptr = va_arg(arg_ptr, char * const *);
    va_end(arg_ptr);

    /* Note: Passing `&arg0' to spawnvpe() is not portable. */

    rc = spawnvpe(mode, name, (char * const *)&arg0, env_ptr);
    LIBCLOG_RETURN_INT(rc);
}
