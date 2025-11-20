/*
    closefrom.c
    Copyright (c) 2025 bww bitwise works GmbH
*/

#include "libc-alias.h"
#include <unistd.h>
#include <emx/io.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_IO
#include <InnoTekLIBC/logstrict.h>

void _STD(closefrom)(int fh)
{
    LIBCLOG_ENTER("fh=%d\n", fh);
    if (!__libc_FHCloseRange(fh, -1, 0))
        LIBCLOG_RETURN_VOID();
    LIBCLOG_ERROR_RETURN_VOID();
}
