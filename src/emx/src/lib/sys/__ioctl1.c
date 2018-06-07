/* sys/ioctl1.c (emx+gcc) -- Copyright (c) 1992-1996 by Eberhard Mattes
                          -- Copyright (c) 2003 by Knut St. Osmunden */

#include "libc-alias.h"
#include <errno.h>
#define INCL_FSMACROS
#include <os2emx.h>
#include <emx/io.h>
#include <emx/syscalls.h>
#include "syscalls.h"


/** Obsolete interface to DosQueryHType.
 * @deprecated Do not use this!!
 */
int __ioctl1(int handle, int code)
{
    PLIBCFH  pFH;

    /*
     * Get filehandle.
     */
    pFH = __libc_FH(handle);
    if (!pFH)
    {
        errno = EBADF;
        return -1;
    }
    if (pFH->pOps)
    {
        errno = EINVAL;
        return -1;
    }

    /*
     * Check code.
     */
    switch (code)
    {
        /** @todo get rid of this hardcoded shit. */
        case 0:
        {
            ULONG rc;
            ULONG htype, hflags;
            FS_VAR();
            FS_SAVE_LOAD();
            rc = DosQueryHType(handle, &htype, &hflags);
            FS_RESTORE();
            if (rc)
            {
                _sys_set_errno(rc);
                return -1;
            }
            if ((htype & 0xff) == 0)
              return 0;
            return (hflags & 0x0f) | 0x80;
        }
    }

    /* default */
    errno = EINVAL;
    return -1;
}
