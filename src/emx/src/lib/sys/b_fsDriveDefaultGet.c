/* $Id: b_fsDriveDefaultGet.c 2254 2005-07-17 12:25:44Z bird $ */
/** @file
 *
 * LIBC SYS Backend - _getdrive.
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
 * Gets the default drive of the process.
 *
 * @returns 0 on success.
 * @returns Negative error code (errno.h) on failure.
 * @param   pchDrive    Where to store the default drive.
 */
int __libc_Back_fsDriveDefaultGet(char *pchDrive)
{
    LIBCLOG_ENTER("pchDrive=%p\n", (void *)pchDrive);
    FS_VAR();

    /*
     * Get it and return.
     */
    ULONG ulDrive = 0;
    ULONG ulIgnore = 0;
    FS_SAVE_LOAD();
    int rc = DosQueryCurrentDisk(&ulDrive, &ulIgnore);
    FS_RESTORE();
    if (!rc)
    {
        *pchDrive = ulDrive + 'A' - 1;
        LIBCLOG_RETURN_MSG(rc, "ret 0 - *pchDrive=%c\n", *pchDrive);
    }

    rc = -__libc_native2errno(rc);
    LIBCLOG_ERROR_RETURN_INT(rc);
}


