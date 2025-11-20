/*
    close_range.c
    Copyright (c) 2025 bww bitwise works GmbH
*/

#include "libc-alias.h"
#include <unistd.h>
#include <emx/io.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_IO
#include <InnoTekLIBC/logstrict.h>

int _STD(close_range)(unsigned int fhLow, unsigned int fhHigh, int fFlags)
{
    LIBCLOG_ENTER("fhLow=%u fhHigh=%u fFlags=%#x\n", fhLow, fhHigh, fFlags);
    if (!__libc_FHCloseRange(fhLow, fhHigh, fFlags))
        LIBCLOG_RETURN_INT(0);
    LIBCLOG_ERROR_RETURN_INT(-1);
}
