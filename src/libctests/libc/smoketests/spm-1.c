/* $Id: spm-1.c 2055 2005-06-19 08:04:00Z bird $ */
/** @file
 *
 * Test SPM allocation.
 *
 * Copyright (c) 2004 knut st. osmundsen <bird-srcspam@anduin.net>
 *
 *
 * This file is part of InnoTek LIBC.
 *
 * InnoTek LIBC is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * InnoTek LIBC is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with InnoTek LIBC; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */



#include <stdio.h>
#include <stdlib.h>
#include <InnoTekLIBC/sharedpm.h>


static int randmax(int c, int mask)
{
    int i;
    do i = rand() & mask; while (i > c);
    return i;
}

int main()
{
    static void    *apv[0x10000];
    int             i;
    int             c;
    int             cLeft;

    /* exhaust the shared heap with small blocks. */
    for (i = 0; i < sizeof(apv) / sizeof(apv[0]); i++)
    {
        size_t cb = 64 - i % 49;
        if (i % 19)
            cb = 256;
        apv[i] = __libc_spmAlloc(cb);
        if (!apv[i])
            break;
        memset(apv[i], 0xfa, cb);
    }

    __libc_SpmCheck(1, 0);

    /* cleanup */
    c = cLeft = i;
    while (cLeft > c / 4)
    {
        i = randmax(c, 0xffff);
        if (apv[i])
        {
            __libc_spmFree(apv[i]);
            apv[i] = NULL;
            cLeft--;
        }
    }
    //__libc_SpmCheck(1, 1);
    i = c;
    while (i-- > 0)
    {
        __libc_spmFree(apv[i]);
        apv[i] = NULL;
    }

    __libc_SpmCheck(1, 0);


    /* exhaust the shared heap with larger blocks. */
    for (i = 0; i < sizeof(apv) / sizeof(apv[0]); i++)
    {
        size_t cb = 1000 - (i * 7 % 511);
        if (i % 11)
            cb = 256;
        apv[i] = __libc_spmAlloc(cb);
        if (!apv[i])
            break;
        memset(apv[i], 0xfa, cb);
    }

    __libc_SpmCheck(1, 0);

    /* cleanup */
    while (i-- > 0)
    {
        __libc_spmFree(apv[i]);
        apv[i] = NULL;
    }

    __libc_SpmCheck(1, 0);

    return 0;
}
