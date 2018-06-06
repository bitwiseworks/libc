/* $Id: b_fsSync.c 1519 2004-09-27 02:15:07Z bird $ */
/** @file
 *
 * LIBC Backend - __libc_Back_fhToPath().
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
#include <InnoTekLIBC/backend.h>
#define __LIBC_LOG_GROUP    __LIBC_LOG_GRP_BACK_FS
#include <InnoTekLIBC/logstrict.h>

#define INCL_DOSFILEMGR
#define INCL_FSMACROS
#include <os2emx.h>


/**
 * Schedules all file system buffers for writing.
 *
 * @remark  DosShutdown(1) was at one point broken, affecting named pipes IIRC.
 */
void __libc_Back_fsSync(void)
{
    LIBCLOG_ENTER("\n");
    FS_VAR();

    FS_SAVE_LOAD();
    DosShutdown(1);
    FS_RESTORE();

    LIBCLOG_RETURN_VOID();
}

