/* $Id: innidmdll.c 1517 2004-09-19 15:53:31Z bird $ */
/** @file
 *
 * Innotek IDM DLL - demangler DLL for the linker.
 *
 * This DLL is loaded by the linker when a symbol need demangling. This is
 * usually only done for error messages. For some unfortunate reason link386
 * doesn't load the DLL before trying to get the exported functions. It works
 * nice for ilink (which uses 32bit entry points and reloads the dll for
 * every call it seems).
 *
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

/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#include <string.h>
#include <stdlib.h>
#include <demangle.h>

#include "demangle.h"


/*******************************************************************************
*   Defined Constants And Macros                                               *
*******************************************************************************/
#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif


unsigned long _System InitDemangleID32(const char * psInitParms);
unsigned long _System DemangleID32(const char * psMangledName, char * pszPrototype, unsigned long cchPrototype);


/**
 * Initiate the demangler.
 * @returns TRUE (success)
 * @param   psInitParms     Initiations parameter string.
 */
unsigned long _System InitDemangleID32(const char * psInitParms)
{
    return TRUE;
}


/**
 * Demangle name pointed to by pszMangledName.
 *
 * @returns TRUE/FALSE - Success indicator.
 * @param   psMangledName   Symbol to demangle. Pascal string.
 * @param   pszPrototype    Return buffer. (zero string)
 * @param   cchPrototype    Size of the pszPrototype buffer.
 */
unsigned long _System DemangleID32(const char * psMangledName, char * pszPrototype, unsigned long cchPrototype)
{
    char *  pszWeak;
    char *  pszProto;
    char *  pszMang;
    char *  pszMangFree;
    char *  psz;
    int     cch;

    /*
     * Make zero terminated string output of the pascal string.
     */
    pszMang = malloc((unsigned char)*psMangledName + 1);
    if (!pszMang)
        return FALSE;
    memcpy(pszMang, psMangledName + 1, (unsigned char)*psMangledName);
    pszMang[(unsigned char)*psMangledName] = '\0';

    /*
     * Skip any leading underscore.
     */
    pszMangFree = pszMang;
    if (*pszMang == '_')
        pszMang++;

    /*
     * Check if weak, and if so we must
     */
    pszWeak = strstr(pszMang, "$w$");
    if (pszWeak)
        *pszWeak = '\0';

    /*
     * Call the demangler.
     * On failure return mangled name.
     */
    psz = pszProto = cplus_demangle(pszMang, DMGL_PARAMS | DMGL_ANSI);
    if (!psz)
        psz = pszMangFree;

    /*
     * Copy to result buffer.
     */
    cch = strlen(psz);
    if (cch >= cchPrototype)
    {
        memcpy(pszPrototype, psz, cchPrototype - 1);
        pszPrototype[cchPrototype - 1] = '\0';
    }
    else
    {
        memcpy(pszPrototype, psz, cch + 1);
        if (pszWeak && cch + 6 < cchPrototype)
            strcpy(pszPrototype + cch, " weak");
    }

    /*
     * Cleanup.
     */
    free(pszProto);
    free(pszMangFree);

    return 1; /* we never fail */
}

