/* setmode.c (emx+gcc) -- Copyright (c) 1990-1996 by Eberhard Mattes
                       -- Copyright (c) 2003 by Knut St. Osmunden */

#include "libc-alias.h"
#include <io.h>
#include <fcntl.h>
#include <errno.h>
#include <emx/io.h>

int _STD(setmode)(int handle, int mode)
{
    PLIBCFH pFH;
    int     old_mode;

    /*
     * Get filehandle.
     */
    pFH = __libc_FH(handle);
    if (!pFH)
    {
        errno = EBADF;
        return -1;
    }

    /*
     * O_BINARY isn't normally stored in the fFlags member.
     */
    old_mode = (pFH->fFlags & O_TEXT) ? O_TEXT : O_BINARY;
    if (mode == O_BINARY)
        pFH->fFlags &= ~O_TEXT;
    else if (mode == O_TEXT)
        pFH->fFlags = (pFH->fFlags & ~O_BINARY) | O_TEXT; /* paranoia */
    else
    {
        errno = EINVAL;
        return -1;
    }
    return old_mode;
}
