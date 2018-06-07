/* ioctl.c (emx+gcc) -- Copyright (c) 1990-1996 by Eberhard Mattes
                     -- Copyright (c) 2003 by Knut St. Osmunden */

#include "libc-alias.h"
#include <stdlib.h>
#include <stdarg.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/termio.h>
#include <sys/ioctl.h>
#include <emx/io.h>
#include <emx/syscalls.h>

/** Get the low word of the ioctl request number.
 * Used to support ioctl request numbers from old and new _IOC macros.
 */
#define __IOCLW(a) ((unsigned short)(a))


int _STD(ioctl) (int handle, unsigned long request, ...)
{
    va_list     va;
    PLIBCFH     pFH;
    int         rc, saved_errno, arg, *int_ptr;
    const struct termio *tp;

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
     * Make syscall.
     */
    saved_errno = errno; errno = 0;
    va_start (va, request);
    arg = va_arg(va, int);
    va_end (va);
    rc = __ioctl2(handle, request, arg);

    /*
     * On success do small touches of our own.
     */
    /** @todo fFlags can be handled by the syscall just as well as here..*/
    if (rc >= 0 && errno == 0)
    {
        switch (__IOCLW(request))
        {
            case __IOCLW(TCSETAF):
            case __IOCLW(TCSETAW):
            case __IOCLW(TCSETA):
                va_start(va, request);
                tp = va_arg(va, const struct termio *);
                va_end(va);
                if (tp->c_lflag & IDEFAULT)
                    pFH->fFlags &= ~F_TERMIO;
                else
                    pFH->fFlags |= F_TERMIO;
                break;

            case __IOCLW(_TCSANOW):
            case __IOCLW(_TCSADRAIN):
            case __IOCLW(_TCSAFLUSH):
                pFH->fFlags |= F_TERMIO;
                break;

            case __IOCLW(FIONREAD):
                if (pFH->iLookAhead >= 0)
                {
                    va_start(va, request);
                    int_ptr = va_arg(va, int *);
                    va_end(va);
                    ++(*int_ptr);
                }
                break;

            case __IOCLW(FIONBIO):
                va_start(va, request);
                int_ptr = va_arg(va, int *);
                va_end(va);
                if (*int_ptr)
                    pFH->fFlags |= O_NDELAY;
                else
                    pFH->fFlags &= ~O_NDELAY;
                break;
        } /* switch */
    }
    if (errno == 0)
        errno = saved_errno;
    return rc;
}
