/* $Id: wctype.c 1705 2004-12-06 02:19:54Z bird $ */
/** @file
 *
 * wctype.
 *
 * Copyright (c) 2004 knut st. osmundsen <bird@anduin.net>
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
#include <wctype.h>
#include <string.h>


/**
 * Convert a property string to flag value.
 */
wctype_t _STD(wctype)(const char *pszProperty)
{
    static const struct
    {
        const char     *psz;
        unsigned        cch;
        wctype_t        uf;
    } aProps[] =
    {
        #define STR(s) s, sizeof(s) - 1
        { STR("alnum"),     __CT_ALPHA | __CT_DIGIT },
	{ STR("alpha"),     __CT_ALPHA },
	{ STR("blank"),     __CT_BLANK },
	{ STR("cntrl"),     __CT_CNTRL },
	{ STR("digit"),     __CT_DIGIT },
	{ STR("graph"),     __CT_GRAPH },
	{ STR("lower"),     __CT_LOWER },
	{ STR("print"),     __CT_PRINT },
	{ STR("punct"),     __CT_PUNCT },
	{ STR("space"),     __CT_SPACE },
	{ STR("upper"),     __CT_UPPER },
	{ STR("xdigit"),    __CT_XDIGIT },
        /* extensions: */
	{ STR("ascii"),     __CT_ASCII },	
	{ STR("ideogram"),  __CT_IDEOGRAM },	
	{ STR("number"),    __CT_NUMBER },	
	{ STR("rune"),      ~__CT_NUM_MASK},
	{ STR("symbol"),    __CT_SYMBOL },	
        #undef STR
    };
    unsigned cch = strlen(pszProperty);
    int i;
    for (i = 0; i < sizeof(aProps) / sizeof(aProps[0]) - 1; i++)
        if (    aProps[i].cch == cch
            &&  !memcmp(aProps[i].psz, pszProperty, cch))
            return aProps[i].uf;
    /* not found. */
    return 0;
}

