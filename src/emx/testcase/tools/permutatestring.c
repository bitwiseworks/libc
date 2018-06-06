/* $Id: permutatestring.c 1533 2004-10-01 03:47:04Z bird $ */
/** @file
 *
 * permutates the caseness of a string.
 *
 * Copyright (c) 2004 knut st. osmundsen <bird@innotek.de>
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

#include <stdio.h>
#include <string.h>
#include <ctype.h>

/**
 *  "ab":
 *          "ab"
 *          "aB"
 *          "AB"
 *          "Ab"
 */


static void permuate(char *psz, char *pch)
{
    char ch = *pch;
    if (!ch)
        return;
    if (toupper(ch) != ch)
    {
        *pch = toupper(ch);
        printf("%s\n", psz);
        permuate(psz, pch + 1);
        *pch = ch;
    }
    else if (tolower(ch) != ch)
    {
        *pch = tolower(ch);
        printf("%s\n", psz);
        permuate(psz, pch + 1);
        *pch = ch;
    }

    permuate(psz, pch + 1);
}

int main(int argc, char **argv)
{
    int i;

    for (i = 1; i < argc; i++)
    {
        char *psz = strdup(argv[i]);
        strlwr(psz);
        printf("%s\n", psz);
        permuate(psz, psz);
    }
    return 0;
}

