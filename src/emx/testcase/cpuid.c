/* $Id: cpuid.c 2292 2005-08-21 03:17:31Z bird $ */
/** @file
 *
 * The start of a cpuid dumper.
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
#include <stdint.h>

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


#define BIT(n)  (1 << n)
#define CPUID(n) do {   __asm__ __volatile__ ("cpuid" : "=a" (eax), "=d" (edx), "=c" (ecx), "=b" (ebx) : "0" (n)); \
                        printf("%#010x: edx=%08x ecx=%08x eax=%08x ebx=%08x\n", (n), edx, ecx, eax, ebx); } while (0)
#define CPUIDSZ(n, psz) \
    do {   __asm__ __volatile__ ("cpuid" : "=a" (eax), "=d" (edx), "=c" (ecx), "=b" (ebx) : "0" (n)); \
            ((uint32_t *)(psz))[0] = eax; ((uint32_t *)(psz))[1] = ebx; \
            ((uint32_t *)(psz))[2] = ecx; ((uint32_t *)(psz))[3] = edx; } while (0)
int main()
{
    enum {  MANU_AMD, MANU_INTEL, MANU_OTHER }
                enmManufacturer = MANU_OTHER;
    unsigned    c;
    uint32_t    eax, edx, ecx, ebx;

    if (!HasCpuId())
    {
        printf("This processor doesn't support the CPUID instruction!\n");
        return 0;
    }

    CPUID(0);
    c = eax;
    printf("Name:           %.4s%.4s%.4s\n"
           "Supports:       0-%u\n", &ebx, &edx, &ecx, eax);
    if (edx == 0x69746e65 && ecx == 0x444d4163 && ebx == 0x68747541)
        enmManufacturer = MANU_AMD;
    else if (edx == 0x6c65746e && ecx == 0x49656e69 && ebx == 0x756e6547)
        enmManufacturer = MANU_INTEL;

    if (c >= 1)
    {
        CPUID(1);
        printf("Family:         %d\n"
               "Model:          %d\n"
               "Stepping:       %d\n", 
               (eax >> 8) & 7, (eax >> 4) & 7, eax & 7);
        printf("Features:      ");
        if (edx & BIT(0))   printf(" FPU");
        if (edx & BIT(1))   printf(" VME");
        if (edx & BIT(2))   printf(" DE");
        if (edx & BIT(3))   printf(" PSE");
        if (edx & BIT(4))   printf(" TSC");
        if (edx & BIT(5))   printf(" MSR");
        if (edx & BIT(6))   printf(" PAE");
        if (edx & BIT(7))   printf(" MCE");
        if (edx & BIT(8))   printf(" CX8");
        if (edx & BIT(9))   printf(" APIC");
        if (edx & BIT(10))  printf(" 10");
        if (edx & BIT(11))  printf(" SEP");
        if (edx & BIT(12))  printf(" MTRR");
        if (edx & BIT(13))  printf(" PGE");
        if (edx & BIT(14))  printf(" MCA");
        if (edx & BIT(15))  printf(" CMOV");
        if (edx & BIT(16))  printf(" PAT");
        if (edx & BIT(17))  printf(" PSE36");
        if (edx & BIT(18))  printf(" PSN");
        if (edx & BIT(19))  printf(" CLFSH");
        if (edx & BIT(20))  printf(" 20");
        if (edx & BIT(21))  printf(" DS");
        if (edx & BIT(22))  printf(" ACPI");
        if (edx & BIT(23))  printf(" MMX");
        if (edx & BIT(24))  printf(" FXSR");
        if (edx & BIT(25))  printf(" SSE");
        if (edx & BIT(26))  printf(" SSE2");
        if (edx & BIT(27))  printf(" SS");
        if (edx & BIT(28))  printf(" 28");
        if (edx & BIT(29))  printf(" 29");
        if (edx & BIT(30))  printf(" 30");
        if (edx & BIT(31))  printf(" 31");
        printf("\n");
    }

    //CPUID(2);

    /*
     * AMD
     */
    if (enmManufacturer == MANU_AMD)
    {
        CPUID(0x80000000);
        c = eax;
        printf("AMD Name:       %.4s%.4s%.4s\n"
               "AMD Supports:   0x80000000-%#010x\n", &ebx, &edx, &ecx, eax);

        if (c >= 0x80000001)
        {
            CPUID(0x80000001);
            printf("AMD Family:     %d\n"
                   "AMD Model:      %d\n"
                   "AMD Stepping:   %d\n", 
                   (eax >> 8) & 7, (eax >> 4) & 7, eax & 7);
        }

        char szString[4*4*3+1] = {0};
        if (c >= 0x80000002)
            CPUIDSZ(0x80000002, &szString[0]);
        if (c >= 0x80000003)
            CPUIDSZ(0x80000003, &szString[16]);
        if (c >= 0x80000004)
            CPUIDSZ(0x80000004, &szString[32]);
        if (c >= 0x80000002)
            printf("AMD Model:      %s\n", szString);

        if (c >= 0x80000005)
        {
            CPUID(0x80000005);
        }
        if (c >= 0x80000006)
        {
            CPUID(0x80000006);
        }
        if (c >= 0x80000007)
        {
            CPUID(0x80000007);
        }
        if (c >= 0x80000008)
        {
            CPUID(0x80000008);
        }
    }

    return 0;
}
