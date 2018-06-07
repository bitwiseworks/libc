/* $Id: os2error.c 1519 2004-09-27 02:15:07Z bird $ */
/** @file
 *
 * LIBC Backend - DosError() Wrapping.
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
#define INCL_BASE
#define INCL_FSMACROS
#include <os2emx.h>
#include <InnoTekLIBC/os2error.h>



/*******************************************************************************
*   Global Variables                                                           *
*******************************************************************************/
/** We're assuming defaults here which might beat us later... */
static ULONG    gfError = FERR_ENABLEHARDERR | FERR_ENABLEEXCEPTION;


/**
 * Sets the error notifications.
 *
 * @returns See DosError().
 * @param   fError  DosError().
 */
int __libc_OS2ErrorSet(ULONG fError)
{
    FS_VAR();
    FS_SAVE_LOAD();
    int rc = DosError(fError);
    if (!rc)
        gfError = fError;
    FS_RESTORE();
    return rc;
}


/**
 * Pushed (and sets) the error notifications setting.
 *
 * @returns See DosError().
 * @param   pfPushVar   Where to store the current value.
 * @param   fError      DosError().
 */
int __libc_OS2ErrorPush(PULONG pfPushVar, ULONG fError)
{
    FS_VAR();
    FS_SAVE_LOAD();
    *pfPushVar = gfError;
    int rc = DosError(fError);
    if (!rc)
        gfError = fError;
    FS_RESTORE();
    return rc;
}


/**
 * Pop the error notifications setting.
 *
 * @returns See DosError().
 * @param   fPopVar     The value to pop.
 */
int __libc_OS2ErrorPop(ULONG fPopVar)
{
    FS_VAR();
    FS_SAVE_LOAD();
    int rc = DosError(fPopVar);
    if (!rc)
        gfError = fPopVar;
    FS_RESTORE();
    return rc;
}

