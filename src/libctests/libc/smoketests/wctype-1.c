/* $Id: wctype-1.c 2055 2005-06-19 08:04:00Z bird $ */
/** @file
 *
 * LIBC - extremely incomplete wctype testcase.
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

#include <wctype.h>
#include <ctype.h>
#include <stdio.h>
#include <locale.h>

int main()
{
    int rc = 0;
    if (!setlocale(LC_ALL, "C"))
    {
        printf("setlocale failed!\n");
        return 1;
    }
    printf("iscntrl(0x82) -> %d\n", iscntrl(0x82));
    printf("iswcntrl(0x82) -> %d\n", iswcntrl(0x82));
    if (!!iscntrl(0x82) != !!iswcntrl(0x82))
        printf("!!iscntrl(0x82) != !!iswcntrl(0x82) - behaviour isn't specified afaik, just checking.\n");
    return rc;
}

