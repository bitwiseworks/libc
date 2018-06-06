/* sys/write.c (emx+gcc) -- Copyright (c) 1992-1996 by Eberhard Mattes
                         -- Copyright (c) 2003 by Knut St. Osmunden */

#include "libc-alias.h"
#define INCL_PRESERVE_REGISTER_MACROS
#define INCL_ERRORS
#include <os2emx.h>
#include <memory.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/fcntl.h>
#include <emx/io.h>
#include <emx/umalloc.h>
#include <emx/syscalls.h>
#include "syscalls.h"
#define __LIBC_LOG_GROUP    __LIBC_LOG_GRP_BACK_IO
#include <InnoTekLIBC/logstrict.h>

int __write(int handle, const void *buf, size_t cbToWrite)
{
    LIBCLOG_ENTER("fd=%d buf=%p cbToWrite=%zu\n", handle, buf, cbToWrite);
    PLIBCFH pFH;
    int     rc;
    ULONG   cbWritten;

    /*
     * Get filehandle.
     */
    pFH = __libc_FH(handle);
    if (!pFH)
    {
        errno = EBADF;
        LIBCLOG_ERROR_RETURN_INT(-1);
    }

    if (!pFH->pOps)
    {
        /*
         * Standard OS/2 filehandle.
         */
        void *pvBuf_safe = NULL;

        /*
         * Devices doesn't like getting high addresses.
         */
        if (    (pFH->fFlags & __LIBC_FH_TYPEMASK) == F_DEV
            &&  (unsigned)buf >= 512*1024*1024)
        {
            if (cbToWrite > 256)
            {   /* use heap for large buffers */
                pvBuf_safe = _lmalloc(cbToWrite);
                if (!pvBuf_safe)
                {
                    errno = ENOMEM;
                    LIBCLOG_ERROR_RETURN_INT(-1);
                }
                memcpy(pvBuf_safe, buf, cbToWrite);
                buf = pvBuf_safe;
            }
            else
            {   /* use stack for small buffers */
                pvBuf_safe = alloca(cbToWrite);
                memcpy(pvBuf_safe, buf, cbToWrite);
                buf = pvBuf_safe;
                pvBuf_safe = NULL;
            }
        }

        PRESERVE_REGS_SAVE_LOAD_SAFE();
        rc = DosWrite(handle, buf, cbToWrite, &cbWritten);
        PRESERVE_REGS_RESTORE();

        if (pvBuf_safe)
            free(pvBuf_safe);
    }
    else
    {
        /*
         * Non-standard filehandle.
         */
        size_t cbWritten2;
        rc = pFH->pOps->pfnWrite(pFH, handle, buf, cbToWrite, &cbWritten2);
        cbWritten = cbWritten2;
    }

    /*
     * Handle error.
     */
    if (rc)
    {
        if (rc > 0)
            rc = -__libc_native2errno(rc);

        /* If we don't have write access, EBADF should be returned, not EACCES. */
        if (    rc == -EACCES
            &&  (pFH->fFlags & O_ACCMODE) != O_WRONLY
            &&  (pFH->fFlags & O_ACCMODE) != O_RDWR)
            rc = -EBADF;
        errno = -rc;
        LIBCLOG_ERROR_RETURN_INT(-1);
    }
    LIBCLOG_RETURN_INT(cbWritten);
}

