/* $Id: dlopen.c 2254 2005-07-17 12:25:44Z bird $ */
/** @file
 * dlopen - Open a dynamic load library.
 *
 * Copyright (c) 2001-2004 knut st. osmundsen <bird@anduin.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with This program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#include "libc-alias.h"
#include <string.h>
#include <dlfcn.h>
#include <InnoTekLIBC/backend.h>
#include "dlfcn_private.h"
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_LDR
#include <InnoTekLIBC/logstrict.h>


/**
 * Opens the library pszLibrary.
 *
 * @returns Handle to module.
 *          NULL if open failed.
 * @param   pszLibrary      Name of library to load.
 * @param   fFlags          Flags - ignored.
 */
void *  _STD(dlopen)(const char *pszLibrary, int fFlags)
{
    LIBCLOG_ENTER("pszLibrary=%p:{%s} fFlags=%#x\n", (void *)pszLibrary, pszLibrary, fFlags);
    void   *pvModule;
    char    szError[DLFCN_MAX_ERROR];
    int     rc = __libc_Back_ldrOpen(pszLibrary, fFlags, &pvModule, szError, sizeof(szError));
    __libc_dlfcn_enmLastOp      = dlfcn_dlopen;
    __libc_dlfcn_uLastError     = rc;
    __libc_dlfcn_szLastError[0] = '\0';
    if (!rc)
        LIBCLOG_RETURN_P(pvModule);
    strncat(__libc_dlfcn_szLastError, szError, sizeof(__libc_dlfcn_szLastError));
    LIBCLOG_ERROR_RETURN(NULL, "ret NULL - rc=%d szError=%s\n", rc, __libc_dlfcn_szLastError);
}

