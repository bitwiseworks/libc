/* $Id: 858-64bitmath.c 1413 2004-05-01 04:03:54Z bird $ */
/** @file
 *
 * Testcase for 64-bit math error.
 *
 * Copyright (c) 2004 knut st. osmundsen <bird-srcspam@anduin.net>
 *
 *
 * This file is part of InnoTek GCC.
 *
 * InnoTek GCC is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * InnoTek GCC is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with InnoTek GCC; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <stdio.h>
int main()
{
    unsigned long long x = 4481699234003649253LL;
    unsigned long long y = 448169923400364923LL;
    unsigned long long z = x / y;
    printf("858-64bitmath: %llu / %llu = %llu\n", x, y, z);
    if (z != 10)
    {
        printf("858-64bitmath: failed\n");
        return 1;
    }
    printf("858-64bitmath: succeeded\n");
    return 0;
}

