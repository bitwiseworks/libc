/* $Id: b_fsDriveDefaultSet.c 2254 2005-07-17 12:25:44Z bird $ */
/** @file
 *
 * LIBC SYS Backend - _chdrive.
 *
 * Copyright (c) 2004 knut st. osmundsen <bird@innotek.de>
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
#include <InnoTekLIBC/backend.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_BACK_FS
#include <InnoTekLIBC/logstrict.h>


/**
 * Changes the default drive of the process.
 *
 * @returns 0 on success.
 * @returns Negative error code (errno.h) on failure.
 * @param   chDrive     New default drive.
 */
int __libc_Back_fsDriveDefaultSet(char chDrive)
{
    LIBCLOG_ENTER("chDrive=%c (%d)\n", chDrive, chDrive);
    FS_VAR();

    /*
     * Lock the fs global state.
     */
    int rc = __libc_back_fsMutexRequest();
    if (rc)
        LIBCLOG_ERROR_RETURN_INT(rc);

    /*
     * Change the drive and update the in-unix-tree flags.
     */
    ULONG ulDrive = chDrive - (chDrive >= 'A' && chDrive <= 'Z' ? 'A' - 1 : 'a' - 1);
    FS_SAVE_LOAD();
    rc = DosSetDefaultDisk(ulDrive);
    FS_RESTORE();
    if (!rc)
        __libc_gfInUnixTree = 0;
    else
        rc = -__libc_native2errno(rc);

    __libc_back_fsMutexRelease();

    if (!rc)
        LIBCLOG_RETURN_INT(rc);
    LIBCLOG_ERROR_RETURN_INT(rc);
}


