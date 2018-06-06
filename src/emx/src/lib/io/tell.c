/* tell.c (emx+gcc) -- Copyright (c) 1990-1996 by Eberhard Mattes */

#include "libc-alias.h"
#include <io.h>
#include <errno.h>
#include <emx/io.h>
#include <InnoTekLIBC/backend.h>

off_t _STD(tell)(int handle)
{
    PLIBCFH pFH;
    off_t   n;

    pFH = __libc_FH(handle);
    if (!pFH)
        return -1;
    n = __libc_Back_ioSeek(handle, 0L, SEEK_CUR);
    if (n >= 0)
    {
        if (pFH->iLookAhead >= 0 || (pFH->fFlags & F_EOF))
            --n;
        return n;
    }
    errno = -n;
    return -1;
}
