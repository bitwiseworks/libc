/* pipe.c (emx+gcc) -- Copyright (c) 1992-1996 by Eberhard Mattes */

#include "libc-alias.h"
#include <io.h>
#include <fcntl.h>
#include <errno.h>
#include <emx/io.h>
#include <emx/syscalls.h>

extern int _fmode_bin;

/** The default pipe size. */
int     _pipe_size;

int _STD(pipe)(int *two_handles)
{
    PLIBCFH pFHRead, pFHWrite;

    if (__pipe(two_handles, _pipe_size != 0 ? _pipe_size : 8192, &pFHRead, &pFHWrite) != 0)
        return -1;

    /** @todo move _fmode_bin check to __pipe()? */
    if (!_fmode_bin)
    {
        pFHRead->fFlags  |= O_TEXT;
        pFHWrite->fFlags |= O_TEXT;
    }

    return 0;
}
