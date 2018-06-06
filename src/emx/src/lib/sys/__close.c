/* sys/close.c (emx+gcc) -- Copyright (c) 1992-1996 by Eberhard Mattes
                         -- Copyright (c) 2003 by Knut St. Osmunden */

#include "libc-alias.h"
#include <emx/io.h>
#include <emx/syscalls.h>

int __close(int fh)
{
    if (!__libc_FHClose(fh))
        return -0;
    return -1;
}
