/* $Id: alloca-1.c 2055 2005-06-19 08:04:00Z bird $ */
/** @file
 *
 * alloca testcase.
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


/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/*******************************************************************************
*   Global Variables                                                           *
*******************************************************************************/
/** Array with sizes to alloca. */
static const size_t g_acb[] =
{
    0,
    1,
    2,
    3,
    4,
    5,
    6,
    7,
    8,
    0,
    9,
    10,
    11,
    12,
    13,
    14,
    15,
    16,
    17,
    18,
    19,
    20,
    21,
    22,
    23,
    24,
    25,
    26,
    27,
    28,
    29,
    31,
    32,
    33,
    1,
    127,
    128,
    0,
    129,
    130,
    131,
    132,
    133,
    256,
    2047,
    2048,
    2049,
    4090,
    4095,
    4096,
    4097,
    8192,
    16384,
    32763,
    32767,
    32768,
    32774
};


static int verify(void *pv, int ch, size_t cb)
{
    size_t cbLeft = cb;
    char *pch;
    for (pch = (char *)pv; cbLeft > 0; cbLeft--, pch++)
        if (*pch != ch)
        {
            printf("alloca: FAILURE - ch=%d *pch=%d cb=%d off=%d\n", ch, *pch, cb, pch - (char *)pv);
            return 1;
        }
    return 0;
}


int main()
{
    void   *apv[sizeof(g_acb) / sizeof(g_acb[0])] = {0};
    int     rcRet = 0;
    int     i;
    for (i = 0; i < sizeof(g_acb) / sizeof(g_acb[0]); i++)
    {
        apv[i] = alloca(g_acb[i]);
        memset(apv[i], i, g_acb[i]);
        verify(apv[i], i, g_acb[i]);
        if (i > 0)
            verify(apv[i - 1], i - 1, g_acb[i - 1]);
    }

    for (i = 0; i < sizeof(g_acb) / sizeof(g_acb[0]); i++)
        rcRet += verify(apv[i], i, g_acb[i]);

    if (!rcRet)
        printf("alloca: SUCCESS\n");
    else
        printf("alloca: FAILURE - %d errors\n", rcRet);

    return !!rcRet;
}
