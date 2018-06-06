/* $Id: b_threadEnd.c 1576 2004-10-10 11:15:31Z bird $ */
/** @file
 *
 * LIBC SYS Backend - Thread end.
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
#include <errno.h>
#define INCL_DOS
#define INCL_FSMACROS
#include <os2emx.h>
#include <InnoTekLIBC/backend.h>

void __libc_Back_threadEnd(void *pExpRegRec)
{
    FS_VAR();
    FS_SAVE_LOAD();
    DosUnsetExceptionHandler(pExpRegRec);
    FS_RESTORE();
}
