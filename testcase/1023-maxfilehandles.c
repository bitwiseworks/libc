/** @file
 *
 * Testcase for bug #1023 autoincrement of file handles.
 *
 * InnoTek Systemberatung GmbH confidential
 *
 * Copyright (c) 2004 InnoTek Systemberatung GmbH
 * Author: knut st. osmundsen <bird-srcspam@anduin.net>
 *
 * All Rights Reserved
 *
 */

/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#define INCL_DOS
#define INCL_DOSINFOSEG
#ifdef __IBMC__
#include "../src/emx/include/os2emx.h"
#else
#include <os2.h>
#endif
#include <stdio.h>
#include <io.h>
#include <fcntl.h>
#include <string.h>



int main(int argc, const char **argv)
{
    int             i,j;
    LONG            lFHDelta = 0;
    ULONG           cFHs = 0;
    ULONG           cFHLast;
    unsigned        msLastInc;
    unsigned        msEnd;
    unsigned        msStart;
    unsigned        msTotal = 0;
    unsigned        cFiles = 0;
    unsigned        cIncrements = 0;
    int             fStreams;
    PGINFOSEG       pGIS = GETGINFOSEG();
#define MAX_FHS 10000
    static FILE    *apFiles[MAX_FHS];
    static int      aiFiles[MAX_FHS];

    /*
     * Init
     */
    fStreams = argc >= 2;

    DosSetRelMaxFH(&lFHDelta, &cFHs);
    printf("1023-maxfilehandles: starting at cFHs=%d; testing %s.\n", cFHs, fStreams ? "streams" : "io");

    /*
     * Open files.
     */
    cFHLast = cFHs;
    msLastInc = pGIS->msecs;
    for (i = 0; i < MAX_FHS; i++)
    {
        msStart = pGIS->msecs;
        if (fStreams)
            apFiles[i] = fopen(argv[argc - 1], "rb");
        else
            aiFiles[i] = open(argv[argc - 1], O_BINARY | O_RDONLY);
        msEnd = pGIS->msecs;
        if (fStreams ? apFiles[i] == NULL : aiFiles[i] < 0)
        {
            printf("error %d!\n", i + 1);
            break;
        }

        lFHDelta = cFHs = 0;
        DosSetRelMaxFH(&lFHDelta, &cFHs);
        lFHDelta = cFHs = 0;
        DosSetRelMaxFH(&lFHDelta, &cFHs);
        if (cFHs != cFHLast)
        {
            printf("Max FH change %i (fh=%d): %d -> %d (inc: %d ms  since last: %d ms)\n",
                   i + 1, fStreams ? fileno(apFiles[i]) : aiFiles[i],
                   cFHLast, cFHs, msEnd - msStart, msStart - msLastInc);
            /* stats */
            cIncrements++;
            msTotal += msEnd - msLastInc;
            msEnd = msLastInc = pGIS->msecs;
        }
        cFHLast = cFHs;
    }

    msTotal += msEnd - msLastInc;
    if (!msTotal)
        msTotal++;
    cFiles = i;

    lFHDelta = cFHs = 0;
    DosSetRelMaxFH(&lFHDelta, &cFHs);
    printf("cFHs=%d\n", cFHs);
    printf("msTotal=%d  cIncrements=%d  cFiles=%d  i.e. %d handles per second (%s)\n",
           msTotal, cIncrements, cFiles, cFiles * 1000 / msTotal, fStreams ? "streams" : "io");

    /*
     * Free the files.
     */
    msStart = pGIS->msecs;
    if (fStreams)
    {
        for (j = 0; j < i; j++)
            fclose(apFiles[j]);
    }
    else
    {
        for (j = 0; j < i; j++)
            close(aiFiles[j]);
    }
    msEnd = pGIS->msecs;
    msTotal = msEnd - msStart;
    if (!msTotal)
        msTotal++;
    printf("close of %d files took %d ms  i.e. %d handles per seconds\n",
           j, msTotal, cFiles * 1000 / msTotal);

    /*
     * Report the result.
     */
    if (i < 9900)
    {
        printf("1023-maxfilehandles: failed, could only open %d handles, expected > 9900\n", i);
        return 1;
    }

    printf("1023-maxfilehandles: succeeded opening %d handles.\n", i);
    return 0;
}
