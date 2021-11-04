/* close.c (emx+gcc) -- Copyright (c) 1990-1996 by Eberhard Mattes
                     -- Copyright (c) 2003 by Knut St. Osmunden */

#include "libc-alias.h"
#include <io.h>
#include <emx/io.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_IO
#include <InnoTekLIBC/logstrict.h>

int _STD(close)(int fh)
{
    LIBCLOG_ENTER("fh=%d\n", fh);
    int rc;
    rc = __libc_FHClose(fh);
    if (!rc)
        LIBCLOG_RETURN_INT(0);
    /* (it sets errno) */
    LIBCLOG_ERROR_RETURN_INT(-1);
}
