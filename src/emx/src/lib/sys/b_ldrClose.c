/* $Id: b_ldrClose.c 3811 2014-02-16 22:56:50Z bird $ */
/** @file
 *
 * LIBC SYS Backend - dlclose.
 *
 * Copyright (c) 2004-2014 knut st. osmundsen <bird@innotek.de>
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
#define INCL_EXAPIS
#define INCL_FSMACROS
#include <os2emx.h>
#include <InnoTekLIBC/backend.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_BACK_LDR
#include <InnoTekLIBC/logstrict.h>


/**
 * Closes a shared library.
 *
 * @returns 0 on success.
 * @returns Native error number.
 * @param   pvModule        Module handle.
 */
int  __libc_Back_ldrClose(void *pvModule)
{
    LIBCLOG_ENTER("pvModule=%p\n", pvModule);
    int rc;
    if (pvModule == __LIBC_BACK_LDR_GLOBAL)
        rc = NO_ERROR;
    else
    {
        FS_VAR();
        FS_SAVE_LOAD();
        int rc = DosFreeModuleEx((HMODULE)pvModule);
        FS_RESTORE();
    }
    if (!rc)
        LIBCLOG_RETURN_INT(rc);
    LIBCLOG_ERROR_RETURN_INT(rc);
}



