/* $Id: QueryMemStat.c 1276 2004-02-20 22:08:31Z bird $ */
/** @file
 * DosQueryMemState() exploration.
 * Based on toolkit example (addendum.inf).
 */

/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#include <os2.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>


int main(int argc, char *argv[], char *envp[])
{
    APIRET rc=0;
    PVOID pvMem;
    ULONG onepage = 0x1000;

    if (argc < 3)
    {
        printf("Syntax   MEMSTATE <address> <cbMem>\n");
        return 0;
    }
    else
    {
        PVOID pvMem = (PVOID) strtoul(argv[1], NULL, 0);
        ULONG cbMem = strtoul(argv[2], NULL, 0);
        ULONG cPages = (cbMem+0x0fff) >> 12;

        printf(" address     state\n");
        while (cPages > 0)
        {
            ULONG   cbQuery = argc == 4 ? 0x1000 : cPages * 0x1000;
            ULONG   fulFlags = 0xffffffff;
            rc = DosQueryMemState(pvMem, &cbQuery, &fulFlags);
            if (rc)
                printf("*0x%08x DosQueryMemState returned %lu\n", pvMem, rc);
            else
            {
                ULONG i;
                for (i = 0; i < cbQuery; i += 0x1000)
                {
                    const char *psz1, *psz2;
                    switch (fulFlags & PAG_PRESMASK)
                    {
                        case PAG_NPOUT:         psz1 = "not present, not in-core,"; break;
                        case PAG_NPIN:          psz1 = "not present, in-core,";     break;
                        case PAG_PRESENT:       psz1 = "present, in-core,";         break;
                        default:                psz1 = "huh?"; break;
                    }
                    switch (fulFlags & PAG_TYPEMASK)
                    {
                        case PAG_INVALID:       psz2 = "invalid"; break;
                        case PAG_RESIDENT:      psz2 = "resident"; break;
                        case PAG_SWAPPABLE:     psz2 = "swappable"; break;
                        case PAG_DISCARDABLE:   psz2 = "discardable"; break;
                    }
                    printf("%s0x%08x 0x%08x %s %s\n", i ? " " : "*", pvMem + i, fulFlags, psz1, psz2);
                }
            }

            cPages -= cbQuery / 0x1000;
            pvMem = (PVOID)((ULONG)pvMem + 0x1000);
        }
    }

    return rc;
}




