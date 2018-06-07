/* $Id: __convcp.c 2240 2005-07-10 09:12:06Z bird $ */
/** @file
 *
 * Locale - code page translation.
 *
 * Copyright (c) 2003 InnoTek Systemberatung GmbH
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

#include <InnotekLIBC/locale.h>
#include <string.h>

/**
 * Convert an ASCIIZ codepage to UCS-2 form and convert POSIX codepage
 * names to names understood by OS/2 Unicode API.
 */
extern void __libc_TranslateCodepage(const char *cp, UniChar *ucp)
{
    static const struct _cp_aliases
    {
        const char *pszAlias;
        const char *pszIBM;
    } aAliases[] =
    {   /* pszAlias   ->      pszIBM */
        { "SYSTEM",           ""},
        /* different ways of saying ASCII */
        { "ASCII",              "IBM-367"},
        { "US-ASCII",           "IBM-367"},
        { "ISO-646-US",         "IBM-367"},
        { "ANSI_X3.4-1986",     "IBM-367"},
        { "ISO-IR-6",           "IBM-367"},
        { "ANSI_X3.4-1968",     "IBM-367"},
        { "ISO_646.IRV:1991",   "IBM-367"},
        { "ISO646-US",          "IBM-367"},
        { "IBM367",             "IBM-367"},
        { "CP367",              "IBM-367"},
        { "UTF-8",              "IBM-1208"},
        { "UTF8",               "IBM-1208"},
        { "UCS-2",              "IBM-1200"},
        { "UCS2",               "IBM-1200"},
        { "UCS-2BE",            "IBM-1200@endian=big"},
        { "UCS-2LE",            "IBM-1200@endian=little"},
        { "EUC-JP",             "IBM-954"},
        { "EUC-KR",             "IBM-970"},
        { "EUC-TW",             "IBM-964"},
        { "EUC-CN",             "IBM-1383"},
        { "BIG5",               "IBM-950"},
    };
    unsigned    i;
    size_t      sl;

    /*
     * Try aliases.
     */
    for (i = 0; i < sizeof(aAliases) / sizeof(aAliases[0]); i++)
    {
        if (!strcmp(cp, aAliases[i].pszAlias))
        {
            cp = aAliases[i].pszIBM;
            while ((*ucp++ = *cp++) != '\0')
                /* nada */;
            return;
        }
    }

    /*
     * Generic transformations:
     *    CPxxxx     -> IBM-xxxx
     *    ISO-xxxx-x -> ISOxxxx-x
     */
    sl = 0;
    /* Transform CPXXX naming style to IBM-XXX style */
    if (   (cp[0] == 'C' || cp[0] == 'c')
        && (cp[1] == 'P' || cp[1] == 'p'))
    {
        ucp[sl++] = 'I';
        ucp[sl++] = 'B';
        ucp[sl++] = 'M';
        ucp[sl++] = '-';
        cp += 2;
    }
    /* Transform ISO-XXXX-X naming style to ISOXXXX-X style */
    else if (   (cp[0] == 'I' || cp[0] == 'i')
             && (cp[1] == 'S' || cp[1] == 's')
             && (cp[2] == 'O' || cp[2] == 'o')
             && (cp[3] == '-'))
    {
        ucp[sl++] = 'I';
        ucp[sl++] = 'S';
        ucp[sl++] = 'O';
        cp += 4;
    }

    /* copy the rest of the string. */
    while (*cp != '\0')
        ucp[sl++] = *cp++;
    ucp[sl] = '\0';
}
