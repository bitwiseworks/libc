/* isatty.c (emx+gcc) -- Copyright (c) 1990-1996 by Eberhard Mattes
                      -- Copyright (c) 2003 by Knut St. Osmunden */

#include "libc-alias.h"
#include <io.h>
#include <errno.h>
#include <emx/io.h>

int _STD(isatty)(int handle)
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

    return (pFH->fFlags & __LIBC_FH_TYPEMASK) == F_DEV;
}
