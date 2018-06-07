/* lseek.c (emx+gcc) -- Copyright (c) 1990-1996 by Eberhard Mattes */

#include "libc-alias.h"
#include <io.h>
#include <errno.h>
#include <emx/io.h>
#include <InnoTekLIBC/backend.h>

off_t _STD(lseek) (int handle, off_t offset, int origin)
{
    PLIBCFH pFH;
    off_t   n;

    /*
     * Get filehandle.
     */
    pFH = __libc_FH(handle);
    if (!pFH)
      return -1;

    if (    origin == SEEK_CUR
        && (    pFH->iLookAhead >= 0
            || (pFH->fFlags & F_EOF)) )
        --offset;
    n = __libc_Back_ioSeek(handle, offset, origin);
    if (n >= 0)
    {
        pFH->fFlags &= ~F_EOF;          /* Clear Ctrl-Z flag */
        pFH->iLookAhead = -1;           /* Clear lookahead */
        return n;
    }
    errno = -n;
    return -1;
}
