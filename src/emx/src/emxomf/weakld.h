/* $Id: weakld.h 2815 2006-09-11 01:19:43Z bird $ */
/** @file
 *
 * Weak Pre-Linker.
 *
 * Copyright (c) 2003 InnoTek Systemberatung GmbH
 * Author: knut st. osmundsen <bird-srcspam@anduin.net>
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with This program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef __weak_h__
#define __weak_h__

/*******************************************************************************
*   Structures and Typedefs                                                    *
*******************************************************************************/
struct wld;
/** Weak LD Instance Pointer. */
typedef struct wld * PWLD;

/** wld_create() flags. */
enum wld_create_flags
{
    /** Verbose */
    WLDC_VERBOSE        = 1,
    /** Skip extended dictionary of libraries. */
    WLDC_NO_EXTENDED_DICTIONARY_SEARCH = 2,
    /** Case in-sensitive symbol resolution. */
    WLDC_CASE_INSENSITIVE = 4,
    /** The linker is link386. */
    WLDC_LINKER_LINK386 = 0x1000,
    /** The linker is wlink. */
    WLDC_LINKER_WLINK = 0x2000
};


/*******************************************************************************
*   Functions                                                                  *
*******************************************************************************/
/** @group Weak LD - Public methods.
 * @{ */
PWLD    WLDCreate(unsigned fFlags);
int     WLDAddObject(PWLD pWld, FILE *phFile, const char *pszName);
int     WLDAddDefFile(PWLD pWld, FILE *phFile, const char *pszName);
int     WLDAddLibrary(PWLD pWld, FILE *phFile, const char *pszName);
int     WLDPass1(PWLD pWld);
int     WLDGenerateWeakAliases(PWLD pwld, char *pszObjName, char *pszDefName);
int     WLDDestroy(PWLD pWld);
/** @} */

#endif
