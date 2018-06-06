/* $Id: dlclose.c 2254 2005-07-17 12:25:44Z bird $ */
/** @file
 * dlclose - Close a dlopen'ed dynamic load library.
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
#include <dlfcn.h>
#include <InnoTekLIBC/backend.h>
#include "dlfcn_private.h"
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_LDR
#include <InnoTekLIBC/logstrict.h>


/**
 * Closes an open library.
 * @param   pvHandle    Handle to close.
 */
int _STD(dlclose)(void *pvHandle)
{
    LIBCLOG_ENTER("pvHandle=%p\n", pvHandle);
    int rc = __libc_Back_ldrClose(pvHandle);
    __libc_dlfcn_enmLastOp      = dlfcn_dlclose;
    __libc_dlfcn_uLastError     = rc;
    __libc_dlfcn_szLastError[0] = '\0';
    if (!rc)
        LIBCLOG_RETURN_INT(0);
    LIBCLOG_ERROR_RETURN(-1, "ret -1 - rc=%d\n", rc);
}

