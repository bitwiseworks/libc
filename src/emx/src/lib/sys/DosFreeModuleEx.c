/* $Id: DosFreeModuleEx.c 3906 2014-10-23 23:50:44Z bird $ */
/** @file
 *
 * DosFreeModuleEx.
 *
 * Copyright (c) 2004-2014 knut st. osmundsen <bird-srcspam@anduin.net>
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
#define INCL_ERRORS
#define INCL_DOSMODULEMGR
#define INCL_PRESERVE_REGISTER_MACROS
#define INCL_EXAPIS
#include <os2emx.h>
#define __LIBC_LOG_GROUP    __LIBC_LOG_GRP_DOSEX
#include <InnoTekLIBC/logstrict.h>
#include "DosEx.h"


/**
 * Free module loaded using the extended APIs.
 */
APIRET APIENTRY DosFreeModuleEx(HMODULE hmod)
{
    LIBCLOG_ENTER("hmod=%lx\n", hmod);
    int rc;

    /*
     * Validate input.
     */
    if (!hmod)
        LIBCLOG_ERROR_RETURN_INT(ERROR_INVALID_HANDLE);

    /*
     * Free module.
     */
    rc = __libc_dosexFree(DOSEX_TYPE_LOAD_MODULE, (unsigned)hmod);
    if (rc == -1)
    {
        PRESERVE_REGS_SAVE_LOAD_SAFE();
        rc = DosFreeModule(hmod);
        PRESERVE_REGS_RESTORE();
    }
    if (!rc)
        LIBCLOG_RETURN_INT(rc);
    LIBCLOG_ERROR_RETURN_INT(rc);
}

