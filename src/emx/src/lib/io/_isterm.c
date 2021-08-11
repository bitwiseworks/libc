/* _isterm.c (emx+gcc) -- Copyright (c) 1992-1996 by Eberhard Mattes */

#include "libc-alias.h"
#include <io.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <emx/io.h>
#include <emx/syscalls.h>

int _isterm(int handle)
{
    /*
     * Use __ioctl2 that knows how to query non-standard handles rathar than
     * call isatty and osQueryHType.
     */

    int type;
    if (__ioctl2(handle, FGETHTYPE, (int)&type) == -1)
        return 0;
    return type == HT_DEV_CON;
}
