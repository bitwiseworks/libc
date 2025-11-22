/* sys/read.c (emx+gcc) -- Copyright (c) 1992-1996 by Eberhard Mattes
                        -- Copyright (c) 2003 by Knut St. Osmunden */

#include "libc-alias.h"
#define INCL_PRESERVE_REGISTER_MACROS
#define INCL_ERRORS
#include <os2emx.h>
#include <errno.h>
#include <stdlib.h>
#include <memory.h>
#include <sys/fcntl.h>
#include <emx/umalloc.h>
#include <emx/io.h>
#include <emx/syscalls.h>
#include "syscalls.h"
#define __LIBC_LOG_GROUP    __LIBC_LOG_GRP_BACK_IO
#include <InnoTekLIBC/logstrict.h>

int __read (int handle, void *buf, size_t cbToRead)
{
    LIBCLOG_ENTER("fh=%d buf=%p cbToRead=%zu\n", handle, buf, cbToRead);
    int     rc;
    PLIBCFH pFH;
    ULONG   cbRead;

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
        void   *pvBuf_safe = NULL;

        /*
         * Devices doesn't like getting high addresses.
         *      Allocate a buffer in the low heap.
         */
        if (    (pFH->fFlags & __LIBC_FH_TYPEMASK) == F_DEV
            &&  (unsigned)buf >= 512*1024*1024)
        {
            pvBuf_safe = _lmalloc(cbToRead);
            if (!pvBuf_safe)
            {
                errno = ENOMEM;
                LIBCLOG_ERROR_RETURN_INT(-1);
            }
            memcpy(pvBuf_safe, buf, cbToRead);
        }

        PRESERVE_REGS_SAVE_LOAD_SAFE();
        rc = DosRead(handle, pvBuf_safe ? pvBuf_safe : buf, cbToRead, &cbRead);
        PRESERVE_REGS_RESTORE();
        if (pvBuf_safe)
        {
            memcpy(buf, pvBuf_safe, cbToRead);
            free(pvBuf_safe);
        }
    }
    else
    {
        /*
         * Non-standard filehandle.
         */
        size_t  cbRead2;
        rc = pFH->pOps->pfnRead(pFH, handle, buf, cbToRead, &cbRead2);
        cbRead = cbRead2;
    }

    /*
     * Handle errors.
     */
    if (rc)
    {
        if (rc == ERROR_PIPE_NOT_CONNECTED)
            LIBCLOG_RETURN_MSG(0, "ret 0 [pipe-disconnect]\n");
        if (rc > 0)
            rc = -__libc_native2errno(rc);

        /* If we don't have read access, EBADF should be returned, not EACCES. */
        if (    rc == -EACCES
            &&  (pFH->fFlags & O_ACCMODE) != O_RDONLY
            &&  (pFH->fFlags & O_ACCMODE) != O_RDWR)
            rc = -EBADF;
        errno = -rc;
        LIBCLOG_ERROR_RETURN_INT(-1);
    }
    LIBCLOG_RETURN_INT((int)cbRead);
}
