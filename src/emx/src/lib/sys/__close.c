/* sys/close.c (emx+gcc) -- Copyright (c) 1992-1996 by Eberhard Mattes
                         -- Copyright (c) 2003 by Knut St. Osmunden */

#include "libc-alias.h"
#include <emx/io.h>
#include <emx/syscalls.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_IO
#include <InnoTekLIBC/logstrict.h>

int __close(int fh)
{
    LIBCLOG_ENTER("fh=%d\n", fh);
    if (!__libc_FHClose(fh))
        LIBCLOG_RETURN_INT(0);
    LIBCLOG_ERROR_RETURN_INT(-1);
}
