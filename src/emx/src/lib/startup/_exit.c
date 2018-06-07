/* _exit.c (emx+gcc) -- Copyright (c) 1990-1996 by Eberhard Mattes */

#include <stdlib.h>
#include <emx/syscalls.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_INITTERM
#include <InnoTekLIBC/logstrict.h>

/* mkstd.awk: NOUNDERSCORE(exit) */
void _exit(int ret)
{
    LIBCLOG_ENTER("ret=%d\n", ret);
    for (;;)
        __exit(ret);
    LIBCLOG_MSG("shut up\n");
}
