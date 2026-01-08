/* sys/dup2.c (emx+gcc) -- Copyright (c) 1992-1996 by Eberhard Mattes
                        -- Copyright (c) 2003 by Knut St. Osmunden */

#include "libc-alias.h"
#define INCL_FSMACROS
#include <os2emx.h>
#include <errno.h>
#include <string.h>
#include <emx/umalloc.h>
#include <emx/syscalls.h>
#include <emx/io.h>
#include <sys/fcntl.h>
#include "syscalls.h"
#include "b_fs.h"
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_IO
#include <InnoTekLIBC/logstrict.h>

int __dup2(int fh, int fhNew)
{
    LIBCLOG_ENTER("fh=%d fhNew=%d\n", fh, fhNew);
    PLIBCFH     pFH;
    PLIBCFH     pFHNew;
    int         rc;

    /*
     * Validate the two handles.
     * (Specs saith fhNew must not be below zero.)
     */
    if (    fhNew < 0
        ||  !(pFH = __libc_FH(fh)))
    {
        errno = EBADF; /* Note: Per POSIX specs */
        LIBCLOG_ERROR_RETURN_INT(-1);
    }

    /*
     * If the two filehandles are the same, then just
     * check that it's still around and return accordingly.
     */
    if (fhNew == fh)
    {
        ULONG   ulState;
        FS_VAR();
        FS_SAVE_LOAD();
        rc = 0;
        if (!pFH->pOps)
            rc = DosQueryFHState(fh, &ulState);
        FS_RESTORE();
        if (rc)
        {
            _sys_set_errno(rc);
            LIBCLOG_ERROR_RETURN_INT(-1);
        }
        LIBCLOG_RETURN_INT(fhNew);
    }

    /*
     * Ensure the specified handle is within the bounds
     * of the number of filehandles this process can open.
     * (Specs saith so, and others say OS/2 may crash if fhNew is out of range.)
     */
    rc = __libc_FHEnsureHandles(fhNew);
    if (rc)
    {
        errno = EBADF; /* Note: Per POSIX specs */
        LIBCLOG_ERROR_RETURN_INT(-1);
    }

    /*
     * If fhNew exists and is not of the OS/2 native
     * file handle type, we'll close it here.
     */
    pFHNew = __libc_FH(fhNew);
    if (pFHNew && pFHNew->pOps)
    {
        rc = __libc_FHClose(fhNew);
        if (rc)
            LIBCLOG_ERROR_RETURN_INT(-1); /* it sets errno */
    }

    /*
     * Do the stuff.
     */
    if (!pFH->pOps)
    {
        HFILE   hNew = fhNew;
        FS_VAR();

        FS_SAVE_LOAD();
        rc = DosDupHandle(fh, &hNew);
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
        FS_RESTORE();
    }
    else
        rc = pFH->pOps->pfnDuplicate(pFH, fh, &fhNew);

    /*
     * Done
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

