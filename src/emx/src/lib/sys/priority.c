/* $Id: priority.c 1913 2005-04-25 05:18:54Z bird $ */
/** @file
 *
 * LIBC SYS Backend - Priority Conversion.
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
#include "libc-alias.h"
#include "priority.h"
#include <InnoTekLIBC/backend.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_BACK_PROCESS
#include <InnoTekLIBC/logstrict.h>

#define INCL_DOSPROCESS
#include <os2emx.h>

/** @page Priority Convertion Matrix
 *
 * @code
 *
 *  Unix            OS/2
 *  ----------------------------
 *  -20             TC 31
 *  -19             TC 15
 *  -18             TC  0
 *  -17             FG 31
 *  -16             FG 25
 *  -15             FG 20
 *  -14             FG 15
 *  -13             FG 10
 *  -12             FG  5
 *  -11             FG  1
 *  -10             FG  0
 *   -9             RG 31
 *   -8             RG 30
 *   -7             RG 25
 *   -6             RG 20
 *   -5             RG 15
 *   -4             RG 10
 *   -3             RG  7
 *   -2             RG  5
 *   -1             RG  2
 *    0             RG  0
 *    1             ID 31
 *    2             ID 30
 *    3             ID 29
 *    4             ID 28
 *    5             ID 27
 *    6             ID 26
 *    7             ID 25
 *    8             ID 24
 *    9             ID 23
 *   10             ID 22
 *   11             ID 20
 *   12             ID 17
 *   13             ID 15
 *   14             ID 12
 *   15             ID 10
 *   16             ID  7
 *   17             ID  5
 *   18             ID  2
 *   19             ID  1
 *   20             ID  0
 */


/** Convert from OS/2 to Unix. Indexed with class and level. */
static const signed char gToUnix[5][32] =
{
    /*  0,   1,   2,   3,   4,   5,   6,   7,   8,   9,  10,  11,  12,  13,  14,  15,  16,  17,  18,  19,  20,  21,  22,  23,  24,  25,  26,  27,  28,  29,  30,  31 */
    { 0 }, /* unused */
    /* PRTYC_IDLETIME */
    {  20,  19,  18,  18,  18,  17,  17,  16,  16,  16,  15,  15,  14,  14,  14,  13,  13,  12,  12,  12,  11,  11,  10,   9,   8,   7,   6,   5,   4,   3,   2,   1 },
    /* PRTYC_REGULAR */
    {   0,   0,  -1,  -1,  -1,  -2,  -2,  -3,  -3,  -3,  -4,  -4,  -4,  -4,  -4,  -5,  -5,  -5,  -5,  -5,  -6,  -6,  -6,  -6,  -6,  -7,  -7,  -7,  -7,  -7,  -8,  -9 },
    /* PRTYC_TIMECRITICAL */
    { -18, -18, -18, -18, -18, -18, -18, -18, -18, -18, -18, -18, -18, -18, -18, -19, -19, -19, -19, -19, -19, -19, -19, -19, -19, -19, -19, -19, -19, -19, -19, -20 },
    /* PRTYC_FOREGROUNDSERVER */
    { -10, -11, -11, -11, -11, -12, -12, -12, -12, -12, -13, -13, -13, -13, -13, -14, -14, -14, -14, -14, -15, -15, -15, -15, -15, -16, -16, -16, -16, -16, -16, -17 }
};

/**
 * Converts from OS/2 to Unix priority.
 * @returns Unix priority.
 * @param   iPrio       The OS/2 priority. (High byte is class, low byte is level.)
 */
int __libc_back_priorityUnixFromOS2(int iPrio)
{
    int iClass = iPrio >> 8;
    int iLevel = iPrio & 0xff;
    LIBC_ASSERTM(   iClass == PRTYC_TIMECRITICAL
                 || iClass == PRTYC_FOREGROUNDSERVER
                 || iClass == PRTYC_REGULAR
                 || iClass == PRTYC_IDLETIME, "iClass=%d (iPrio=%#x)\n", iClass, iPrio);
    LIBC_ASSERTM(iLevel >= 0 && iLevel < 32, "iLevel=%d (iPrio=%#x)\n", iLevel, iPrio);
    return gToUnix[iClass][iLevel];
}


/** Convert from Unix to OS/2. Indexed by level + 20. */
static const unsigned short gToOS2[41] =
{
    /* TC (-20 to -18) */
    0x031f, 0x30f, 0x300,
    /* FG (-17 to -10) */
    0x041f, 0x0419, 0x0414, 0x040f, 0x040a, 0x0405, 0x0401, 0x0400,
    /* RG (-9 to 0) */
    0x021f, 0x021e, 0x0219, 0x0214, 0x020f, 0x020a, 0x0207, 0x0205, 0x0202, 0x0200,
    /* ID (1 to 20) */                                                   /* vv10vv */
    0x011f, 0x011e, 0x011d, 0x011c, 0x011b, 0x011a, 0x0119, 0x0118, 0x0117, 0x0116, 0x0114, 0x0111, 0x010f, 0x010c, 0x010a, 0x0107, 0x0105, 0x0102, 0x0101, 0x0100
};


/**
 * Converts from Unix to OS/2 priority.
 * @returns OS/2 priority. (High byte is class, low byte is level.)
 * @param   iNice       The Unix priority.
 */
int __libc_back_priorityOS2FromUnix(int iNice)
{
    LIBC_ASSERTM(iNice + 20 >= 0 && iNice + 20 <= sizeof(gToOS2) / sizeof(gToOS2[0]), "iNice=%d\n", iNice);
    return gToOS2[iNice + 20];
}


#if 0
/* testing the tables */
#include <stdio.h>
static int iPrev = 20;
static int checkOS2(int iPrio)
{
    int iNice = __libc_back_priorityUnixFromOS2(iPrio);
    int iRev = __libc_back_priorityOS2FromUnix(iNice);
    if (    iRev > iPrio
        || (iRev & ~0xff) < (iPrio & ~0xff)
        || iPrev < iNice)
    {
        printf("Error: iPrio=%#x iNice=%d iPrev=%d iRev=%#x\n", iPrio, iNice, iPrev, iRev);
        return 1;
    }
    else
        printf("Info: iPrio=%#x iNice=%d\n", iPrio, iNice);
    iPrev = iNice;
    return 0;
}

int main()
{
    int i;
    int rc = 0;
    for (i = 0; i < 32; i++)
        rc += checkOS2(0x100 | i);
    for (i = 0; i < 32; i++)
        rc += checkOS2(0x200 | i);
    for (i = 0; i < 32; i++)
        rc += checkOS2(0x400 | i);
    for (i = 0; i < 32; i++)
        rc += checkOS2(0x300 | i);
    for (i = -20; i <= 20; i++)
    {
        int iPrio = __libc_back_priorityOS2FromUnix(i);
        int iRev = __libc_back_priorityUnixFromOS2(iPrio);
        if (iRev != i)
        {
            printf("Error: i=%d iPrio=%#x iRev=%d\n", i, iPrio, iRev);
            rc++;
        }
    }
    return rc;
}

#endif
