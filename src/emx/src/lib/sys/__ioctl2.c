/* sys/ioctl2.c (emx+gcc) -- Copyright (c) 1992-1996 by Eberhard Mattes
                          -- Copyright (c) 2003 by Knut St. Osmunden */

#include "libc-alias.h"
#include <sys/ioctl.h>
#include <errno.h>
#define INCL_DOSFILEMGR
#define INCL_DOSERRORS
#define INCL_FSMACROS
#include <os2emx.h>
#include <sys/filio.h>
#include <emx/io.h>
#include <emx/syscalls.h>
#include "syscalls.h"

/** Get the low word of the ioctl request number.
 * Used to support ioctl request numbers from old and new _IOC macros.
 */
#define __IOCLW(a) ((unsigned short)(a))
/** This is the syscall for ioctl(). */
int __ioctl2(int handle, unsigned long request, int arg)
{
    PLIBCFH pFH;
    int     rc;
    ULONG   type;
    ULONG   flags;
    int *   int_ptr;
    FS_VAR();

    /*
     * Get filehandle.
     */
    pFH = __libc_FH(handle);
    if (!pFH)
    {
        errno = EBADF;
        return -1;
    }

    if (!pFH->pOps)
    {
        /*
         * Standard OS/2 filehandle.
         */
        switch (__IOCLW(request))
        {
            case __IOCLW(FGETHTYPE):
                int_ptr = (int *)arg;
                FS_SAVE_LOAD();
                rc = DosQueryHType (handle, &type, &flags);
                FS_RESTORE();
                if (rc != 0)
                {
                    _sys_set_errno (rc);
                    return -1;
                }
                switch (type & 0xff)
                {
                    case 0:                 /* File */
                        *int_ptr = HT_FILE;
                        break;
                    case 1:                 /* Character device */
                        if (flags & 3)
                            *int_ptr = HT_DEV_CON;
                        else if (flags & 4)
                            *int_ptr = HT_DEV_NUL;
                        else if (flags & 8)
                            *int_ptr = HT_DEV_CLK;
                        else
                            *int_ptr = HT_DEV_OTHER;
                        break;
                    case 2:                 /* Pipe */
                        FS_SAVE_LOAD();
                        rc = DosQueryNPHState (handle, &flags);
                        FS_RESTORE();
                        if (rc == 0 || rc == ERROR_PIPE_NOT_CONNECTED)
                            *int_ptr = HT_NPIPE;
                        else
                            *int_ptr = HT_UPIPE;
                        break;
                    default:
                        errno = EINVAL;
                        return -1;
                }
                return 0;

/** @todo lots of FIO* things to go!! */
#if 0

#define	FIOCLEX		 _IO('f', 1)		/* set close on exec on fd */
#define	FIONCLEX	 _IO('f', 2)		/* remove close on exec */
#define	FIONREAD	_IOR('f', 127, int)	/* get # bytes to read */
#define	FIONBIO		_IOW('f', 126, int)	/* set/clear non-blocking i/o */
#define	FIOASYNC	_IOW('f', 125, int)	/* set/clear async i/o */
#define	FIOSETOWN	_IOW('f', 124, int)	/* set owner */
#define	FIOGETOWN	_IOR('f', 123, int)	/* get owner */
#define	FIODTYPE	_IOR('f', 122, int)	/* get d_flags type part */
#define	FIOGETLBA	_IOR('f', 121, int)	/* get start blk # */

#endif

            default:
                errno = EINVAL;
                return -1;
        }
    }
    else
    {
        /*
         * Non-standard filehandle.
         */
        int rcRet;
        rc = pFH->pOps->pfnIOControl(pFH, handle, request, arg, &rcRet);
        if (rc)
        {
            if (rc > 0)
                _sys_set_errno(rc);
            else
                errno = -rc;
            return -1;
        }
        return rcRet;
    }
}
