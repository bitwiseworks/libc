/* $Id: nl_langinfo-1.c 2055 2005-06-19 08:04:00Z bird $ */
/** @file
 *
 * nl_langinfo(CODESET) testcase.
 *
 * Copyright (c) 2005 knut st. osmundsen <bird@anduin.net>
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
#include <locale.h>
#include <langinfo.h>


int main (int argc, char *argv[])
{
    int rcRet = 0;
    const char *psz1 = setlocale(LC_ALL, "");
    const char *psz2 = setlocale(LC_ALL, NULL);
    const char *psz3 = nl_langinfo(CODESET);
    if (!psz1 || !psz2 || strcmp(psz1, psz2))
    {
        printf("nl_langinfo-1: FAILURE - setlocale(LC_ALL, \"\") -> %s ; setlocale(LC_ALL, NULL) -> %s\n", psz1, psz2);
        rcRet++;
    }
    if (!psz2 || !psz3 || !strstr(psz2, psz3))
    {
        printf("nl_langinfo-1: FAILURE - setlocale(LC_ALL, NULL) -> %s ; nl_langinfo(CODESET) -> %s\n", psz2, psz3);
        rcRet++;
    }

    /*
     * Summary.
     */
    if (!rcRet)
        printf("nl_langinfo-1: SUCCESS\n");
    else
        printf("nl_langinfo-1: FAILURE - %d errors\n", rcRet);
    return rcRet;
}

