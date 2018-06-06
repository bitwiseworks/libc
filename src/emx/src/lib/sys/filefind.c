/* sys/filefind.c (emx+gcc) -- Copyright (c) 1992-1996 by Eberhard Mattes
   sys/filefind.c (libc)    -- Copyright (c) 2003-2004 by knut st. osmundsen
*/

/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#include "libc-alias.h"
#define INCL_BASE
#define INCL_FSMACROS
#include <os2emx.h>
#include "b_fs.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <emx/syscalls.h>
#include "syscalls.h"
#include <InnoTekLIBC/thread.h>
#include <InnoTekLIBC/pathrewrite.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_BACK_IO
#include <InnoTekLIBC/logstrict.h>

/**
 * Close a directory find session.
 * @param   pFD     The directory find data.
 */
static void find_close(struct find_data *pFD)
{
    if (pFD->hdir != HDIR_CREATE)
    {
        FS_VAR();
        FS_SAVE_LOAD();
        DosFindClose(pFD->hdir);
        FS_RESTORE();
        pFD->hdir = HDIR_CREATE;
    }
    pFD->cFiles = 0;
    pFD->pchNext = NULL;
}

/* Build a `struct _find' structure from a FILEFINDBUF4 structure and
   move to the next one. */

/**
 * Build a 'struct _find' from the next file in the directory find data.
 * @returns 0 on success.
 * @returns -1 on failure, ENOENT as errno.
 * @param   pFD     The directory find data.
 */
static int find_conv(struct find_data *pFD, struct _find *fp)
{
    union
    {
        const char             *pch;
        const FILEFINDBUF4     *pFindbuf4;
        const FILEFINDBUF4L    *pFindbuf4L;
    } u;
    u.pch = pFD->pchNext;

    /*
     * If there ain't any entries, flag ENOENT error, clean up and
     * return failure.
     */
    if (!pFD->cFiles)
    {
        find_close(pFD);
        errno = ENOENT;
        return -1;
    }

    /*
     * Fill-in target object.
     *  About the find structure we know that these fields are not the same
     *  in the two versions:
     *      cbFile, cbFileAlloc, attrFile, cchName, achName.
     */
    fp->time = XUSHORT(u.pFindbuf4->ftimeLastWrite);
    fp->date = XUSHORT(u.pFindbuf4->fdateLastWrite);
#if OFF_MAX > LONG_MAX
    if (pFD->fType == FIL_QUERYEASIZEL)
    {
        fp->cbFile = u.pFindbuf4L->cbFile;
        fp->attr   = (unsigned char)u.pFindbuf4L->attrFile;
        strcpy(fp->szName, &u.pFindbuf4L->achName[0]);
#if 0 //@todo DT_LNK
        if (u.pFindbuf4L->cbList >= LIBC_UNIX_EA_MIN
            && find_is_symlink(u.pFindbuf4L->achName))
            fp->attr |= 0xf0;
#endif
    }
    else
#endif
    {
        fp->cbFile = u.pFindbuf4->cbFile;
        fp->attr   = (unsigned char)u.pFindbuf4->attrFile;
        strcpy(fp->szName, &u.pFindbuf4->achName[0]);
    }

    /*
     * Next entry.
     */
    if (pFD->cFiles && u.pFindbuf4->oNextEntryOffset)
    {
        pFD->pchNext = u.pch + u.pFindbuf4->oNextEntryOffset;
        pFD->cFiles--;
    }
    else
        pFD->cFiles = 0;

    return 0;
}


/**
 * Start a directory find session.
 * It's only possible to perform one such session per thread. A call to this
 * function will terminate the previous one.
 *
 * @returns 0 on success
 * @returns -1 on failure and errno set appropriately.
 * @param   pszName Search pattern. Usually "<somepath>\\*"
 * @param   attr    File attributes to include in the search.
 *                  If 0 files with any attributes may be included.
 * @param   fp      Where to put the data on the first file found.
 */
int __findfirst(const char *pszName, int attr, struct _find *fp)
{
    LIBCLOG_ENTER("pszName=%s attr=%#x fp=%p\n", pszName, attr, (void *)fp);
    int                 rc;
    char                szNativePath[PATH_MAX];
    struct find_data   *pFD = &__libc_threadCurrent()->b.sys.fd;
    FS_VAR();

    /*
     * Rewrite the specified file path.
     */
    rc = __libc_back_fsResolve(pszName, BACKFS_FLAGS_RESOLVE_PARENT, &szNativePath[0], NULL);
    if (rc)
    {
        errno = -rc;
        LIBCLOG_ERROR_RETURN_INT(-1);
    }

    /*
     * Cleanup any open find sessions first.
     */
    if (pFD->hdir != HDIR_CREATE)
    {
        /* Closing the handle is not strictly required as DosFindFirst
           can reuse an open handle.  However, this simplifies error
           handling below (will DosFindFirst close the handle on error
           if it is open?). */
        FS_SAVE_LOAD();
        DosFindClose(pFD->hdir);
        FS_RESTORE();
        pFD->hdir = HDIR_CREATE;
    }

    /*
     * Start file enumeration.
     */
    pFD->cFiles = sizeof(pFD->achBuffer) / 40;
#if OFF_MAX > LONG_MAX
    pFD->fType = __libc_gpfnDosOpenL ? FIL_QUERYEASIZEL : FIL_QUERYEASIZE;
    if (pFD->fType == FIL_QUERYEASIZEL)  /* the L version is buggy!! Make sure there is enough space. */
        pFD->cFiles = sizeof(pFD->achBuffer) / sizeof(FILEFINDBUF4L);
#else
    pFD->fType = FIL_QUERYEASIZE;
#endif
    FS_SAVE_LOAD();
    bzero(&pFD->achBuffer[0], sizeof(pFD->achBuffer));
    rc = DosFindFirst((PCSZ)&szNativePath[0],
                      &pFD->hdir,
                      attr & (FILE_NORMAL | FILE_READONLY | FILE_HIDDEN | FILE_SYSTEM | FILE_DIRECTORY | FILE_ARCHIVED),
                      &pFD->achBuffer[0],
                      sizeof(pFD->achBuffer),
                      &pFD->cFiles,
                      pFD->fType);
    FS_RESTORE();
    if (rc)
    {
        pFD->hdir = HDIR_CREATE; /* Perhaps modified by DosFindFirst */
        pFD->cFiles = 0;
        pFD->pchNext = NULL;
        _sys_set_errno(rc);
        LIBCLOG_ERROR_RETURN_INT(-1);
    }
    pFD->pchNext = &pFD->achBuffer[0];
    rc = find_conv(pFD, fp);
    LIBCLOG_RETURN_INT(rc);
}


/**
 * Get the next file in the current directory find session.
 * It's only possible to perform one such session per thread. A call to this
 * function will terminate the previous one.
 *
 * @returns 0 on success
 * @returns -1 on failure and errno set appropriately.
 * @param   pszName Search pattern. Usually "<somepath>\\*"
 * @param   attr    File attributes to include in the search.
 *                  If 0 files with any attributes may be included.
 * @param   fp      Where to put the data on the first file found.
 */
int __findnext(struct _find *fp)
{
    ULONG               rc;
    struct find_data   *pFD = &__libc_threadCurrent()->b.sys.fd;
    FS_VAR();

    /*
     * Check incoming.
     */
    if (pFD->hdir == HDIR_CREATE)
    {
        errno = EINVAL;
        return -1;
    }

    /*
     * Do we need to fetch more files?
     */
    if (!pFD->cFiles)
    {
        pFD->cFiles = sizeof(pFD->achBuffer) / 40;
        if (pFD->fType == FIL_QUERYEASIZEL)  /* the L version is buggy!! Make sure there is enough space. */
            pFD->cFiles = sizeof(pFD->achBuffer) / sizeof(FILEFINDBUF4L);
        FS_SAVE_LOAD();
        rc = DosFindNext(pFD->hdir, &pFD->achBuffer[0], sizeof(pFD->achBuffer), &pFD->cFiles);
        FS_RESTORE();
        if (rc)
        {
            find_close(pFD);
            _sys_set_errno(rc);
            return -1;
        }
        pFD->pchNext = &pFD->achBuffer[0];
    }

    return find_conv(pFD, fp);
}

