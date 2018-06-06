/* $Id: tmpfile-1.c 2055 2005-06-19 08:04:00Z bird $ */
/** @file
 *
 * LIBC - tmpfile testcase.
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

int main()
{
    int cErrors = 0;
    for (int i = 0; i <= 11000; i++)
    {
        FILE *pFile = tmpfile();
        if (pFile)
        {
            fclose(pFile);
            if (!(i % 1000))
                printf("tmpfile-1: i=%d\n", i);
        }
        else
        {
            printf("tmpfile-1: failure i=%d\n", i);
            if (++cErrors > 10)
                break;
        }
    }
    return !!cErrors;
}

