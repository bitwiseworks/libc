/* $Id: b_fsDirCreate.c 2522 2006-02-05 01:53:28Z bird $ */
/** @file
 *
 * LIBC SYS Backend - mkdir.
 *
 * Copyright (c) 2003-2004 knut st. osmundsen <bird@innotek.de>
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
#define INCL_FSMACROS
#define INCL_ERRORS
#include <os2emx.h>
#include "b_fs.h"
#include "syscalls.h"
#include <errno.h>
#include <sys/stat.h>
#include <InnoTekLIBC/libc.h>
#include <InnoTekLIBC/backend.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_BACK_FS
#include <InnoTekLIBC/logstrict.h>


/**
 * Creates a new directory.
 *
 * @returns 0 on success.
 * @returns -1 and errno on failure.
 * @param   pszPath     Path of the new directory.
 * @param   Mode        Permissions on the created directory.
 */
int __libc_Back_fsDirCreate(const char *pszPath, mode_t Mode)
{
    LIBCLOG_ENTER("pszPath=%s Mode=%#x\n", pszPath, Mode);
    FS_VAR();

    /*
     * Resolve the path.
     */
    char szNativePath[PATH_MAX];
    int rc = __libc_back_fsResolve(pszPath, BACKFS_FLAGS_RESOLVE_PARENT | BACKFS_FLAGS_RESOLVE_DIR, &szNativePath[0], NULL);
    if (__predict_false(rc != 0))
        LIBCLOG_ERROR_RETURN_INT(rc);

    /*
     * Unix attributes.
     */
    PEAOP2 pEaOp2 = NULL;
    if (__predict_true(__libc_back_fsInfoSupportUnixEAs(szNativePath)))
    {
        Mode &= ~__libc_gfsUMask;
        Mode &= S_IRWXG | S_IRWXO | S_IRWXU | S_ISUID | S_ISGID | S_ISTXT | S_ISVTX;
        Mode |= S_IFDIR;

        pEaOp2 = alloca(sizeof(EAOP2) + sizeof(__libc_gFsUnixAttribsCreateFEA2List));
        struct __LIBC_FSUNIXATTRIBSCREATEFEA2LIST *pFEas = (struct __LIBC_FSUNIXATTRIBSCREATEFEA2LIST *)(pEaOp2 + 1);
        *pFEas = __libc_gFsUnixAttribsCreateFEA2List;
        __libc_back_fsUnixAttribsInit(pFEas, szNativePath, Mode);
        pEaOp2->fpGEA2List = NULL;
        pEaOp2->fpFEA2List = (PFEA2LIST)pFEas;
        pEaOp2->oError     = 0;
    }

    /*
     * Create directory.
     */
    FS_SAVE_LOAD();
    rc = DosCreateDir((PCSZ)&szNativePath[0], pEaOp2);
    if (__predict_false(rc == ERROR_EAS_NOT_SUPPORTED))
        rc = DosCreateDir((PCSZ)&szNativePath[0], NULL);
    FS_RESTORE();
    if (__predict_true(!rc))
        LIBCLOG_RETURN_INT(0);

    /*
     * We must return -EEXIST if a fs object by the given name already exists.
     * OS/2 returns ERROR_ACCESS_DENIED in those cases.
     */
    if (rc == ERROR_ACCESS_DENIED)
    {
        FILESTATUS3 fsts3;
        FS_SAVE_LOAD();
        rc = DosQueryPathInfo((PCSZ)&szNativePath[0], FIL_STANDARD, &fsts3, sizeof(fsts3));
        FS_RESTORE();
        rc = !rc ? -EEXIST : -EACCES;
    }
    else
        rc = -__libc_native2errno(rc);
    LIBCLOG_ERROR_RETURN_INT(rc);
}

