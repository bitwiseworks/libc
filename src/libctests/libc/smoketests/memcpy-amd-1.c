/* $Id: memcpy-amd-1.c 2291 2005-08-20 22:57:05Z bird $ */
/** @file
 *
 * _memcpy_amd testcase.
 *
 * Copyright (c) 2005 knut st. osmundsen <bird@anduin.net>
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
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

char abSrc[0x40000];
char abDst[sizeof(abSrc) + 16384];

static void init(void)
{
    unsigned *pu = (unsigned *)abSrc;
    unsigned cb = sizeof(abSrc);
    while (cb > 0)
    {
        *pu = (cb | (cb >> 16)) ^ 0x12488421;
        pu++;
        cb -= sizeof(*pu);
    }
}

static int iszero(const void *pv, size_t cb)
{
    int rc = 0;
    char *pch = (char *)pv;
    switch ((uintptr_t)pch & 7) /* assumes 8byte aligned end! */
    {
        case 0: rc |= *pch++ == 0; cb--;
        case 1: rc |= *pch++ == 0; cb--;
        case 2: rc |= *pch++ == 0; cb--;
        case 3: rc |= *pch++ == 0; cb--;
        case 4: rc |= *pch++ == 0; cb--;
        case 5: rc |= *pch++ == 0; cb--;
        case 6: rc |= *pch++ == 0; cb--;
        case 7: rc |= *pch++ == 0; cb--;
            break;
    }
    if (!rc && cb > 0)
    {
        register unsigned *pu = (unsigned *)pch;
        register unsigned *puEnd = (unsigned *)(pch + cb);
        while (pu < puEnd)
        {
            if (*pu)
                return 0;
            pu++;
        }
    }
    return rc;
}

static void compare(size_t cb)
{
    if (    memcmp(abSrc, abDst, cb)
        ||  !iszero(abDst + cb, sizeof(abDst) - cb))
    {
        int cErrors = 0;
        size_t i;
        for (i = 0; i < cb && cErrors < 256; i++)
            if (abDst[i] != abSrc[i])
            {
                printf("%#x: %#x != %#x\n", i, abDst[i], abSrc[i]);
                cErrors++;
            }

        while (i < sizeof(abDst) && cErrors < 256)
        {
            if (abDst[i])
            {
                printf("%#x: %#x != 0 (zero area)\n", i, abDst[i]);
                cErrors++;
            }
            i++;
        }

        if (cErrors)
            exit(1);
    }
}

//#define _memcpy_amd memcpy

static void test(size_t cb, size_t off)
{
    _memcpy_amd(abDst, abSrc, off);
    _memcpy_amd(abDst + off, abSrc + off, cb);
    compare(cb + off);
    bzero(abDst, cb + off);
}

/**
 * Checks if the current CPU supports CPUID.
 *
 * @returns 1 if CPUID is supported, 0 if it doesn't.
 */
static inline int HasCpuId(void)
{
    int         fRet = 0;
    uint32_t    u1;
    uint32_t    u2;
    __asm__ ("pushf\n\t"
             "pop   %1\n\t"
             "mov   %1, %2\n\t"
             "xorl  $0x200000, %1\n\t"
             "push  %1\n\t"
             "popf\n\t"
             "pushf\n\t"
             "pop   %1\n\t"
             "cmpl  %1, %2\n\t"
             "setne %0\n\t"
             "push  %2\n\t"
             "popf\n\t"
             : "=m" (fRet), "=r" (u1), "=r" (u2));
    return fRet;
}

static int IsAmd(void)
{
    if (HasCpuId())
    {
        uint32_t eax, ebx, ecx, edx;
        __asm__ __volatile__ ("cpuid" : "=a" (eax), "=d" (edx), "=c" (ecx), "=b" (ebx) : "0" (0));
        if (edx == 0x69746e65 && ecx == 0x444d4163 && ebx == 0x68747541)
            return 1;
    }
    return 0;
}

int main()
{
    size_t i, j;
    if (!IsAmd())
    {
        printf("amd-memcpy-1: skipping, not right amd cpu\n");
        return 0;
    }
    init();

    printf("amd-memcpy-1: small\n");
    for (i = 0; i < 256; i++)
        for (j = 0; j <= 64; j++)
            test(i, j);

    printf("amd-memcpy-1: medium\n");
    for (i = 0; i < 256; i++)
        for (j = 0; j <= 64; j++)
            test(i + 4096-128, j);

    printf("amd-memcpy-1: medium large\n");
    for (i = 0; i < 256; i++)
        for (j = 0; j <= 64; j++)
            test(i + 16384-128, j);

    printf("amd-memcpy-1: large\n");
    for (i = 0; i < 256; i++)
        for (j = 0; j <= 64; j++)
            test(i + 0x20000-128, j);

    printf("amd-memcpy-1: various\n");
    test(sizeof(abSrc) / 32, 0);
    test(sizeof(abSrc) / 16, 0);
    test(sizeof(abSrc) / 8, 0);
    test(sizeof(abSrc) / 4, 0);
    test(sizeof(abSrc) / 2, 0);
    test(sizeof(abSrc), 0);
    test(sizeof(abSrc) / 19, 0);
    test(sizeof(abSrc) / 17, 0);
    test(sizeof(abSrc) / 7, 0);
    test(sizeof(abSrc) / 5, 0);
    test(sizeof(abSrc) / 3, 0);
    return 0;
}

