/* $Id: dlfcn_private.h 1546 2004-10-07 06:42:16Z bird $ */
/** @file
 * dlfcn - global data (thread unsafe, but I don't care)
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

#ifndef __DLFCN_PRIVATE_H_
#define __DLFCN_PRIVATE_H_

/*******************************************************************************
*   Defined Constants And Macros                                               *
*******************************************************************************/
/** Max native error info length. */
#define DLFCN_MAX_ERROR         260
/** Max error string returned by dlerror. */
#define DLFCN_MAX_ERROR_STR     (DLFCN_MAX_ERROR + 64)

/*******************************************************************************
*   Structures and Typedefs                                                    *
*******************************************************************************/
/** Enumeration of possible dlfcn operations in relation to dlerror processing. */
enum dlfcn_enmOp {  dlfcn_none = 0,
                    dlfcn_dlopen, dlfcn_dlsym, dlfcn_dlclose, dlfcn_dlerror};

/*******************************************************************************
*   Global Variables                                                           *
*******************************************************************************/
/** Buffer which have two uses.
 * Primary use is for dlerror return string.
 * Secondary use is for dlopen and dlsym to pass a string to dlerror.
 * Use dlfcn_enmLastOp to determin the content status.
 */
extern char             __libc_dlfcn_szLastError[DLFCN_MAX_ERROR_STR];
/** Last return code from these functions. */
extern unsigned         __libc_dlfcn_uLastError;
/** Last operation. Decides the state of the *LastError variables. */
extern enum dlfcn_enmOp __libc_dlfcn_enmLastOp;


#endif
