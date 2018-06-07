/* $Id: b_fsStat.c 2254 2005-07-17 12:25:44Z bird $ */
/** @file
 *
 * LIBC Backend - File System Statistics.
 *
 * Copyright (c) 2004 knut st. osmundsen <bird-srcspam@anduin.net>
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
#include "b_fs.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/mount.h>
#include <sys/syslimits.h>
#include <InnoTekLIBC/backend.h>
#define __LIBC_LOG_GROUP    __LIBC_LOG_GRP_BACK_FS
#include <InnoTekLIBC/logstrict.h>

#define INCL_BASE
#define INCL_FSMACROS
#include <os2emx.h>
#include <InnoTekLIBC/os2error.h>


/*******************************************************************************
*   Internal Functions                                                         *
*******************************************************************************/
static int fsStatInternal(const char *pszPath, struct statfs *pStatFs);

/**
 * Get the statistics for the filesystem which pszPath is located on.
 *
 * @returns 0 on success.
 * @returns Negative error code (errno.h) on failure.
 * @param   pszPath     The path to somewhere in the filesystem.
 * @param   pStatFs     Where to store the obtained information.
 */
int __libc_Back_fsStat(const char *pszPath, struct statfs *pStatFs)
{
    LIBCLOG_ENTER("pszPath=%p:{%s} pStatFs=%p\n", (void *)pszPath, pszPath, (void *)pStatFs);

    /*
     * Validate input.
     */
    if (!pStatFs)
        LIBCLOG_ERROR_RETURN_INT(-EFAULT);

    /*
     * Make absolute path (but no symlink resolving I think).
     */
    /** @todo Check if fsstat() can work on a non-existing path spec. */
    char szNativePath[PATH_MAX];
    int rc = __libc_back_fsResolve(pszPath, BACKFS_FLAGS_RESOLVE_FULL | BACKFS_FLAGS_RESOLVE_DIR_MAYBE, &szNativePath[0], NULL);
    if (rc)
        LIBCLOG_ERROR_RETURN_INT(rc);

    /*
     * Do the query.
     */
    szNativePath[2] = '\0';
    rc = fsStatInternal(&szNativePath[0], pStatFs);

    if (!rc)
        LIBCLOG_RETURN_INT(rc);
    LIBCLOG_ERROR_RETURN_INT(rc);
}



/**
 * Get file system statistics
 *
 * @returns 0 on success.
 * @returns Negative error code (errno.h) on failure.
 * @param   fh          The filehandle of any file within the mounted file system.
 * @param   pStatFs     Where to store the statistics.
 */
int __libc_Back_fsStatFH(int fh, struct statfs *pStatFs)
{
    LIBCLOG_ENTER("fh=%d pStatFs=%p\n", fh, (void *)pStatFs);

    /*
     * Validate input.
     */
    if (!pStatFs)
        LIBCLOG_ERROR_RETURN_INT(-EFAULT);

    /*
     * Try resolve the filehandle to a filename and then
     * pass it to the statfs() backend.
     */
    int     rc = 0;
    char    szFilename[PATH_MAX];
    __LIBC_PFH pFH = __libc_FH(fh);
    if (pFH && pFH->pFsInfo)
        strcpy(szFilename, pFH->pFsInfo->szMountpoint);
    else
        rc = __libc_Back_ioFHToPath(fh, szFilename, sizeof(szFilename));
    if (!rc)
    {
        szFilename[2] = '\0';
        rc = fsStatInternal(szFilename, pStatFs);
    }

    if (!rc)
        LIBCLOG_RETURN_INT(rc);
    LIBCLOG_ERROR_RETURN_INT(rc);
}



/**
 * Get the statistics for all the mounted filesystems.
 *
 * @returns Number of returned statfs structs on success.
 * @returns Number of mounted filesystems on success if paStatFS is NULL
 * @returns Negative error code (errno.h) on failure.
 * @param   paStatFs    Where to to store the statistics.
 * @parma   cStatFS     Number of structures the array pointed to by paStatFs can hold.
 * @param   fFlags      Flags, currently ignored.
 */
int __libc_Back_fsStats(struct statfs *paStatFs, unsigned cStatFs, unsigned fFlags)
{
    LIBCLOG_ENTER("paStatFs=%p cStatFS=%d fFlags=%x\n", (void *)paStatFs, cStatFs, fFlags);

    char        szMountPoint[4] = "C:";
    int         iVolume;
    unsigned    iFS;
    if (!paStatFs)
    {
        /*
         * Count mounted file systems.
         */
        for (iFS = 0, iVolume = 'C'; iVolume <= 'Z'; iVolume++)
        {
            szMountPoint[0] = iVolume;
            if (!fsStatInternal(szMountPoint, NULL))
                iFS++;
        }
    }
    else
    {
        /*
         * Query filesystem info for the first cStatFs mounts.
         */
        for (iFS = 0, iVolume = 'C'; iFS < cStatFs && iVolume <= 'Z'; iVolume++)
        {
            szMountPoint[0] = iVolume;
            if (!fsStatInternal(szMountPoint, &paStatFs[iFS]))
                iFS++;
        }
    }

    LIBCLOG_RETURN_INT(iFS);
}


/**
 * Internal fsStat() which assumes input is valid.
 *
 * @returns 0 on success.
 * @returns Negative error code (errno.h) on failure.
 * @param   pszMountPoint   Path to the filesystem mount point.
 * @param   pStatFs         Where to store the statistics.
 *                          If NULL we'll only check if there is an FS mounted at that point.
 */
static int fsStatInternal(const char *pszMountPoint, struct statfs *pStatFs)
{
    /*
     * Query if anything is attached/mounted at the specified mount point.
     */
    FS_VAR()
    FS_SAVE_LOAD();
    ULONG       fError;
    __libc_OS2ErrorPush(&fError, FERR_DISABLEHARDERR);

    char        fsqBuffer[sizeof(FSQBUFFER2) + 3 * CCHMAXPATH];
    PFSQBUFFER2 pfsq = (PFSQBUFFER2)&fsqBuffer[0];
    ULONG       cb = sizeof(fsqBuffer);
    int rc = DosQueryFSAttach((PCSZ)pszMountPoint, 0, FSAIL_QUERYNAME, pfsq, &cb);
    if (!rc)
    {
        if (pStatFs)
        {
            /*
             * Start filling the structure.
             */
            bzero(pStatFs, sizeof(*pStatFs));
            strncpy(pStatFs->f_fstypename, (char *)&pfsq->szName[pfsq->cbName + 1] /* FSDName */, sizeof(pStatFs->f_fstypename) - 1);
            int fDefDevName = 0;
            if (/*!strcmp(pStatFs->f_fstypename, "LAN") && */pfsq->cbFSAData)
            {
                strncpy(pStatFs->f_mntfromname, (char *)&pfsq->szName[pfsq->cbName + 1 + pfsq->cbFSDName + 1] /* FSDData */, sizeof(pStatFs->f_mntfromname) - 1);
                /* convert UNC labels */
                if (    pStatFs->f_mntfromname[0] == '\\'
                    &&  pStatFs->f_mntfromname[1] == '\\'
                    &&  pStatFs->f_mntfromname[2] != '\\')
                {
                    for (char *psz = pStatFs->f_mntfromname; *psz; psz++)
                        if (*psz == '\\')
                            *psz = '/';
                }
            }
            else
            {
                strcpy(pStatFs->f_mntfromname, "vol_");
                pStatFs->f_mntfromname[4] = pszMountPoint[0];
                pStatFs->f_mntfromname[5] = '\0';
                fDefDevName = 1;
            }
            strcpy(pStatFs->f_mntonname, pszMountPoint);
            if (pStatFs->f_mntonname[2] == '\0' && pStatFs->f_mntonname[1] == ':')
            {
                pStatFs->f_mntonname[2] = '/';
                pStatFs->f_mntonname[3] = '\0';
            }
            pStatFs->f_flags        = MNT_NOSYMFOLLOW;
            pStatFs->f_type         = 0;
            switch (pfsq->iType)
            {
                case FSAT_LOCALDRV:
                    pStatFs->f_type = MNT_LOCAL;
                    break;
                case FSAT_REMOTEDRV:
                    break;
                default:
                    LIBC_ASSERTM_FAILED("Unexpected FS type %d for drive %s\n", pfsq->iType, pszMountPoint);
                    break;
            }

            /*
             * Get allocation information.
             */
            FSALLOCATE  FsAlloc;
            rc = DosQueryFSInfo(pszMountPoint[0] - 'A' + 1, FSIL_ALLOC, &FsAlloc, sizeof(FsAlloc));
            if (!rc)
            {
                pStatFs->f_bsize    = FsAlloc.cbSector * FsAlloc.cSectorUnit;
                pStatFs->f_iosize   = pStatFs->f_bsize;
                pStatFs->f_blocks   = FsAlloc.cUnit;
                pStatFs->f_bfree    = FsAlloc.cUnitAvail;
                pStatFs->f_bavail   = pStatFs->f_bfree;
            }
            else
            {
                LIBC_ASSERTM_FAILED("DosQueryFSInfo(%s, ALLOC,) -> %d\n", pszMountPoint, rc);
                pStatFs->f_bsize    = -1;
                pStatFs->f_iosize   = -1;
                pStatFs->f_blocks   = -1;
                pStatFs->f_bfree    = -1;
                pStatFs->f_bavail   = -1;
            }

            /*
             * Get the serial number.
             */
            FSINFO  FsInfo;
            rc = DosQueryFSInfo(pszMountPoint[0] - 'A' + 1, FSIL_VOLSER, &FsInfo, sizeof(FsInfo));
            if (!rc)
            {
                pStatFs->f_fsid.val[0] = *(int16_t *)&FsInfo.fdateCreation;
                pStatFs->f_fsid.val[1] = *(int16_t *)&FsInfo.ftimeCreation;
                if (fDefDevName)
                {
                    memcpy(pStatFs->f_mntfromname, FsInfo.vol.szVolLabel, FsInfo.vol.cch);
                    pStatFs->f_mntfromname[FsInfo.vol.cch] = '\0';
                }
            }
            else
            {
                LIBC_ASSERTM_FAILED("DosQueryFSInfo(%s, VOL,) -> %d\n", pszMountPoint, rc);
                pStatFs->f_fsid.val[0] = -1;
                pStatFs->f_fsid.val[1] = -1;
            }

            /*
             * Fake a bit of info.
             */
            pStatFs->f_owner       = 0 /*root*/;
            pStatFs->f_files       = -1;
            pStatFs->f_ffree       = -1;
            pStatFs->f_asyncreads  = -1;
            pStatFs->f_asyncwrites = -1;
            pStatFs->f_syncreads   = -1;
            pStatFs->f_syncwrites  = -1;

            rc = 0;
        }
    }
    else
    {
        errno = ENOENT;
        rc = -1;
    }
    __libc_OS2ErrorPop(fError);

    FS_RESTORE();
    return rc;
}

