/* $Id: timetabs_gen.c 903 2003-12-15 05:53:40Z bird $ */
/** @file
 *
 * Generate year table.
 *
 * Copyright (c) 2003 knut st. osmundsen <bird-srcspam@anduin.net>
 *
 *
 * This file is part of Innotek LIBC.
 *
 * Innotek LIBC is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Innotek LIBC is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with Innotek LIBC; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <stdio.h>
#include <emx/time.h>

int main()
{
    int i;
    int iDays;
    int aiDays[_YEARS];

    /*
     * Calc table.
     */
    for (i = 1970, iDays = 0; i >= 1900;)
    {
        aiDays[i - 1900] = iDays;
        /* prev */
        i--;
        iDays -= 365;
        if ((i % 4) == 0 && ((i % 100) || !(i % 1000)))
            iDays--;
    }
    for (i = 1970, iDays = 0; i < (_YEARS + 1900); i++)
    {
        aiDays[i - 1900] = iDays;
        /* next */
        iDays += 365;
        if ((i % 4) == 0 && ((i % 100) || !(i % 1000)))
            iDays++;
    }

    /*
     * Array
     */
    for (i = 1900; i < (_YEARS + 1900); i++)
    {
        iDays = aiDays[i - 1900];
        if (!(i % 10))
            printf("   %6d,", iDays);
        else
            printf("%6d,", iDays);
        if (!((i + 1) % 10))
            printf("  /* %d - %d */\n", i - 9, i);
    }

    return 0;
}
