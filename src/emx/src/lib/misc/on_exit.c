/* $Id: on_exit.c 2786 2006-08-27 14:26:13Z bird $ */
/** @file
 *
 * LIBC on_exit().
 *
 * Copyright (c) 2005-2006 knut st. osmundsen <bird@anduin.net>
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
#include <stdlib.h>
#include <InnoTekLIBC/atexit.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_INITTERM
#include <InnoTekLIBC/logstrict.h>


int _STD(on_exit)(void (*pfnCallback)(int iExit, void *pvUser), void *pvUser)
{
    LIBCLOG_ENTER("pfnCallback=%p pvUser=%p\n", (void *)pfnCallback, pvUser);
    __LIBC_PATEXIT pCur = __libc_atexit_new((void *)pfnCallback);
    if (pCur)
    {
        pCur->u.OnExit.pfnCallback = pfnCallback;
        pCur->u.OnExit.pvUser = pvUser;
        pCur->enmType = __LIBC_ATEXITTYPE_ONEXIT;
        LIBCLOG_RETURN_INT(0);
    }
    LIBCLOG_ERROR_RETURN_INT(-1);
}

