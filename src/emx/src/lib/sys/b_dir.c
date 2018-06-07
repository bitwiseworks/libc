/* $Id: b_dir.c 2545 2006-02-07 05:08:44Z bird $ */
/** @file
 *
 * LIBC SYS Backend - Directory Access.
 *
 * Copyright (c) 2005 knut st. osmundsen <bird@anduin.net>
 *
 *
 * This file is part of InnoTek LIBC.
 *
 * InnoTek LIBC is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * InnoTek LIBC is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with InnoTek LIBC; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */


/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#include "libc-alias.h"
#define INCL_BASE
#define INCL_FSMACROS
#include <os2emx.h>
#include "b_fs.h"
#include "b_dir.h"
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <sys/fcntl.h>
#include <sys/dirent.h>
#include <emx/umalloc.h>
#include <emx/io.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_BACK_IO
#include <InnoTekLIBC/logstrict.h>
#include <InnoTekLIBC/libc.h>


/*******************************************************************************
*   Internal Functions                                                         *
*******************************************************************************/
static int dirClose(__LIBC_PFH pFH, int fh);
static int dirRead(__LIBC_PFH pFH, int fh, void *pvBuf, size_t cbRead, size_t *pcbRead);
static int dirWrite(__LIBC_PFH pFH, int fh, const void *pvBuf, size_t cbWrite, size_t *pcbWritten);
static int dirDuplicate(__LIBC_PFH pFH, int fh, int *pfhNew);
static int dirFileControl(__LIBC_PFH pFH, int fh, int iRequest, int iArg, int *prc);
static int dirIOControl(__LIBC_PFH pFH, int fh, int iIOControl, int iArg, int *prc);
static int dirSeek(__LIBC_PFH pFH, int fh, off_t off, int iMethod, off_t *poffNew);
static int dirSelect(int cFHs, struct fd_set *pRead, struct fd_set *pWrite, struct fd_set *pExcept, struct timeval *tv, int *prc);
static int dirForkChild(__LIBC_PFH pFH, int fh, __LIBC_PFORKHANDLE pForkHandle, __LIBC_FORKOP enmOperation);
static int dirOpen(const char *pszNativePath, unsigned fLibc, int *pfh, __LIBC_PFHDIR *ppFHDir);


/*******************************************************************************
*   Global Variables                                                           *
*******************************************************************************/
/**
 * Directory file handle operations.
 */
static const __LIBC_FHOPS gDirOps =
{
    enmFH_Directory,
    dirClose,
    dirRead,
    dirWrite,
    dirDuplicate,
    dirFileControl,
    dirIOControl,
    //dirSeek,
    dirSelect,
    NULL,
    dirForkChild
};


/**
 * Close operation.
 *
 * @returns 0 on success.
 * @returns OS/2 error code or negated errno on failure.
 * @param   pFH         Pointer to the handle structure to operate on.
 * @param   fh          It's associated filehandle.
 */
static int dirClose(__LIBC_PFH pFH, int fh)
{
    __LIBC_PFHDIR pFHDir = (__LIBC_PFHDIR)pFH;
    LIBCLOG_ENTER("pFH=%p:{.hDir=%lx} fh=%d\n", (void *)pFH, pFHDir->hDir, fh);

    if (pFHDir->hDir != HDIR_CREATE)
    {
        FS_VAR_SAVE_LOAD();
        int rc = DosFindClose(pFHDir->hDir);
        LIBC_ASSERTM(rc == NO_ERROR, "DosFindClose(%lx) -> %d\n", pFHDir->hDir, rc); (void)rc;
        pFHDir->hDir = HDIR_CREATE;
        FS_RESTORE();
    }
    if (pFHDir->uBuf.pv)
    {
        free(pFHDir->uBuf.pv);
        pFHDir->uBuf.pv = NULL;
    }
    if (pFHDir->Core.pszNativePath)
    {
        free(pFHDir->Core.pszNativePath);
        pFHDir->Core.pszNativePath = NULL;
    }

    LIBCLOG_RETURN_INT(0);
}


/**
 * Read operation.
 *
 * @returns 0 on success.
 * @returns OS/2 error code or negated errno on failure.
 * @param   pFH         Pointer to the handle structure to operate on.
 * @param   fh          It's associated filehandle.
 * @param   pvBuf       Pointer to the buffer to read into.
 * @param   cbRead      Number of bytes to read.
 * @param   pcbRead     Where to store the count of bytes actually read.
 */
static int dirRead(__LIBC_PFH pFH, int fh, void *pvBuf, size_t cbRead, size_t *pcbRead)
{
    LIBCLOG_ENTER("pFH=%p fh=%d pvBuf=%p cbRead=%d pcbRead=%p\n", (void *)pFH, fh, pvBuf, cbRead, (void *)pcbRead);
    if (cbRead >= sizeof(struct dirent))
    {
        ssize_t cb = __libc_back_dirGetEntries((__LIBC_PFHDIR)pFH, pvBuf, cbRead);
        if (cb >= 0)
        {
            *pcbRead = cb;
            LIBCLOG_RETURN_INT(0);
        }
        LIBCLOG_ERROR_RETURN_INT(cb);
    }
    LIBCLOG_ERROR_RETURN_INT(-EOVERFLOW); /* not a proper read error, but it's the best I can think of for this situation. */
}


/** Write operation.
 * @returns 0 on success.
 * @returns OS/2 error code or negated errno on failure.
 * @param   pFH         Pointer to the handle structure to operate on.
 * @param   fh          It's associated filehandle.
 * @param   pvBuf       Pointer to the buffer which contains the data to write.
 * @param   cbWrite     Number of bytes to write.
 * @param   pcbWritten  Where to store the count of bytes actually written.
 */
static int dirWrite(__LIBC_PFH pFH, int fh, const void *pvBuf, size_t cbWrite, size_t *pcbWritten)
{
    LIBCLOG_ENTER("pFH=%p fh=%d pvBuf=%p cbWrite=%d pcbWritten=%p\n", (void *)pFH, fh, pvBuf, cbWrite, (void *)pcbWritten);
    LIBCLOG_ERROR_RETURN_INT(-EOPNOTSUPP);
}


/** Duplicate handle operation.
 * @returns 0 on success, OS/2 error code on failure.
 * @param   pFH         Pointer to the handle structure to operate on.
 * @param   fh          It's associated filehandle.
 * @param   pfhNew      Where to store the duplicate filehandle.
 *                      The input value describe how the handle is to be
 *                      duplicated. If it's -1 a new handle is allocated.
 *                      Any other value will result in that value to be
 *                      used as handle. Any existing handle with that
 *                      value will be closed.
 */
static int dirDuplicate(__LIBC_PFH pFH, int fh, int *pfhNew)
{
    /** @todo */
    return -EOPNOTSUPP;
}


/** File Control operation.
 * @returns 0 on success.
 * @returns OS/2 error code or negated errno on failure.
 * @param   pFH         Pointer to the handle structure to operate on.
 * @param   fh          It's associated filehandle.
 * @param   iRequest    Which file file descriptior request to perform.
 * @param   iArg        Argument which content is specific to each
 *                      iRequest operation.
 * @param   prc         Where to store the value which upon success is
 *                      returned to the caller.
 */
static int dirFileControl(__LIBC_PFH pFH, int fh, int iRequest, int iArg, int *prc)
{
    LIBCLOG_ENTER("pFH=%p fh=%d iRequest=%d iArg=%d prc=%p\n", (void *)pFH, fh, iRequest, iArg, (void *)prc);
    LIBCLOG_ERROR_RETURN_INT(-EOPNOTSUPP);
}


/** I/O Control operation.
 * @returns 0 on success.
 * @returns OS/2 error code or negated errno on failure.
 * @param   pFH         Pointer to the handle structure to operate on.
 * @param   fh          It's associated filehandle.
 * @param   iIOControl  Which I/O control operation to perform.
 * @param   iArg        Argument which content is specific to each
 *                      iIOControl operation.
 * @param   prc         Where to store the value which upon success is
 *                      returned to the caller.
 */
static int dirIOControl(__LIBC_PFH pFH, int fh, int iIOControl, int iArg, int *prc)
{
    LIBCLOG_ENTER("pFH=%p fh=%d iIOControl=%d iArg=%d prc=%p\n", (void *)pFH, fh, iIOControl, iArg, (void *)prc);
    LIBCLOG_ERROR_RETURN_INT(-EOPNOTSUPP);
}


/** Seek to a new position in the 'file'.
 *
 * @returns 0 on success.
 * @returns OS/2 error code or negated errno on failure.
 * @param   pFH         Pointer to the handle structure to operate on.
 * @param   fh          It's associated filehandle.
 * @param   off         The offset to seek. The meaning depends on SEEK_.
 * @param   iMethod     The seek method, any of the SEEK_* macros.
 * @param   poffNew     Where to store the new file offset.
 */
static int dirSeek(__LIBC_PFH pFH, int fh, off_t off, int iMethod, off_t *poffNew)
{
    /** @todo Implement reading and seeking. */
    return -1;
}


/** Select operation.
 * The select operation is only performed if all handles have the same
 * select routine (the main worker checks this).
 *
 * @returns 0 on success.
 * @returns OS/2 error code or negated errno on failure.
 * @param   cFHs        Range of handles to be tested.
 * @param   pRead       Bitmap for file handles to wait upon to become ready for reading.
 * @param   pWrite      Bitmap for file handles to wait upon to become ready for writing.
 * @param   pExcept     Bitmap of file handles to wait on (error) exceptions from.
 * @param   tv          Timeout value.
 * @param   prc         Where to store the value which upon success is
 *                      returned to the caller.
 */
static int dirSelect(int cFHs, struct fd_set *pRead, struct fd_set *pWrite, struct fd_set *pExcept, struct timeval *tv, int *prc)
{
    LIBCLOG_ENTER("cFHs=%d pRead=%p pWrite=%p pExcept=%p tv=%p prc=%p\n",
                  cFHs, (void *)pRead, (void *)pWrite, (void *)pExcept, (void *)tv, (void *)prc);
    LIBCLOG_ERROR_RETURN_INT(-EOPNOTSUPP);
}


/** Fork notification - child context.
 * Only the __LIBC_FORK_OP_FORK_CHILD operation is forwarded atm.
 * If NULL it's assumed that no notifiction is needed.
 *
 * @returns 0 on success.
 * @returns OS/2 error code or negated errno on failure.
 * @param   pFH             Pointer to the handle structure to operate on.
 * @param   fh              It's associated filehandle.
 * @param   pForkHandle     The fork handle.
 * @param   enmOperation    The fork operation.
 */
static int dirForkChild(__LIBC_PFH pFH, int fh, __LIBC_PFORKHANDLE pForkHandle, __LIBC_FORKOP enmOperation)
{
    switch (enmOperation)
    {
        case __LIBC_FORK_OP_FORK_CHILD:
        {
            __LIBC_PFHDIR pFHDir = (__LIBC_PFHDIR)pFH;

            /* make pattern */
            char szNativePath[PATH_MAX + 5];
            size_t cch = strlen(pFH->pszNativePath);
            memcpy(szNativePath, pFH->pszNativePath, cch);
            szNativePath[cch] = '/';
            szNativePath[cch + 1] = '*';
            szNativePath[cch + 2] = '\0';

            pFHDir->hDir   = HDIR_CREATE;
            pFHDir->cFiles = pFHDir->cbBuf / 40;
#if OFF_MAX > LONG_MAX
            if (pFHDir->fType == FIL_QUERYEASIZEL)  /* the L version is buggy!! Make sure there is enough space. */
                pFHDir->cFiles = pFHDir->cbBuf / sizeof(FILEFINDBUF4L);
#endif
            pFHDir->uCur.pv = pFHDir->uBuf.pv;
            bzero(pFHDir->uBuf.pv, pFHDir->cbBuf);

            int rc = DosFindFirst((PCSZ)szNativePath,
                                  &pFHDir->hDir,
                                  FILE_NORMAL | FILE_READONLY | FILE_HIDDEN | FILE_SYSTEM | FILE_DIRECTORY | FILE_ARCHIVED,
                                  pFHDir->uBuf.pv,
                                  pFHDir->cbBuf,
                                  &pFHDir->cFiles,
                                  pFHDir->fType);
            if (rc)
            {
                pFHDir->hDir = HDIR_CREATE;
                return rc;
            }
            if (pFHDir->uCurEntry)
            {
                unsigned uCurEntry = pFHDir->uCurEntry;
                pFHDir->uCurEntry = 0;
                dirSeek(&pFHDir->Core, fh, uCurEntry * sizeof(struct dirent), SEEK_SET, NULL);
            }
            break;
        }

        default:
            break;
    }
    return 0;
}


/**
 * Opens a directory.
 * Internal worker used by the other open/inherit calls.
 *
 * @returns 0 on success.
 * @returns Negated errno on failure.
 * @param   pszNativePath       Pointer to the native path. The buffer must be on stack
 *                              and must have space for 4 extra bytes!
 * @param   pfh                 On input the requested filehandle. -1 means any.
 *                              On output (success only) the allocated file handle.
 * @param   ppFHDir             Where to store the handle structure pointer on success.
 */
static int dirOpen(const char *pszNativePath, unsigned fLibc, int *pfh, __LIBC_PFHDIR *ppFHDir)
{
    LIBC_ASSERT((fLibc & (O_BINARY | __LIBC_FH_TYPEMASK)) == (O_BINARY | F_DIR));

    /*
     * Setting up a temporary handle; allocate buffer and suchlike.
     */
    int             rc = -ENOMEM;
    __LIBC_FHDIR    Tmp;
    Tmp.hDir        =  HDIR_CREATE;
    Tmp.cbBuf       = 0xf800;
    Tmp.uBuf.pv     = _lmalloc(Tmp.cbBuf);
    if (!Tmp.uBuf.pv)
    {
        Tmp.cbBuf   = 0x1000;
        Tmp.uBuf.pv = _lmalloc(Tmp.cbBuf);
    }
    if (Tmp.uBuf.pv)
    {
        Tmp.uCur.pv = Tmp.uBuf.pv;
        Tmp.cFiles  = Tmp.cbBuf / 40;
#if OFF_MAX > LONG_MAX
        Tmp.fType   = __libc_gpfnDosOpenL ? FIL_QUERYEASIZEL : FIL_QUERYEASIZE;
        if (Tmp.fType == FIL_QUERYEASIZEL)  /* the L version is buggy!! Make sure there is enough space. */
            Tmp.cFiles = Tmp.cbBuf / sizeof(FILEFINDBUF4L);
#else
        pFD->fType  = FIL_QUERYEASIZE;
#endif
        Tmp.Core.pszNativePath = _hstrdup(pszNativePath);
        if (Tmp.Core.pszNativePath)
        {
            /*
             * Try perform the search (find everything!).
             */
            char   *psz = strchr(pszNativePath, '\0');
            if (psz[-1] != '/' && psz[-1] != '\\')
            {
                psz[0] = '/';
                psz[1] = '*';
                psz[2] = '\0';
            }
            else
            {
                psz[0] = '*';
                psz[1] = '\0';
            }
            bzero(Tmp.uBuf.pv, Tmp.cbBuf);
            FS_VAR_SAVE_LOAD();
            rc = DosFindFirst((PCSZ)pszNativePath,
                              &Tmp.hDir,
                              FILE_NORMAL | FILE_READONLY | FILE_HIDDEN | FILE_SYSTEM | FILE_DIRECTORY | FILE_ARCHIVED,
                              Tmp.uBuf.pv,
                              Tmp.cbBuf,
                              &Tmp.cFiles,
                              Tmp.fType);
            *psz = '\0';
            if (!rc)
            {
                __LIBC_PFH pFH;
                rc = __libc_FHAllocate(*pfh, fLibc, sizeof(__LIBC_FHDIR), &gDirOps, &pFH, pfh);
                if (!rc)
                {
                    pFH->pFsInfo        = __libc_back_fsInfoObjByDev(pFH->Dev);
                    pFH->pszNativePath  = Tmp.Core.pszNativePath;

                    __LIBC_PFHDIR pFHDir = (__LIBC_PFHDIR)pFH;
                    pFHDir->fInUnixTree = 0;
                    pFHDir->fType       = Tmp.fType;
                    pFHDir->uBuf.pv     = Tmp.uBuf.pv;
                    pFHDir->uCur.pv     = Tmp.uCur.pv;
                    pFHDir->cFiles      = Tmp.cFiles;
                    pFHDir->cbBuf       = Tmp.cbBuf;
                    pFHDir->hDir        = Tmp.hDir;
                    pFHDir->uCurEntry   = 0;

                    *ppFHDir = pFHDir;
                    FS_RESTORE();
                    return 0;
                }

                /* bailout */
                DosFindClose(Tmp.hDir);
            }
            FS_RESTORE();
            free(Tmp.Core.pszNativePath);
        }
        free(Tmp.uBuf.pv);
    }

    if (rc > 0)
        rc = -__libc_native2errno(rc);
    return rc;
}


/**
 * Re-opens a directory we inherited from our parent process.
 *
 * @returns 0 on success.
 * @returns Negated error code (errno.h) on failure.
 * @param   fh                  The file handle this directory shall have.
 * @param   pszNativePath       Pointer to the native path.
 * @param   fInUnixTree         Flag indicating whether the open() call was referencing a path in the unix tree or not.
 * @param   fFlags              Filehandle flags.
 * @param   Inode               The inode number of the directory.
 * @param   Dev                 The device number of the device with the directory.
 * @param   uCurEntry           The current position.
 */
int __libc_back_dirInherit(int fh, const char *pszNativePath, unsigned fInUnixTree, unsigned fFlags, ino_t Inode, dev_t Dev, unsigned uCurEntry)
{
    LIBCLOG_ENTER("fh=%d pszNativePath=%p:{%s} fInUnixTree=%d fFlags=%#x Inode=%#llx Dev=%#x uCurEntry=%d\n",
                  fh, (void *)pszNativePath, pszNativePath, fInUnixTree, fFlags, (unsigned long long)Inode, Dev, uCurEntry);

    /*
     * Open the directory.
     */
    __LIBC_PFHDIR pFHDir;
    char szNativePath[PATH_MAX + 5];
    strcpy(szNativePath, pszNativePath);
    int rc = dirOpen(szNativePath, fFlags, &fh, &pFHDir);
    if (!rc)
    {
        pFHDir->Core.Inode = Inode;
        pFHDir->Core.Dev   = Dev;
        pFHDir->fInUnixTree = fInUnixTree;

        /*
         * Seek to the current positions.
         */
        if (uCurEntry)
            dirSeek(&pFHDir->Core, fh, uCurEntry * sizeof(struct dirent), SEEK_SET, NULL);

        LIBCLOG_MSG("pFHDir=%p:{.hDir=%#lx, .fType=%d, .cFiles=%ld, .cbBuf=%#x, .uCurEntry=%d} fh=%d\n",
                    (void *)pFHDir, pFHDir->hDir, pFHDir->fType, pFHDir->cFiles, pFHDir->cbBuf, pFHDir->uCurEntry, fh);
        LIBCLOG_RETURN_INT(0);
    }
    LIBCLOG_ERROR_RETURN_INT(rc);

}


/**
 * Opens a directory specified by a path already made native.
 *
 * @returns File handle (>=0) on success.
 * @returns negative error code (errno.h) on failure.
 * @param   pszNativePath       Pointer to the native path. The buffer must be on stack
 *                              and must have space for 4 extra bytes!
 * @param   fInUnixTree         Set if the native path is in the unix tree.
 * @param   fLibc               The LIBC open() flags.
 * @param   pStat               Pointer to the stat structure for the directory.
 */
int __libc_Back_dirOpenNative(char *pszNativePath, unsigned fInUnixTree, unsigned fLibc, struct stat *pStat)
{
    LIBCLOG_ENTER("pszNativePath=%p:{%s} fInUnixTree=%d fLibc=%#x pStat=%p\n", (void *)pszNativePath, pszNativePath, fInUnixTree, fLibc, (void *)pStat);

    /*
     * Validate input.
     */
    if (!S_ISDIR(pStat->st_mode))
        LIBCLOG_ERROR_RETURN_INT(-ENOTDIR);
    if ((fLibc & O_ACCMODE) != O_RDONLY)
        LIBCLOG_ERROR_RETURN_INT(-EISDIR);
    if ((fLibc & (O_EXCL | O_CREAT)) == (O_EXCL | O_CREAT))
        LIBCLOG_ERROR_RETURN_INT(-EEXIST);
    if (fLibc & (O_TRUNC | O_APPEND))
        LIBCLOG_ERROR_RETURN_INT(-EPERM);
    if (fLibc & (O_TRUNC | O_APPEND | O_SIZE))
        LIBCLOG_ERROR_RETURN_INT(-EINVAL);

    /*
     * Directory handles are binary, period.
     */
    fLibc &= ~(O_TEXT | __LIBC_FH_TYPEMASK | __LIBC_FH_FDFLAGS_MASK);
    fLibc |= O_BINARY | F_DIR;

    /*
     * Open the directory.
     */
    int fh = -1;
    __LIBC_PFHDIR pFHDir;
    int rc = dirOpen(pszNativePath, fLibc, &fh, &pFHDir);
    if (!rc)
    {
        pFHDir->Core.Inode = pStat->st_ino;
        pFHDir->Core.Dev   = pStat->st_dev;
        pFHDir->fInUnixTree = fInUnixTree;

        LIBCLOG_MSG("pFHDir=%p:{.hDir=%#lx, .fType=%d, .cFiles=%ld, .cbBuf=%#x} fh=%d\n",
                    (void *)pFHDir, pFHDir->hDir, pFHDir->fType, pFHDir->cFiles, pFHDir->cbBuf, fh);
        LIBCLOG_RETURN_INT(fh);
    }
    LIBCLOG_ERROR_RETURN_INT(rc);
}



/**
 * Reads directory entries.
 *
 * @returns Number of bytes read.
 * @returns Negative error code (errno.h) on failure.
 *
 * @param   fh      The file handle of an open directory.
 * @param   pvBuf   Where to store the directory entries.
 *                  The returned data is a series of dirent structs with
 *                  variable name size. d_reclen must be used the offset
 *                  to the next struct (from the start of the current one).
 * @param   cbBuf   Size of the buffer.
 *
 */
ssize_t __libc_back_dirGetEntries(__LIBC_PFHDIR pFHDir, void *pvBuf, size_t cbBuf)
{
    /* prepare the native path */
    char szNativePath[PATH_MAX + 5];
    size_t cch = strlen(pFHDir->Core.pszNativePath);
    memcpy(szNativePath, pFHDir->Core.pszNativePath, cch);
    szNativePath[cch++] = '/';

    /* copy data */
    ssize_t cbRet = 0;
    while (cbBuf >= sizeof(struct dirent))
    {
        /*
         * Fill the buffer?
         */
        if (pFHDir->cFiles <= 0)
        {
            FS_VAR_SAVE_LOAD();
            pFHDir->cFiles = pFHDir->cbBuf / 40;
#if OFF_MAX > LONG_MAX
            if (pFHDir->fType == FIL_QUERYEASIZEL)  /* the L version is buggy!! Make sure there is enough space. */
                pFHDir->cFiles = pFHDir->cbBuf / sizeof(FILEFINDBUF4L);
#endif
            pFHDir->uCur.pv = pFHDir->uBuf.pv;

            int rc = DosFindNext(pFHDir->hDir,
                                 pFHDir->uBuf.pv,
                                 pFHDir->cbBuf,
                                 &pFHDir->cFiles);
            FS_RESTORE();
            if (rc)
            {
                /** @todo verify that ERROR_NO_MORE_FILES is returned repeatedly. */
                if (!cbRet && rc != ERROR_NO_MORE_FILES)
                    cbRet = -__libc_native2errno(rc);
                pFHDir->cFiles = 0;
                break;
            }
            LIBC_ASSERT(pFHDir->cFiles);
            if (!pFHDir->cFiles) /* paranoia */
                break;
        }

        /*
         * Fill the return buffer.
         */
        struct dirent *pDir = (struct dirent *)pvBuf;
        pDir->d_reclen = sizeof(struct dirent);
        pDir->d_type = DT_UNKNOWN;
        pDir->d_fileno = -1; /** @todo */
        int fMayHaveEAs;
        if (__predict_true(pFHDir->fType == FIL_QUERYEASIZEL))
        {
            fMayHaveEAs = pFHDir->uCur.p4L->cbList > sizeof(USHORT) * 2 + 1;
            if (pFHDir->uCur.p4L->attrFile & FILE_DIRECTORY)
                pDir->d_type = DT_DIR;
            pDir->d_namlen = pFHDir->uCur.p4L->cchName;
            memcpy(&pDir->d_name[0], &pFHDir->uCur.p4L->achName[0], pDir->d_namlen);
        }
        else
        {
            LIBC_ASSERT(pFHDir->fType == FIL_QUERYEASIZE);
            fMayHaveEAs = pFHDir->uCur.p4->cbList > sizeof(USHORT) * 2 + 1;
            if (pFHDir->uCur.p4->attrFile & FILE_DIRECTORY)
                pDir->d_type = DT_DIR;
            pDir->d_namlen = pFHDir->uCur.p4->cchName;
            memcpy(&pDir->d_name[0], &pFHDir->uCur.p4->achName[0], pDir->d_namlen);
        }
        pDir->d_name[pDir->d_namlen] = '\0';

        /*
         * Get unix attributes mode and ino, try EAs first.
         */
        memcpy(&szNativePath[cch], pDir->d_name, pDir->d_namlen + 1);
        if (fMayHaveEAs)
        {
            if (!__libc_gfNoUnix)
            {
                /** @todo */
                /** @todo */
                fMayHaveEAs = 0;
            }
            else
                fMayHaveEAs = 0;
        }
        if (!fMayHaveEAs)
        {
            ino_t Inode; /** @todo use d_ino directly when we've corrected the structure. */
            __libc_back_fsPathCalcInodeAndDev(szNativePath, &Inode);
            pDir->d_fileno = Inode;
            if (pDir->d_type == DT_UNKNOWN)
                pDir->d_type = DT_REG;
        }

        /*
         * Advance buffers.
         */
        cbBuf -= sizeof(struct dirent);
        cbRet += sizeof(struct dirent);
        pvBuf = (char *)pvBuf + sizeof(struct dirent);
        pFHDir->cFiles--;
        pFHDir->uCur.pu8 += pFHDir->uCur.p4->oNextEntryOffset;
    }
    return cbRet;
}

