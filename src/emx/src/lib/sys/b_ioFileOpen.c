/* $Id: b_ioFileOpen.c 3063 2007-04-08 20:24:07Z bird $ */
/** @file
 *
 * LIBC SYS Backend - open.
 *
 * Copyright (c) 2003-2007 knut st. osmundsen <bird-src-spam@innotek.de>
 * Copyright (c) 1992-1996 by Eberhard Mattes
 *
 *
 * This file is part of InnoTek LIBC.
 *
 * InnoTek LIBC is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * InnoTek LIBC is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with InnoTek LIBC; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */


/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#include "libc-alias.h"
#define INCL_DOSERRORS
#define INCL_FSMACROS
#include <os2emx.h>
#include "b_fs.h"
#include "b_dir.h"
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <share.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <emx/umalloc.h>
#include <emx/syscalls.h>
#include <emx/io.h>
#include "syscalls.h"
#include <InnoTekLIBC/backend.h>
#include <InnoTekLIBC/libc.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_BACK_IO
#include <InnoTekLIBC/logstrict.h>

#define SH_MASK 0x70

/**
 * Opens or creates a file.
 *
 * @returns Filehandle to the opened file on success.
 * @returns Negative error code (errno.h) on failure.
 * @param   pszFile     Path to the file.
 * @param   fLibc       The LIBC open flags (O_*).
 * @param   fShare      The share flags (SH_*).
 * @param   cbInitial   Initial filesize.
 * @param   Mode        The specified permission mask.
 * @param   ppFH        Where to store the LIBC filehandle structure which was created
 *                      for the opened file.
 */
int __libc_Back_ioFileOpen(const char *pszFile, unsigned fLibc, int fShare, off_t cbInitial, mode_t Mode, PLIBCFH *ppFH)
{
    LIBCLOG_ENTER("pszFile=%p:{%s} fLibc=%#x fShare=%#x cbInitial=%lld Mode=0%o ppFH=%p\n",
                  pszFile, pszFile, fLibc, fShare, cbInitial, Mode, (void*)ppFH);

    /*
     * Validate input.
     */
    if ((fLibc & O_ACCMODE) == O_ACCMODE)
        LIBCLOG_ERROR_RETURN_INT(-EINVAL);

    /*
     * The SH_COMPAT mode is weird and unless supported by the host we map it to SH_DENYNO.
     */
    fShare &= SH_MASK;
    if (fShare == SH_COMPAT)
        fShare = SH_DENYNO;

    /*
     * Correct the initial size.
     */
    if (!(fLibc & O_SIZE))
        cbInitial = 0;

    /*
     * Calc the open mode mask from the libc flags and the shared flags in flFlags.
     * Assumes:     OPEN_ACCESS_READONLY == O_RDONLY
     *              OPEN_ACCESS_WRITEONLY == O_WRONLY
     *              OPEN_ACCESS_READWRITE == O_RDWR
     */
    ULONG flOpenMode = fShare & SH_MASK;
    flOpenMode |= fLibc & O_ACCMODE;
    if (fLibc & O_NOINHERIT)
        flOpenMode |= OPEN_FLAGS_NOINHERIT;

    if (fLibc & (O_FSYNC | O_SYNC))
        flOpenMode |= OPEN_FLAGS_WRITE_THROUGH;
    if (fLibc & O_DIRECT)
        flOpenMode |= OPEN_FLAGS_NO_CACHE;

    /*
     * Translate ERROR_OPEN_FAILED to ENOENT unless O_EXCL is set (see below).
     */
    int rcOpenFailed = -ENOENT;

    /*
     * Compute the flOpenFlags using fLibc.
     */
    ULONG flOpenFlags;
    if (fLibc & O_CREAT)
    {
        if (fLibc & O_EXCL)
        {
            flOpenFlags = OPEN_ACTION_FAIL_IF_EXISTS | OPEN_ACTION_CREATE_IF_NEW;
            rcOpenFailed = -EEXIST;
        }
        else if (fLibc & O_TRUNC)
            flOpenFlags = OPEN_ACTION_REPLACE_IF_EXISTS | OPEN_ACTION_CREATE_IF_NEW;
        else
            flOpenFlags = OPEN_ACTION_OPEN_IF_EXISTS | OPEN_ACTION_CREATE_IF_NEW;
    }
    else
    {
        fLibc &= ~O_EXCL; /* O_EXCL doesn't make sense without O_CREAT */
        if (fLibc & O_TRUNC)
            flOpenFlags = OPEN_ACTION_REPLACE_IF_EXISTS | OPEN_ACTION_FAIL_IF_NEW;
        else
            flOpenFlags = OPEN_ACTION_OPEN_IF_EXISTS | OPEN_ACTION_FAIL_IF_NEW;
    }

    /*
     * Resolve the specified file path.
     */
    char szNativePath[PATH_MAX + 4];
    int fInUnixTree = 0;
    int rc = __libc_back_fsResolve(pszFile,
                                   (flOpenFlags & OPEN_ACTION_FAIL_IF_NEW
                                        ? fLibc & O_NOFOLLOW
                                            ? BACKFS_FLAGS_RESOLVE_FULL_SYMLINK       | BACKFS_FLAGS_RESOLVE_DIR_MAYBE
                                            : BACKFS_FLAGS_RESOLVE_FULL               | BACKFS_FLAGS_RESOLVE_DIR_MAYBE
                                        : fLibc & O_NOFOLLOW
                                            ? BACKFS_FLAGS_RESOLVE_FULL_SYMLINK_MAYBE | BACKFS_FLAGS_RESOLVE_DIR_MAYBE
                                            : BACKFS_FLAGS_RESOLVE_FULL_MAYBE         | BACKFS_FLAGS_RESOLVE_DIR_MAYBE),
                                   &szNativePath[0],
                                   &fInUnixTree);
    if (rc)
        LIBCLOG_ERROR_RETURN_INT(rc);

    dev_t   Dev = 0;
    ino_t   Inode = 0;
    PEAOP2 pEaOp2 = NULL;
    if (!__libc_gfNoUnix)
    {
        /*
         * Stat the file to see if it's there, and if it is what kind of file it is.
         * For some of the file types there are race conditions involved, but we don't care.
         */
        struct stat st;
        rc = __libc_back_fsNativeFileStat(szNativePath, &st);
        if (!rc)
        {
            /*
             * If we're truncating a file and no Mode bits are specified, we'll
             * use the ones from the original file.
             */
            /** @todo Check if O_TRUNC and the OS/2 semantics really match.. I don't think O_TRUNC mean replace really. */
            Mode &= ALLPERMS;
            if (!Mode && (fLibc & (O_CREAT | O_TRUNC)) == O_TRUNC)
                Mode = st.st_mode;

            /*
             * Take action based on the type of file we're facing.
             */
            switch (st.st_mode & S_IFMT)
            {
                /*
                 * Regular file:    Presently we don't have to do anything special.
                 */
                case S_IFREG:
                {
                    /** @todo handle st_flags. */
                    break;
                }

                /*
                 * Directory:       Open it as a directory.
                 */
                case S_IFDIR:
                {
                    rc = __libc_Back_dirOpenNative(szNativePath, fInUnixTree, fLibc, &st);
                    if (rc >= 0)
                    {
                        if (ppFH)
                            *ppFH = __libc_FH(rc);
                        LIBCLOG_RETURN_INT(rc);
                    }
                    LIBCLOG_ERROR_RETURN_INT(rc);
                    break;
                }

                /*
                 * Named Pipe:      Open / create the named pipe.
                 */
                case S_IFIFO:
                {
                    /** @todo implement named pipes. */
                    LIBCLOG_ERROR_RETURN_INT(-ENOTSUP);
                }

                /*
                 * Character device or Block device:
                 *                  Translate the major/minor to an OS/2 device name and open that,
                 *                  unless it's a native device of course (see b_fsNativeFileStat.c).
                 */
                case S_IFCHR:
                case S_IFBLK:
                {
                    if (st.st_rdev == 0)
                        break;
                    /** @todo assign major & minor numbers and do translations. */
                    LIBCLOG_ERROR_RETURN_INT(-ENOTSUP);
                }

                /*
                 * Symbolic link:   These cannot be opened by open().
                 */
                case S_IFLNK:
                    LIBCLOG_ERROR_RETURN_INT(-EMLINK);


                /*
                 * Local socket:    Open the local socket.
                 */
                case S_IFSOCK:
                {
                    /** @todo integrate sockets with libc. */
                    LIBCLOG_ERROR_RETURN_INT(-ENOTSUP);
                }

                /*
                 * Stat cannot return anything else.
                 */
                default:
                    __libc_Back_panic(0, NULL, "Invalid file type returned in st_mode=%x for '%s'\n", st.st_mode, szNativePath);
                    break;
            }
        }
        /* else: didn't find it, just go ahead. */

        /*
         * Create Unix attributes for a file which is potentially created / replaced.
         */
        Mode &= ~__libc_gfsUMask;
        Mode &= ACCESSPERMS;
        Mode |= S_IFREG;
        if (    (flOpenFlags & (OPEN_ACTION_CREATE_IF_NEW | OPEN_ACTION_REPLACE_IF_EXISTS))
            &&  __libc_back_fsInfoSupportUnixEAs(szNativePath))
        {
            pEaOp2 = alloca(sizeof(EAOP2) + sizeof(__libc_gFsUnixAttribsCreateFEA2List));
            struct __LIBC_FSUNIXATTRIBSCREATEFEA2LIST *pFEas = (struct __LIBC_FSUNIXATTRIBSCREATEFEA2LIST *)(pEaOp2 + 1);
            *pFEas = __libc_gFsUnixAttribsCreateFEA2List;
            Dev = __libc_back_fsUnixAttribsInit(pFEas, szNativePath, Mode);
            Inode = pFEas->u64INO;
            pEaOp2->fpGEA2List = NULL;
            pEaOp2->fpFEA2List = (PFEA2LIST)pFEas;
            pEaOp2->oError     = 0;
        }
    }
    else
    {
        /* Normalize the mode mask and calc the file attributes. */
        Mode &= ~__libc_gfsUMask;
        Mode &= ACCESSPERMS;
        Mode |= S_IFREG;
    }

    /*
     * Try to open the file.
     */
    FS_VAR_SAVE_LOAD();
    ULONG       flAttr = Mode & S_IWUSR || !Mode ? FILE_NORMAL : FILE_READONLY;
    ULONG       ulAction;
    HFILE       hFile;
    unsigned    cExpandRetries = 0;
    for (;;)
    {
#if OFF_MAX > LONG_MAX
        if (__libc_gfHaveLFS)
        {
            LONGLONG cbInitialTmp = cbInitial;
            rc = DosOpenL((PCSZ)&szNativePath[0], &hFile, &ulAction, cbInitialTmp, flAttr, flOpenFlags, flOpenMode, pEaOp2);
        }
        else
        {
            ULONG cbInitialTmp = (ULONG)cbInitial;
            if (cbInitial > LONG_MAX)
            {
                FS_RESTORE();
                LIBCLOG_ERROR_RETURN_INT(-EOVERFLOW);
            }
            rc = DosOpen((PCSZ)&szNativePath[0], &hFile, &ulAction, cbInitialTmp, flAttr, flOpenFlags, flOpenMode, pEaOp2);
        }
#else
        {
            ULONG cbInitialTmp = cbInitial;
            rc = DosOpen((PCSZ)&szNativePath[0], &hFile, &ulAction, cbInitialTmp, flAttr, flOpenFlags, flOpenMode, pEaOp2);
        }
#endif
        /* Check for EA errors. */
        if (pEaOp2 && rc == ERROR_EAS_NOT_SUPPORTED)
        {
            pEaOp2 = NULL;
            continue;
        }

        /* Check if we're out of handles. */
        if (rc != ERROR_TOO_MANY_OPEN_FILES)
            break;
        if (cExpandRetries++ >= 3)
            break;
        __libc_FHMoreHandles();
    }   /* ... retry 3 times ... */

    if (!rc)
    {
        /*
         * Figure the handle type.
         */
        ULONG   fulType;
        ULONG   fulDevFlags;
        rc = DosQueryHType(hFile, &fulType, &fulDevFlags);
        if (__predict_true(!rc))
        {
            switch (fulType & 0xff)
            {
                default: /* paranoia */
                case HANDTYPE_FILE:
                    fLibc |= F_FILE;
                    /* If a new file the unix EAs needs to be established. */
                    if (    !pEaOp2
                        ||  (ulAction != FILE_CREATED && ulAction != FILE_TRUNCATED)) /** @todo validate that FILE_TRUNCATED will have replaced EAs. */
                        Dev = __libc_back_fsPathCalcInodeAndDev(szNativePath, &Inode);
                    break;
                case HANDTYPE_DEVICE:
                    fLibc |= F_DEV;
                    if (!(fulDevFlags & 0xf))
                        Dev = makedev('c', 0);
                    else if (fulDevFlags & 1 /*KBD*/)
                        Dev = makedev('c', 1);
                    else if (fulDevFlags & 2 /*SCR*/)
                        Dev = makedev('c', 2);
                    else if (fulDevFlags & 4 /*NUL*/)
                        Dev = makedev('c', 4);
                    else /*if (fulDevFlags & 8 / *CLK* /)*/
                        Dev = makedev('c', 8);
                    __libc_back_fsPathCalcInodeAndDev(szNativePath, &Inode);
                    break;
                case HANDTYPE_PIPE:
                    fLibc |= F_PIPE; /** @todo this is a named pipe! */
                    Dev = makedev('p', 0);
                    __libc_back_fsPathCalcInodeAndDev(szNativePath, &Inode);
                    break;
            }

            /*
             * Register the handle and calc Dev and Inode.
             */
            __LIBC_PFH pFH;
            rc = __libc_FHAllocate(hFile, fLibc, sizeof(LIBCFH), NULL, &pFH, NULL);
            if (__predict_true(!rc))
            {
                pFH->Inode          = Inode;
                pFH->Dev            = Dev;
                pFH->pFsInfo        = __libc_back_fsInfoObjByDev(Dev);
                pFH->pszNativePath  = _hstrdup(szNativePath);

                if (ppFH)
                    *ppFH = pFH;
                FS_RESTORE();
                LIBCLOG_MSG("pFH=%p hFile=%#lx fLibc=%#x fulType=%#lx ulAction=%lu Dev=%#x Inode=%#llx\n",
                            (void *)pFH, hFile, fLibc, fulType, ulAction, Dev, Inode);
            }
        }

        if (__predict_false(rc != NO_ERROR))
            DosClose(hFile);
    }
    FS_RESTORE();

    /*
     * Handle any errors.
     */
    if (__predict_false(rc != NO_ERROR))
    {
        if (rc == ERROR_OPEN_FAILED)
            rc = rcOpenFailed;
        else
            rc = -__libc_native2errno(rc);
        LIBCLOG_ERROR_RETURN_INT(rc);
    }
    LIBCLOG_RETURN_INT((int)hFile);
}

