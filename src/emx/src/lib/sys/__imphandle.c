/* imphandl.c (emx+gcc) -- Copyright (c) 1994-1996 by Eberhard Mattes
                        -- Copyright (c) 2003 by Knut St. Osmunden */

#include "libc-alias.h"
#include <emx/io.h>
#include <emx/syscalls.h>

int __imphandle(int fh)
{
    PLIBCFH pFH = __libc_FH(fh);
    if (pFH)
        return fh;
    return -1;
}
