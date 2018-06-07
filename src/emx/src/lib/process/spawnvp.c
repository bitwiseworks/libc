/* spawnvp.c (emx+gcc) -- Copyright (c) 1990-1995 by Eberhard Mattes */

#include "libc-alias.h"
#include <stddef.h>
#include <process.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_PROCESS
#include <InnoTekLIBC/logstrict.h>

int _STD(spawnvp)(int mode, const char *name, char * const argv[])
{
    LIBCLOG_ENTER("mode=%#x name=%s argv=%p\n", mode, name, argv);
    int rc = spawnvpe(mode, name, argv, NULL);
    LIBCLOG_RETURN_INT(rc);
}
