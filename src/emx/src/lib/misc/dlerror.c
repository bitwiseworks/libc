/* $Id: dlerror.c 2254 2005-07-17 12:25:44Z bird $ */
/** @file
 * dlerror - Get error of last dlfcn operation.
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
#include <stdio.h>
#include <dlfcn.h>
#include <InnoTekLIBC/backend.h>
#include "dlfcn_private.h"
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_LDR
#include <InnoTekLIBC/logstrict.h>


/**
 * Returns the last error.
 * The last error is reset by this call.
 * @returns NULL if not error since last dlerror call.
 *          Error description string.
 *
 * @remark  This function is not thread safe.
 */
const char *_STD(dlerror)(void)
{
    LIBCLOG_ENTER("\n");
    /*
     * Check if any errors pending.
     * Copy the error state if that's the case.
     */
    unsigned uError = __libc_dlfcn_uLastError;
    if (uError)
    {
        int         enmOp = __libc_dlfcn_enmLastOp;
        char        szTmpBuffer[DLFCN_MAX_ERROR];
        szTmpBuffer[0] = '\0';
        strncpy(szTmpBuffer, __libc_dlfcn_szLastError, sizeof(szTmpBuffer));

        /*
         * Reset error state.
         */
        __libc_dlfcn_uLastError     = 0;
        __libc_dlfcn_enmLastOp      = dlfcn_dlerror;
        __libc_dlfcn_szLastError[0] = '\0';

        /*
         * Last operation name.
         */
        const char *pszOp;
        switch (enmOp)
        {
            case dlfcn_dlopen:  pszOp = "dlopen"; break;
            case dlfcn_dlclose: pszOp = "dlclose"; break;
            case dlfcn_dlsym:   pszOp = "dlerror"; break;
            default:            pszOp = "!internal error!"; break;
        }

        /*
         * Format the message.
         */
        if (szTmpBuffer[0] != '\0')
            sprintf(&__libc_dlfcn_szLastError[0], "%s rc=%d extra=%s", pszOp, uError, &szTmpBuffer[0]);
        else
            sprintf(&__libc_dlfcn_szLastError[0], "%s rc=%d",          pszOp, uError);
        LIBCLOG_RETURN_MSG(&__libc_dlfcn_szLastError[0], "ret %p:{%s}\n", (void *)&__libc_dlfcn_szLastError[0], &__libc_dlfcn_szLastError[0]);
    }

    /*
     * No error.
     */
    __libc_dlfcn_uLastError     = 0;
    __libc_dlfcn_enmLastOp      = dlfcn_dlerror;
    __libc_dlfcn_szLastError[0] = '\0';
    LIBCLOG_RETURN_P(NULL);
}

