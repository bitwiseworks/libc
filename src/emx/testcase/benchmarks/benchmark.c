/* $Id: benchmark.c 801 2003-10-04 17:42:15Z bird $ */
/** @file
 *
 * Benchmark Driver Program
 *
 *
 * Copyright (c) 2001-2003 knut st. osmundsen <bird-srcspam@anduin.net>
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with This program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#ifdef __OS2__
#define INCL_BASE
#endif
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "PrfTiming.h"


/** Number of seconds to sample a testcase where we count iterations. */
#define SAMPLE_TIME     3.0


/**
 * Execute pszPgm using CRT methods for executing it.
 *
 * @returns Number of childs per minutte.
 * @returns -1 on failure.
 * @param   pszPgm  Program to execute.
 * @param   pszMsg  Message to display before doing the execution.
 */
int CrtExecIterate(const char * pszPgm, const char *pszMsg)
{
    long double rdCur;
    long double rdStart;
    long double rdEnd;
    unsigned    cChilds;
    int         pid;
#ifdef __NOTPC__
    int         status;
#endif

    /*
     * Message.
     */
    if (pszMsg)
        printf(pszMsg);

    /*
     * Main process.
     */
    cChilds = 0;                        /* child count */
    rdEnd = getHz() * SAMPLE_TIME;
    rdEnd += rdStart = rdCur = gettime();
    while (rdEnd > rdCur)
    {
#ifndef __NOTPC__
        pid = spawnl(P_WAIT, pszPgm, pszPgm, NULL); /* pid == 0 on success */
        if (pid < 0)
        {
            printf("spawnl failed for '%s'\n", pszPgm);
            return -1;
        }
#else
        pid = fork();
        if (pid == 0)
        {/* child code */
            execl(pszPgm, pszPgm, NULL);
            fprintf(stderr, "we should NEVER be here!!\n");
            return -1;
        }
        if (pid > 0)
            pid = wait(&status);
#endif
        cChilds++;
        rdCur = gettime();
    }

    /*
     * Result.
     */
    cChilds /= ((rdCur - rdStart) / getHz());
    if (pszMsg)
        printf("%d childs per second.\n", cChilds);

    return cChilds;
}

/**
 * Execute pszPgm using OS methods for executing it.
 *
 * @returns Number of childs per minutte.
 * @returns -1 on failure.
 * @param   pszPgm  Program to execute.
 * @param   pszMsg  Message to display before doing the execution.
 */
int OSExecIterate(const char * pszPgm, const char *pszMsg)
{
    long double rdCur;
    long double rdStart;
    long double rdEnd;
    unsigned    cChilds;
    int         rc;
    int         pid;
#ifdef __NOTPC__
    int         status;
#endif

    /*
     * Message.
     */
    if (pszMsg)
        printf(pszMsg);

    /*
     * Main process.
     */
    cChilds = 0;                        /* child count */
    rdEnd = getHz() * SAMPLE_TIME;
    rdEnd += rdStart = rdCur = gettime();
    while (rdEnd > rdCur)
    {
#ifdef __OS2__
        RESULTCODES res;
        rc = DosExecPgm(NULL, 0, EXEC_SYNC, "micro.exe\0", NULL, &res, "micro.exe");
        if (rc)
        {
            printf("failed! (rc=%d)\n", rc);
            return 1;
        }
#elif defined(__NOTPC__)
        pid = fork();
        if (pid == 0)
        {/* child code */
            execl(pszPgm, pszPgm, NULL);
            fprintf(stderr, "we should NEVER be here!!\n");
            return -1;
        }
        if (pid > 0)
            pid = wait(&status);
#else
#error "Not ported to this OS."
#endif
        cChilds++;
        rdCur = gettime();
    }

    /*
     * Result.
     */
    cChilds /= ((rdCur - rdStart) / getHz());
    if (pszMsg)
        printf("%d childs per second.\n", cChilds);

    return cChilds;
}



int main(int argc, char **argv)
{
    setvbuf(stdout, NULL, _IONBF, 0); /* no buffering. */

    /*
     * Output some general stuff about this box.
     */
    printf("LIBC Benchmark v0.0.0\n");
    printSystemInfo();

    /*
     * Do calibration.
     */
    OSExecIterate( "micro",                 "Calibration OS... ");
    CrtExecIterate("micro",                 "Calibration CRT... ");

    /*
     * Do startup time tests.
     */
    CrtExecIterate("startup1-static",       "Startup - bare, static:  \t");
    CrtExecIterate("startup1-dynamic",      "Startup - bare, dynamic: \t");

    return 0;
}
