/* $Id: FIL_STANDARDL.c 889 2003-12-10 16:51:33Z bird $ */
/** @file
 *
 * Testcase for FIL_STANDARDL defect.
 *
 *
 * Example of how to get into trouble:
 *      FIL_STANDARDL.EXE 99 c:\os2\dll\*
 *
 * Where 99 specifies the number of files to request and c:\os2\dll\* is
 * the file pattern to search for.
 *
 *
 * InnoTek Systemberatung GmbH confidential
 *
 * Copyright (c) 2003 InnoTek Systemberatung GmbH
 * Author: knut st. osmundsen <bird-srcspam@anduin.net>
 *
 * All Rights Reserved
 *
 */



/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#define INCL_BASE
#include <os2.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>


/*******************************************************************************
*   Defined Constants And Macros                                               *
*******************************************************************************/
/** Size of the file find buffer. */
#define CBBUFFER    2048


int main(int argc, char **argv)
{
    static char     achBuffer[CBBUFFER];
    PFILEFINDBUF3L  pCur;
    int             rc;
    int             i;
    ULONG           cFilesArg;
    ULONG           cFiles;
    HDIR            hDir;

    /*
     * Validate and parse the arguments.
     */
    if (argc <= 2)
    {
        printf("syntax: %s <cfiles> <pattern> [pattern [..]]\n", argv[0]);
        return EXIT_FAILURE;
    }

    cFilesArg = atol(argv[1]);
    if (cFilesArg == 0)
    {
        printf("syntax error!\n");
        return EXIT_FAILURE;
    }


    /*
     * Do the work.
     */
    for (i = 2; i < argc; i++)
    {
        /*
         * Start file find...
         */
        cFiles = cFilesArg;
        hDir = HDIR_CREATE;
        rc = DosFindFirst(argv[i], &hDir, FILE_NORMAL, &achBuffer[0], CBBUFFER, &cFiles, FIL_STANDARDL);
        if (!rc)
        {
            do
            {
                /*
                 * Dump files.
                 */
                pCur = (PFILEFINDBUF3L)&achBuffer[0];
                while (cFiles-- > 0)
                {
                    printf("%03d %s\n", (int)pCur->cchName, pCur->achName);

                    /* next */
                    pCur = (PFILEFINDBUF3L)((char*)pCur + pCur->oNextEntryOffset);
                }

                /*
                 * Next chunk.
                 */
                cFiles = cFilesArg;
                rc = DosFindNext(hDir, &achBuffer[0], CBBUFFER, &cFiles);
            } while (!rc);
            DosFindClose(hDir);
        }
    }

    return EXIT_SUCCESS;
}

