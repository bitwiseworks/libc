/* close.c (emx+gcc) -- Copyright (c) 1990-1996 by Eberhard Mattes
                     -- Copyright (c) 2003 by Knut St. Osmunden */

#include "libc-alias.h"
#include <io.h>
#include <emx/io.h>

int _STD(close)(int fh)
{
    int rc;
    rc = __libc_FHClose(fh);
    if (!rc)
        return 0;
    /* (it sets errno) */
    return -1;
}
