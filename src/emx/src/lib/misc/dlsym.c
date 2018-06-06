/* $Id: dlsym.c 2254 2005-07-17 12:25:44Z bird $ */
/** @file
 * dlsym - Find a symbol in a dlopen'ed dynamic load library.
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
 * Find the address of a given symbol within the given library.
 *
 * @returns Address of symbol.
 *          NULL if not found.
 * @param   pvHandle    Module handle returned by dlopen.
 * @param   pszSymbol   Name of the symbol.
 *                      This can be an ordinal on OS/2.
 */
void * _STD(dlsym)(void *pvHandle, const char *pszSymbol)
{
    LIBCLOG_ENTER("pvHandle=%p pszSymbol=%p:{%s}\n", pvHandle, pszSymbol, pszSymbol);
    void   *pfn;
    int rc = __libc_Back_ldrSymbol(pvHandle, pszSymbol, &pfn);
    __libc_dlfcn_enmLastOp      = dlfcn_dlsym;
    __libc_dlfcn_uLastError     = rc;
    __libc_dlfcn_szLastError[0] = '\0';
    if (!rc)
        LIBCLOG_RETURN_P(pfn);

    strncat(__libc_dlfcn_szLastError, pszSymbol, sizeof(__libc_dlfcn_szLastError));
    LIBCLOG_ERROR_RETURN(NULL, "ret NULL (rc=%d)\n", rc);
}

/**
 * BSD convenience.
 * Exactly the same as dlsym, only that it's return type is a function pointer.
 *
 * @returns Address of symbol.
 *          NULL if not found.
 * @param   pvHandle    Module handle returned by dlopen.
 * @param   pszSymbol   Name of the symbol.
 *                      This can be an ordinal on OS/2.
 */
dlfunc_t dlfunc(void * __restrict pvHandle, const char * __restrict pszSymbol)
{
    LIBCLOG_ENTER("pvHandle=%p pszSymbol=%p:{%s}\n", pvHandle, pszSymbol, pszSymbol);
    dlfunc_t pfn = (dlfunc_t)dlsym(pvHandle, pszSymbol);
    if (pfn)
        LIBCLOG_RETURN_P(pfn);
    LIBCLOG_ERROR_RETURN(pfn, "ret NULL\n");
}

