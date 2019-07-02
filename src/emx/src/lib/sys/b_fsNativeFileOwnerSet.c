/* $Id: b_fsNativeFileOwnerSet.c 3841 2014-03-16 19:46:11Z bird $ */
/** @file
 * kNIX - internal [l]chown, OS/2.
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
 * Sets the file ownership of a native file.
 *
 * @returns 0 on success.
 * @returns Negative error code (errno.h) on failure.
 * @param   hNative         Native handle, -1 if only the path is available.
 * @param   pszNativePath   The native path.
 * @param   uid             The user id of the new owner, pass -1 to not change it.
 * @param   gid             The group id of the new group, pass -1 to not change it.
 */
int __libc_back_fsNativeFileOwnerSetCommon(intptr_t hNative, const char *pszNativePath, int fUnixEAs, uid_t uid, gid_t gid)
{
    FS_VAR_SAVE_LOAD();
    int rc;
    if (fUnixEAs)
    {
        struct stat StOrg;
        memset(&StOrg, 0, sizeof(StOrg));
        rc = __libc_back_fsUnixAttribsGet(hNative, pszNativePath, &StOrg);
        if (__predict_true(!rc || rc == -ENOTSUP))
        {
            /** @todo check for permissions to change file ownership. */
            if (uid != (uid_t)-1 || gid != (gid_t)-1)
            {
                /* construct FEA2 stuff. */
                #pragma pack(1)
                struct __LIBC_FSUNIXATTRIBSSETOWNER
                {
                    ULONG   cbList;

                    ULONG   offUid;
                    BYTE    fUidEA;
                    BYTE    cbUidName;
                    USHORT  cbUidValue;
                    CHAR    szUidName[sizeof(EA_UID)];
                    USHORT  usUidType;
                    USHORT  cbUidData;
                    uint32_t u32Uid;
                    CHAR    achUidAlign[((sizeof(EA_UID) + 4) & ~3) - sizeof(EA_UID)];

                    ULONG   offGid;
                    BYTE    fGidEA;
                    BYTE    cbGidName;
                    USHORT  usGidValue;
                    CHAR    szGidName[sizeof(EA_GID)];
                    USHORT  usGidType;
                    USHORT  cbGidData;
                    uint32_t u32Gid;
                    CHAR    achGidAlign[((sizeof(EA_GID) + 4) & ~3) - sizeof(EA_GID)];
                } EAs =
                {
                    sizeof(EAs),

                    /* .offUid      =*/ offsetof(struct __LIBC_FSUNIXATTRIBSSETOWNER, offGid)
                                      - offsetof(struct __LIBC_FSUNIXATTRIBSSETOWNER, offUid),
                    /* .fUidEA      =*/ 0,
                    /* .cbUidName   =*/ sizeof(EA_UID) - 1,
                    /* .cbUidValue  =*/ sizeof(uint32_t) + 4,
                    /* .szUidName   =*/ EA_UID,
                    /* .usUidType   =*/ EAT_BINARY,
                    /* .cbUidData   =*/ sizeof(uint32_t),
                    /* .u32Uid      =*/ uid != (uid_t)-1 ? uid : StOrg.st_uid,
                    /* .achUidAlign =*/ "",

                    /* .offGid      =*/ 0,
                    /* .fGidEA      =*/ 0,
                    /* .cbGidName   =*/ sizeof(EA_GID) - 1,
                    /* .cbGidValue  =*/ sizeof(uint32_t) + 4,
                    /* .szGidName   =*/ EA_GID,
                    /* .usGidType   =*/ EAT_BINARY,
                    /* .cbGidData   =*/ sizeof(uint32_t),
                    /* .u32Gid      =*/ gid != (gid_t)-1 ? gid : StOrg.st_gid,
                    /* .achGidAlign =*/ "",
                };
                #pragma pack()

                if (uid != -1 && uid != 0)
                    EAs.fUidEA = FEA_NEEDEA;
                if (gid != -1 && gid != 0)
                    EAs.fGidEA = FEA_NEEDEA;

                EAOP2 EaOp2;
                rc = __libc_back_fsNativeSetEAs(hNative, pszNativePath, (PFEA2LIST)&EAs, &EaOp2);
                if (__predict_false(rc != NO_ERROR))
                {
                    LIBCLOG_ERROR2("__libc_back_fsNativeSetEAs('%s',,,,) -> %d, oError=%#lx\n", pszNativePath, rc, EaOp2.oError);
                    if (   rc == ERROR_EAS_NOT_SUPPORTED
                        && (uid == -1 || uid == 0)
                        && (gid == -1 || gid == 0))
                        rc = 0;
                    else
                        rc = -__libc_native2errno(rc);
                }
            }
            else
                rc = 0; /* No change and thus no not-supported complaints. */
        }
    }
    else
    {
        /*
         * In this mode we'll simply check if the file exists and return the
         * appropriate errors if it doesn't or we cannot access it.
         */
        union
        {
            FILESTATUS4     fsts4;
            FILESTATUS4L    fsts4L;
        } info;

#if OFF_MAX > LONG_MAX
        if (__libc_gfHaveLFS)
            rc = DosQueryPathInfo((PCSZ)pszNativePath, FIL_STANDARDL, &info, sizeof(info.fsts4L) - sizeof(info.fsts4L.cbList));
        else
#endif
            rc = DosQueryPathInfo((PCSZ)pszNativePath, FIL_STANDARD, &info, sizeof(info.fsts4) - sizeof(info.fsts4.cbList));

        if (rc != NO_ERROR)
            rc = -__libc_native2errno(rc);
    }
    FS_RESTORE();
    return rc;
}


/**
 * Sets the file ownership of a native file.
 *
 * @returns 0 on success.
 * @returns Negative error code (errno.h) on failure.
 * @param   pszNativePath   The native path.
 * @param   uid             The user id of the new owner, pass -1 to not change it.
 * @param   gid             The group id of the new group, pass -1 to not change it.
 */
int __libc_back_fsNativeFileOwnerSet(const char *pszNativePath, uid_t uid, gid_t gid)
{
    LIBCLOG_ENTER("pszNativePath=%p:{%s} uid=%ld gid=%ld\n", (void *)pszNativePath, pszNativePath, (long)uid, (long)gid);

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
     * Join paths with the file handle base code path.
     */
    int rc = __libc_back_fsNativeFileOwnerSetCommon(-1, pszNativePath, __libc_back_fsInfoSupportUnixEAs(pszNativePath), uid, gid);
    if (__predict_false(rc != 0))
        LIBCLOG_ERROR_RETURN_INT(rc);

    LIBCLOG_RETURN_INT(0);
}

