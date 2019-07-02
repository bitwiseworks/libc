/* $Id: b_fsNativeFileModeSet.c 3817 2014-02-19 01:40:34Z bird $ */
/** @file
 * LIBC SYS Backend - internal [l]chmod.
 */

/*
 * Copyright (c) 2005-2014 knut st. osmundsen <bird-src-spam@anduin.net>
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
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_BACK_FS
#include <InnoTekLIBC/logstrict.h>


/**
 * Sets the file access mode of a native file.
 *
 * @returns 0 on success.
 * @returns Negative error code (errno.h) on failure.
 * @param   fh      Handle to file.
 * @param   Mode    The filemode.
 */
int __libc_back_fsNativeFileModeSet(const char *pszNativePath, mode_t Mode)
{
    LIBCLOG_ENTER("pszNativePath=%p:{%s} Mode=%#x\n", (void *)pszNativePath, pszNativePath, Mode);
    union
    {
        FILESTATUS4     fsts4;
        FILESTATUS4L    fsts4L;
    } info;
#if OFF_MAX > LONG_MAX
    int     fLarge = 0;
#endif
    FS_VAR();

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
     * (Devices are subject to mode in POSIX.)
     */
    /** @todo copy device check from the path resolver. */

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
    if (rc)
    {
        rc = -__libc_native2errno(rc);
        FS_RESTORE();
        LIBCLOG_ERROR_RETURN_INT(rc);
    }

    /*
     * Update OS/2 attributes.
     */
#if OFF_MAX > LONG_MAX
    if (fLarge)
    {
        if (Mode & S_IWRITE)
            info.fsts4L.attrFile &= ~FILE_READONLY;
        else
            info.fsts4L.attrFile = FILE_READONLY;
        rc = DosSetPathInfo((PCSZ)pszNativePath, FIL_STANDARDL, &info, sizeof(info.fsts4L) - sizeof(info.fsts4L.cbList), 0);
    }
    else
#endif
    {
        if (Mode & S_IWRITE)
            info.fsts4.attrFile &= ~FILE_READONLY;
        else
            info.fsts4.attrFile |= FILE_READONLY;
        rc = DosSetPathInfo((PCSZ)pszNativePath, FIL_STANDARD, &info, sizeof(info.fsts4) - sizeof(info.fsts4.cbList), 0);
    }
    if (__predict_false(rc != NO_ERROR))
    {
        FS_RESTORE();
        rc = -__libc_native2errno(rc);
        LIBCLOG_ERROR_RETURN_INT(rc);
    }

    /*
     * If in unix mode we'll have to update/add the MODE too.
     */
    if (fUnixEAs)
    {
        mode_t CurMode;
        rc = __libc_back_fsUnixAttribsGetMode(-1, pszNativePath, &CurMode);
        if (__predict_true(!rc || rc == -ENOTSUP))
        {
            /* correct the passed in Mode mask. */
            Mode &= ALLPERMS; /** @todo sticky bit and set uid/gid access validation... */
            if (!(CurMode & ~ALLPERMS))
            {
#if OFF_MAX > LONG_MAX
                if ((fLarge ? info.fsts4L.attrFile : info.fsts4.attrFile) & FILE_DIRECTORY)
#else
                if (info.fsts4.attrFile & FILE_DIRECTORY)
#endif
                    Mode |= S_IFDIR;
                else
                    Mode |= S_IFREG;
            }
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

            if (!S_ISREG(Mode) && !S_ISDIR(Mode))
                EAs.fEA = FEA_NEEDEA;

            /* finally, try update / add the EA. */
            EAOP2 EaOp2;
            rc = __libc_back_fsNativeSetEAs(-1, pszNativePath, (PFEA2LIST)&EAs, &EaOp2);
            if (__predict_false(rc != NO_ERROR))
            {
                LIBCLOG_ERROR("__libc_back_fsNativeSetEAs('%s',,,,) -> %d, oError=%#lx\n", pszNativePath, rc, EaOp2.oError);
                if (rc == ERROR_EAS_NOT_SUPPORTED)
                    rc = NO_ERROR;
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
    FS_RESTORE();

    LIBCLOG_RETURN_INT(0);
}

