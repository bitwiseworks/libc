/* $Id: dlfcn_data.c 1546 2004-10-07 06:42:16Z bird $ */
/** @file
 * dlfcn - global data (thread unsafe, but I don't care)
 *
 * Copyright (c) 2001-2003 knut st. osmundsen <bird@anduin.net>
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
#include "dlfcn_private.h"


/*******************************************************************************
*   Global Variables                                                           *
*******************************************************************************/
enum dlfcn_enmOp    __libc_dlfcn_enmLastOp = dlfcn_none;
unsigned            __libc_dlfcn_uLastError;
char                __libc_dlfcn_szLastError[DLFCN_MAX_ERROR_STR];

