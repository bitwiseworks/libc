/* $Id: b_fsNativeFileStat.c 3812 2014-02-17 13:25:58Z bird $ */
/** @file
 *
 * LIBC SYS Backend - internal stat.
 *
 * Copyright (c) 2004-2007 knut st. osmundsen <bird-src-spam@innotek.de>
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
#define INCL_BASE
#define INCL_FSMACROS
#include <os2emx.h>
#include "b_fs.h"
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <limits.h>
#include "syscalls.h"
#include <InnoTekLIBC/libc.h>
#include <InnoTekLIBC/pathrewrite.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_BACK_FS
#include <InnoTekLIBC/logstrict.h>


/**
 * Stats a native file.
 *
 * @returns 0 on success.
 * @returns Negative error code (errno.h) on failure.
 * @param   pszNativePath   Path to the file to stat. This path is resolved, no
 *                          processing required.
 * @param   pStat           Where to store the file stats.
 */
int __libc_back_fsNativeFileStat(const char *pszNativePath, struct stat *pStat)
{
    LIBCLOG_ENTER("pszNativePath=%p:{%s} pStat=%p\n", (void *)pszNativePath, pszNativePath, (void *)pStat);
    union
    {
        FILESTATUS4     fsts4;
        FILESTATUS4L    fsts4L;
    } info;
#if OFF_MAX > LONG_MAX
    int     fLarge = 0;
#endif
    FS_VAR();

    bzero(pStat, sizeof(*pStat));

    /*
     * Validate input, refusing named pipes.
     */
    if (    (pszNativePath[0] == '/' || pszNativePath[0] == '\\')
        &&  (pszNativePath[1] == 'p' || pszNativePath[1] == 'P')
        &&  (pszNativePath[2] == 'i' || pszNativePath[2] == 'I')
        &&  (pszNativePath[3] == 'p' || pszNativePath[3] == 'P')
        &&  (pszNativePath[4] == 'e' || pszNativePath[4] == 'E')
        &&  (pszNativePath[5] == '/' || pszNativePath[5] == '\\'))
        LIBCLOG_ERROR_RETURN_INT(-ENOENT);

    /*
     * If potential device, then perform real check.
     */
    if (    (pszNativePath[1]== 'd' || pszNativePath[1] == 'D')
        &&  (pszNativePath[2] == 'e' || pszNativePath[2] == 'E')
        &&  (pszNativePath[3] == 'v' || pszNativePath[3] == 'V')
        &&  (pszNativePath[4] == '/' || pszNativePath[4] == '\\')
        )
    {
        ULONG       cb = 64;
        PFSQBUFFER2 pfsqb = alloca(cb);
        if (!DosQueryFSAttach((PCSZ)pszNativePath, 0, FSAIL_QUERYNAME, pfsqb, &cb))
        {
            /* fake unix stuff */
            /** @todo this doesn't entirely match the open and file handle stat code.
             * Fortunately, the user will not see that unless won't have the filename... */
            pStat->st_dev = __libc_back_fsPathCalcInodeAndDev(pszNativePath, &pStat->st_ino);
            pStat->st_attr = FILE_NORMAL;
            pStat->st_mode = S_IFCHR | S_IRWXO | S_IRWXG | S_IRWXU;
            pStat->st_uid = 0;
            pStat->st_gid = 0;
            pStat->st_rdev = 0; /* b_ioFileOpen depends on this being 0 for devices! */
            pStat->st_nlink = 1;
            pStat->st_size = 0;
            pStat->st_blocks = 0;
            pStat->st_blksize = 4096 * 12; /* 48kb */
            pStat->st_atime = 0;
            pStat->st_mtime = 0;
            pStat->st_ctime = 0;
            pStat->st_birthtime = 0;
            LIBCLOG_RETURN_INT(0);
        }
    }

    /*
     * Get path info.
     */
    FS_SAVE_LOAD();
    int rc;
    const int fUnixEAs = __libc_back_fsInfoSupportUnixEAs(pszNativePath);
    if (fUnixEAs)
    {
#if OFF_MAX > LONG_MAX
        if (__libc_gfHaveLFS)
        {
            rc = DosQueryPathInfo((PCSZ)pszNativePath, FIL_QUERYEASIZEL, &info, sizeof(info.fsts4L));
            fLarge = 1;
        }
        else
#endif
            rc = DosQueryPathInfo((PCSZ)pszNativePath, FIL_QUERYEASIZE, &info, sizeof(info.fsts4));
        /* If the file is open in write mode, we cannot even get the EA size. stupid.
         * It'll fail with ERROR_SHARING_VIOLATION, which we handle rigth below. */
    }
    else
        rc = ERROR_SHARING_VIOLATION; /* take the fallback path if we don't want EAs. */

    /* Now, if the file is open in write mode, we cannot even get the EA size. stupid. */
    if (rc == ERROR_SHARING_VIOLATION)
    {
#if OFF_MAX > LONG_MAX
        if (__libc_gfHaveLFS)
        {
            rc = DosQueryPathInfo((PCSZ)pszNativePath, FIL_STANDARDL, &info, sizeof(info.fsts4L) - sizeof(info.fsts4L.cbList));
            fLarge = 1;
        }
        else
#endif
            rc = DosQueryPathInfo((PCSZ)pszNativePath, FIL_STANDARD, &info, sizeof(info.fsts4) - sizeof(info.fsts4.cbList));
        info.fsts4L.cbList = LIBC_UNIX_EA_MIN;
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
    pStat->st_mtime = pStat->st_ctime = _sys_p2t(info.fsts4.ftimeLastWrite, info.fsts4.fdateLastWrite);
    if (   FTIMEZEROP(info.fsts4.ftimeCreation)
        && FDATEZEROP(info.fsts4.fdateCreation))
        pStat->st_birthtime = pStat->st_mtime;
    else
        pStat->st_birthtime = _sys_p2t(info.fsts4.ftimeCreation, info.fsts4.fdateCreation);
    if (   FTIMEZEROP(info.fsts4.ftimeLastAccess)
        && FDATEZEROP(info.fsts4.fdateLastAccess))
        pStat->st_atime = pStat->st_mtime;
    else
        pStat->st_atime = _sys_p2t(info.fsts4.ftimeLastAccess, info.fsts4.fdateLastAccess);

#if OFF_MAX > LONG_MAX
    ULONG fAttributes = fLarge ? info.fsts4L.attrFile : info.fsts4.attrFile;
#else
    ULONG fAttributes = info.fsts4.attrFile;
#endif
    pStat->st_attr = fAttributes;
#if OFF_MAX > LONG_MAX
    if (fLarge)
    {
        pStat->st_size = info.fsts4L.cbFile;
        pStat->st_blocks = info.fsts4L.cbFileAlloc / S_BLKSIZE;
    }
    else
#endif
    {
        pStat->st_size = info.fsts4.cbFile;
        pStat->st_blocks = info.fsts4.cbFileAlloc / S_BLKSIZE;
    }
    if (fAttributes & FILE_DIRECTORY)
    {
        /* directory */
        pStat->st_mode = S_IFDIR | S_IRWXO | S_IRWXG | S_IRWXU;
    }
    else
    {
        pStat->st_mode = S_IFREG;
        if (fAttributes & FILE_READONLY)
            pStat->st_mode |= (S_IREAD >> 6) * 0111;
        else
            pStat->st_mode |= ((S_IREAD|S_IWRITE) >> 6) * 0111;

        /* Mark .exe, .com, .cmd and .bat as executables. */
        if ((pStat->st_mode & (S_IFMT | ((S_IEXEC >> 6) * 0111))) == S_IFREG)
        {
            const char *pszExt = _getext(pszNativePath);
            if (   pszExt++
                && pszExt[0] && pszExt[1] && pszExt[2] && !pszExt[3]
                && strstr("!exe!Exe!EXe!EXE!ExE!eXe!eXE!exE"
                          "!com!Com!COm!COM!CoM!cOm!cOM!coM"
                          "!bat!Bat!BAt!BAT!BaT!bAt!bAT!baT"
                          "!btm!Btm!BTm!BTM!BtM!bTm!bTM!btM"
                          "!cmd!Cmd!CMd!CMD!CmD!cMd!cMD!cmD",
                          pszExt)
                   )
                pStat->st_mode |= (S_IEXEC >> 6) * 0111;
        }
    }

    /* fake unix stuff */
    pStat->st_uid = 0;
    pStat->st_gid = 0;
    pStat->st_dev = 0;
    pStat->st_ino = 0;
    pStat->st_rdev = 0;
    pStat->st_nlink = 1;
    pStat->st_blksize = 4096 * 12; /* 48kb */
    /* If in unix mode we'll check the EAs (if any). */
    rc = 1;
    if (    fUnixEAs
        && (fLarge ? info.fsts4L.cbList : info.fsts4.cbList) >= LIBC_UNIX_EA_MIN)
        rc = __libc_back_fsUnixAttribsGet(-1, pszNativePath, pStat);
    if (rc)
        pStat->st_dev = __libc_back_fsPathCalcInodeAndDev(pszNativePath, &pStat->st_ino);

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
