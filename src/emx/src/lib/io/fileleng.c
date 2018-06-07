/* fileleng.c (emx+gcc) -- Copyright (c) 1990-1996 by Eberhard Mattes */

#include "libc-alias.h"
#include <io.h>
#include <errno.h>
#include <InnoTekLIBC/backend.h>

off_t _STD(filelength)(int handle)
{
    off_t offcur = __libc_Back_ioSeek(handle, 0L, SEEK_CUR);
    if (offcur >= 0)
    {
        off_t offend = __libc_Back_ioSeek(handle, 0L, SEEK_END);
        __libc_Back_ioSeek(handle, offcur, SEEK_SET);
        if (offend >= 0)
            return offend;
    }
    errno = -offcur;
    return -1;
}
