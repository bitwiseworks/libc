/* $Id: _atfork_callback.c 2254 2005-07-17 12:25:44Z bird $ */
/** @file
 *
 * _atfork_callback().
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
#define __LIBC_LOG_GROUP    __LIBC_LOG_GRP_FORK
#include <InnoTekLIBC/logstrict.h>
#include <InnoTekLIBC/fork.h>


int _atfork_callback(__LIBC_PFORKMODULE pModule, __LIBC_PFORKHANDLE pForkHandle, __LIBC_FORKOP enmOperation)
{
    LIBCLOG_ENTER("pModule=%p pForkHandle=%p enmOperation=%d\n", (void *)pModule, (void *)pForkHandle, enmOperation);
    int rc = __libc_ForkDefaultModuleCallback(pModule, pForkHandle, enmOperation);
    LIBCLOG_RETURN_INT(rc);
}

