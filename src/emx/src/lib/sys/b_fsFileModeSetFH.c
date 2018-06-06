/* $Id: b_fsFileModeSetFH.c 3792 2012-03-23 00:48:44Z bird $ */
/** @file
 *
 * LIBC SYS Backend - fchmod.
 *
 * Copyright (c) 2005 knut st. osmundsen <bird@innotek.de>
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
#define INCL_ERRORS
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
 * Sets the file access mode of a file by filehandle.
 *
 * @returns 0 on success.
 * @returns Negative error code (errno.h) on failure.
 * @param   fh      Handle to file.
 * @param   Mode    The filemode.
 */
int __libc_Back_fsFileModeSetFH(int fh, mode_t Mode)
{
    LIBCLOG_ENTER("fh=%d Mode=%#x\n", fh, Mode);

    /*
     * Get filehandle.
     */
    PLIBCFH pFH;
    int rc = __libc_FHEx(fh, &pFH);
    if (rc)
        LIBCLOG_ERROR_RETURN_INT(rc);

    /*
     * Check the type.
     */
    switch (pFH->fFlags & __LIBC_FH_TYPEMASK)
    {
        /* fail */
        case F_SOCKET:
        case F_PIPE: /* treat as socket for now */
            LIBCLOG_ERROR_RETURN_INT(-EINVAL);
        /* ignore */
        case F_DEV:
            LIBCLOG_RETURN_INT(0);

        /* use the path access. */
        case F_DIR:
            if (__predict_false(!pFH->pszNativePath))
                LIBCLOG_ERROR_RETURN_INT(-EINVAL);
            rc = __libc_back_fsNativeFileModeSet(pFH->pszNativePath, Mode);
            if (rc)
                LIBCLOG_ERROR_RETURN_INT(rc);
            LIBCLOG_RETURN_INT(rc);

        /* treat */
        default:
        case F_FILE:
            break;
    }

    if (!pFH->pOps)
    {
        /*
         * Standard OS/2 file handle.
         */
        FS_VAR();
        FS_SAVE_LOAD();
        FILESTATUS3 fsts3;

        /*
         * Get file info, we don't need EA size nor large file sizes.
         */
        rc = DosQueryFileInfo(fh, FIL_STANDARD, &fsts3, sizeof(FILESTATUS3));
        if (rc)
        {
            FS_RESTORE();
            rc = -__libc_native2errno(rc);
            LIBCLOG_ERROR_RETURN_INT(rc);
        }

        /*
         * If in unix mode we'll have to update/add the MODE too.
         */
        if (    !__libc_gfNoUnix
            &&  pFH->pFsInfo
            &&  pFH->pFsInfo->fUnixEAs)
        {
            mode_t CurMode;
            rc = __libc_back_fsUnixAttribsGetMode(fh, pFH->pszNativePath, &CurMode);
            if (__predict_true(!rc || rc == -ENOTSUP))
            {
                /* correct the passed in Mode mask. */
                Mode &= ALLPERMS; /** @todo sticky bit and set uid/gid access validation... */
                if (!(CurMode & ~ALLPERMS))
                    Mode |= S_IFREG;
                else
                    Mode |= CurMode & ~ALLPERMS;

                /* construct FEA2 stuff. */
                #pragma pack(1)
                struct __LIBC_FSUNIXATTRIBSSETMODE
                {
                    ULONG   cbList;
                    ULONG   off;
                    BYTE    fEA;
                    BYTE    cbName;
                    USHORT  cbValue;
                    CHAR    szName[sizeof(EA_MODE)];
                    USHORT  usType;
                    USHORT  cbData;
                    uint32_t u32Mode;
                } EAs =
                {
                    sizeof(EAs), 0, 0, sizeof(EA_MODE) - 1, sizeof(uint32_t) + 4, EA_MODE, EAT_BINARY, sizeof(uint32_t), Mode
                };
                #pragma pack()
                EAOP2 EaOp2;
                EaOp2.fpGEA2List = NULL;
                EaOp2.fpFEA2List = (PFEA2LIST)&EAs;
                EaOp2.oError = 0;

                if (!S_ISREG(Mode) && !S_ISDIR(Mode))
                    EAs.fEA = FEA_NEEDEA;

                /* finally, try update / add the EA. */
                rc = DosSetFileInfo(fh, FIL_QUERYEASIZE, &EaOp2, sizeof(EaOp2));
                if (__predict_false(rc != NO_ERROR))
                {
                    LIBCLOG_ERROR("DosSetFileInfo(%d,,,,) -> %d, oError=%#lx\n", fh, rc, EaOp2.oError);
                    if (rc != ERROR_EAS_NOT_SUPPORTED && pFH->pszNativePath)
                    {
                        EaOp2.oError = 0;
                        int rc2 = DosSetPathInfo((PCSZ)pFH->pszNativePath, FIL_QUERYEASIZE, &EaOp2, sizeof(EaOp2), 0);
                        if (rc2)
                            LIBCLOG_ERROR("DosSetPathInfo('%s',,,,) -> %d, oError=%#lx\n", pFH->pszNativePath, rc2, EaOp2.oError);
                    }
                    if (rc == ERROR_EAS_NOT_SUPPORTED)
                        rc = 0;
                    else
                        rc = -__libc_native2errno(rc);
                }
            }

            if (__predict_false(rc != 0))
            {
                FS_RESTORE();
                LIBCLOG_ERROR_RETURN_INT(rc);
            }
        }

        /*
         * Update OS/2 bits.
         */
        if (Mode & S_IWRITE)
            fsts3.attrFile &= ~FILE_READONLY;
        else
            fsts3.attrFile |= FILE_READONLY;
        rc = DosSetFileInfo(fh, FIL_STANDARD, &fsts3, sizeof(fsts3));
        FS_RESTORE();
        if (rc)
        {
            rc = -__libc_native2errno(rc);
            LIBCLOG_ERROR_RETURN_INT(rc);
        }
    }
    else
    {
        /*
         * Non-standard handle - fail.
         */
        LIBCLOG_ERROR_RETURN_INT(-EOPNOTSUPP);
    }

    LIBCLOG_RETURN_INT(0);
}

