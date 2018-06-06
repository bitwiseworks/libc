/* _isterm.c (emx+gcc) -- Copyright (c) 1992-1996 by Eberhard Mattes */

#include "libc-alias.h"
#include <io.h>
#include <errno.h>
#include <emx/io.h>
#include <emx/syscalls.h>

int _isterm(int handle)
{
    PLIBCFH pFH;

    /*
     * Get filehandle.
     */
    pFH = __libc_FH(handle);
    if (!pFH)
    {
        errno = EBADF;
        return 0;
    }

    /*
     * Is it a atty?
     */
    if (!isatty(handle))
        return 0;

    /*
     * If it's a device we assume is a terminal...
     * (Hope that's correct interpretation of the __ioctl1() call...)
     */
    return (pFH->fFlags & __LIBC_FH_TYPEMASK) == F_DEV;
}
