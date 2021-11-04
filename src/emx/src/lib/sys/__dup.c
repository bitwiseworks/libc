/* sys/dup.c (emx+gcc) -- Copyright (c) 1992-1996 by Eberhard Mattes
                       -- Copyright (c) 2003 by Knut St. Osmunden */

#include "libc-alias.h"
#define INCL_FSMACROS
#define INCL_ERRORS
#include <os2emx.h>
#include <errno.h>
#include <emx/io.h>
#include <emx/umalloc.h>
#include <sys/fcntl.h>
#include <emx/syscalls.h>
#include "syscalls.h"
#include "b_fs.h"
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_IO
#include <InnoTekLIBC/logstrict.h>

int __dup(int fh)
{
    LIBCLOG_ENTER("fh=%d\n", fh);
    PLIBCFH     pFH;
    int         fhNew;
    int         rc;

    /*
     * Get filehandle.
     */
    pFH = __libc_FH(fh);
    if (!pFH)
    {
        errno = EBADF;
        LIBCLOG_ERROR_RETURN_INT(-1);
    }

    /*
     * Choose path.
     */
    if (!pFH->pOps)
    {
        int     cExpandRetries;
        HFILE   hNew;
        FS_VAR();
        FS_SAVE_LOAD();
        for (cExpandRetries = 0;;)
        {
            hNew = ~0;
            rc = DosDupHandle(fh, &hNew);
            if (rc != ERROR_TOO_MANY_OPEN_FILES)
                break;
            if (cExpandRetries++ >= 3)
                break;
            /* autoincrement. */
            __libc_FHMoreHandles();
        }   /* ... retry 3 times ... */
        FS_RESTORE();
        fhNew = (int)hNew;
        if (!rc)
        {
            PLIBCFH pFHNew;
            rc = __libc_FHAllocate(hNew, pFH->fFlags, sizeof(LIBCFH), NULL, &pFHNew, NULL);
            if (!rc)
            {
                pFHNew->fFlags      = pFH->fFlags;
                pFHNew->iLookAhead  = pFH->iLookAhead;
                pFHNew->Inode       = pFH->Inode;
                pFHNew->Dev         = pFH->Dev;
                pFHNew->pFsInfo     = __libc_back_fsInfoObjAddRef(pFH->pFsInfo);
                pFHNew->pszNativePath = _hstrdup(pFH->pszNativePath);

                /*
                 * OS/2 does, as SuS specifies, clear the noinherit flag. So, we update
                 * the flags of the new handle and in strict mode call __libc_FHSetFlags
                 * to invoke the out of sync check in there.
                 */
                pFHNew->fFlags &= ~((FD_CLOEXEC << __LIBC_FH_FDFLAGS_SHIFT) | O_NOINHERIT);
                LIBC_ASSERT(!__libc_FHSetFlags(pFHNew, hNew, pFHNew->fFlags));
            }
            else
                DosClose(hNew);
        }
    }
    else
    {
        fhNew = -1; /* any handle */
        rc = pFH->pOps->pfnDuplicate(pFH, fh, &fhNew);
    }

    /*
     * Done.
     */
    if (rc)
    {
        if (rc > 0)
            _sys_set_errno(rc);
        else
            errno = -rc;
        LIBCLOG_ERROR_RETURN_INT(-1);
    }
    LIBCLOG_RETURN_INT(fhNew);
}
