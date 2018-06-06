/* $Id: b_fsFileStatFH.c 2522 2006-02-05 01:53:28Z bird $ */
/** @file
 *
 * LIBC SYS Backend - fstat.
 *
 * Copyright (c) 2003-2005 knut st. osmundsen <bird@innotek.de>
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
#define INCL_FSMACROS
#include <os2emx.h>
#include "b_fs.h"
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <emx/io.h>
#include <emx/syscalls.h>
#include <limits.h>
#include "syscalls.h"
#include <InnoTekLIBC/libc.h>
#include <InnoTekLIBC/backend.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_BACK_FS
#include <InnoTekLIBC/logstrict.h>



/**
 * Gets the file stats for a file by filehandle.
 *
 * @returns 0 on success.
 * @returns Negative error code (errno.h) on failure. The content
 *          of *pStat is undefined.
 * @param   fh      Handle to file.
 * @param   pStat   Where to store the stats.
 */
int __libc_Back_fsFileStatFH(int fh, struct stat *pStat)
{
    LIBCLOG_ENTER("fh=%d pStat=%p\n", fh, (void *)pStat);

    /*
     * Get filehandle.
     */
    PLIBCFH pFH;

    int rc = __libc_FHEx(fh, &pFH);
    if (rc)
        LIBCLOG_ERROR_RETURN_INT(rc);

    int fUnixEAs = !__libc_gfNoUnix && pFH->pFsInfo && pFH->pFsInfo->fUnixEAs;
    if (/*!pFH->pOps*/ 1)
    {
        /*
         * Use query handle type to figure out the file type.
         */
        bzero(pStat, sizeof(*pStat));
        /* fake unix stuff */
        //pStat->st_uid = 0;
        //pStat->st_gid = 0;
        pStat->st_ino = pFH->Inode;
        pStat->st_dev = pFH->Dev;
        //pStat->st_rdev = 0;
        pStat->st_nlink = 1;
        pStat->st_blksize = 4096*12; /* 48KB */

        if ((pFH->fFlags & __LIBC_FH_TYPEMASK) == F_FILE)
        {
            /*
             * Regular OS/2 file.
             */
            union
            {
                FILESTATUS4     fsts4;
                FILESTATUS4L    fsts4L;
            } info;
#if OFF_MAX > LONG_MAX
            int     fLarge = 0;
#endif
            pStat->st_mode = S_IFREG;

            /*
             * Get file info.
             * We have a little hack here, temporarily, write only files
             * cannot read EAs.
             */
            FS_VAR_SAVE_LOAD();
#if OFF_MAX > LONG_MAX
            if (__libc_gpfnDosOpenL)
            {
                if (fUnixEAs)
                    rc = DosQueryFileInfo(fh, FIL_QUERYEASIZEL, &info, sizeof(info.fsts4L));
                else
                    rc = -1;
                if (rc)
                {
                    rc = DosQueryFileInfo(fh, FIL_STANDARDL, &info, sizeof(FILESTATUS3L));
                    info.fsts4L.cbList = ~0;
                }
                fLarge = 1;
            }
            else
#endif
            {
                if (fUnixEAs)
                    rc = DosQueryFileInfo(fh, FIL_QUERYEASIZE, &info, sizeof(info.fsts4));
                else
                    rc = -1;
                if (rc)
                {
                    rc = DosQueryFileInfo(fh, FIL_STANDARD, &info, sizeof(FILESTATUS3));
                    info.fsts4.cbList = ~0;
                }
            }
            FS_RESTORE();
            if (rc)
            {
                rc = -__libc_native2errno(rc);
                LIBCLOG_ERROR_RETURN_INT(rc);
            }

            /*
             * Format stats struct.
             *      We know the info struct layouts!
             *      Only cbFile, cbFileAlloc and attrFile need be accessed
             *      using the specific structure.
             */
            /* Times: FAT might not return create and access time. */
            pStat->st_ctime = pStat->st_mtime = _sys_p2t(info.fsts4.ftimeLastWrite, info.fsts4.fdateLastWrite);
            if (    FTIMEZEROP(info.fsts4.ftimeCreation)
                &&  FDATEZEROP(info.fsts4.fdateCreation))
                pStat->st_birthtime = pStat->st_mtime;
            else
                pStat->st_birthtime = _sys_p2t(info.fsts4.ftimeCreation, info.fsts4.fdateCreation);
            if (    FTIMEZEROP(info.fsts4.ftimeLastAccess)
                &&  FDATEZEROP(info.fsts4.fdateLastAccess))
                pStat->st_atime = pStat->st_mtime;
            else
                pStat->st_atime = _sys_p2t(info.fsts4.ftimeLastAccess, info.fsts4.fdateLastAccess);

#if OFF_MAX > LONG_MAX
            if (fLarge)
            {
                pStat->st_size = info.fsts4L.cbFile;
                pStat->st_blocks = info.fsts4L.cbFileAlloc / S_BLKSIZE;
                rc = info.fsts4L.attrFile;
            }
            else
#endif
            {
                pStat->st_size = info.fsts4.cbFile;
                pStat->st_blocks = info.fsts4.cbFileAlloc / S_BLKSIZE;
                rc = info.fsts4.attrFile;
            }
            pStat->st_attr = rc;
            if (rc & FILE_READONLY)
                pStat->st_mode |= S_IROTH | S_IRGRP | S_IRUSR;
            else
                pStat->st_mode |= S_IROTH | S_IRGRP | S_IRUSR | S_IWOTH | S_IWGRP | S_IWUSR;

            /* If in unix mode we'll check the EAs (if any). */
            if (   fUnixEAs
                && (fLarge ? info.fsts4L.cbList : info.fsts4.cbList) >= LIBC_UNIX_EA_MIN)
                __libc_back_fsUnixAttribsGet(fh, pFH->pszNativePath, pStat);
        }
        else
        {
            /*
             * Use the native path if we've got it.
             */
            if (pFH->pszNativePath)
            {
                struct stat st;
                rc = __libc_back_fsNativeFileStat(pFH->pszNativePath, &st);
                if (!rc)
                {
                    *pStat = st;
                    LIBCLOG_RETURN_INT(0);
                }
            }

            /*
             * Ok, try fake something plausible from the handle data.
             */
            if ((pFH->fFlags & O_ACCMODE) == O_RDONLY)
                pStat->st_mode = S_IROTH | S_IRGRP | S_IRUSR;
            else
                pStat->st_mode = S_IROTH | S_IRGRP | S_IRUSR | S_IWOTH | S_IWGRP | S_IWUSR;

            switch (pFH->fFlags & __LIBC_FH_TYPEMASK)
            {
                case F_DEV:
                    pStat->st_mode = S_IFCHR;
                    pStat->st_rdev = pFH->Inode;
                    break;
                case F_SOCKET:
                    pStat->st_mode |= S_IFSOCK | S_IXUSR | S_IXGRP | S_IXOTH;
                    break;
                case F_PIPE:
                    pStat->st_mode |= S_IFIFO;
                    pStat->st_mode &= ~(S_IRWXO | S_IRWXG);
                    break;
                case F_DIR:
                    pStat->st_mode |= S_IFDIR | S_IXUSR | S_IXGRP | S_IXOTH;
                    break;
                default:
                    LIBC_ASSERTM_FAILED("Unknown handle type %#x\n", pFH->fFlags & __LIBC_FH_TYPEMASK);
                    pStat->st_mode |= S_IFREG;
                    break;
            }
        }
    }
    //else
    //{
    //    rc = pFH->pOps->pfnStat(pFH, fh, pStat);
    //    if (rc)
    //    {
    //        if (rc > 0)
    //            rc = -__libc_native2errno(rc);
    //        LIBCLOG_ERROR_RETURN_INT(rc);
    //    }
    //}

    LIBCLOG_MSG("%02x - st_dev:       %#x\n",  offsetof(struct stat, st_dev),       pStat->st_dev);
    LIBCLOG_MSG("%02x - st_attr:      %#x\n",  offsetof(struct stat, st_attr),      pStat->st_attr);
    LIBCLOG_MSG("%02x - st_ino:       %#llx\n",offsetof(struct stat, st_ino),       (long long)pStat->st_ino);
    LIBCLOG_MSG("%02x - st_mode:      %#x\n",  offsetof(struct stat, st_mode),      pStat->st_mode);
    LIBCLOG_MSG("%02x - st_nlink:     %u\n",   offsetof(struct stat, st_nlink),     pStat->st_nlink);
    LIBCLOG_MSG("%02x - st_uid:       %u\n",   offsetof(struct stat, st_uid),       pStat->st_uid);
    LIBCLOG_MSG("%02x - st_gid:       %u\n",   offsetof(struct stat, st_gid),       pStat->st_gid);
    LIBCLOG_MSG("%02x - st_rdev:      %#x\n",  offsetof(struct stat, st_rdev),      pStat->st_rdev);
    LIBCLOG_MSG("%02x - st_lspare:    %#x\n",  offsetof(struct stat, st_lspare),    pStat->st_lspare);
    LIBCLOG_MSG("%02x - st_atime:     %d\n",   offsetof(struct stat, st_atime),     pStat->st_atime);
    LIBCLOG_MSG("%02x - st_mtime:     %d\n",   offsetof(struct stat, st_mtime),     pStat->st_mtime);
    LIBCLOG_MSG("%02x - st_ctime:     %d\n",   offsetof(struct stat, st_ctime),     pStat->st_ctime);
    LIBCLOG_MSG("%02x - st_size:      %lld\n", offsetof(struct stat, st_size),      (long long)pStat->st_size);
    LIBCLOG_MSG("%02x - st_blocks:    %lld\n", offsetof(struct stat, st_blocks),    (long long)pStat->st_blocks);
    LIBCLOG_MSG("%02x - st_blksize:   %u\n",   offsetof(struct stat, st_blksize),   pStat->st_blksize);
    LIBCLOG_MSG("%02x - st_flags:     %#x\n",  offsetof(struct stat, st_flags),     pStat->st_flags);
    LIBCLOG_MSG("%02x - st_gen:       %#x\n",  offsetof(struct stat, st_gen),       pStat->st_gen);
    LIBCLOG_MSG("%02x - st_birthtime: %d\n",   offsetof(struct stat, st_birthtime), pStat->st_birthtime);
    LIBCLOG_RETURN_INT(0);
}

