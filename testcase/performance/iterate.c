/* $Id: iterate.c 405 2003-07-17 01:19:17Z bird $
 *
 * Iterate a program for a number of seconds.
 *
 * InnoTek Systemberatung GmbH confidential
 *
 * Copyright (c) 2003 InnoTek Systemberatung GmbH
 * Author: knut st. osmundsen <bird-srcspam@anduin.net>
 *
 * All Rights Reserved
 *
 */

#define INCL_BASE
#include <os2.h>
#include <string.h>
#include <stdio.h>


static int syntax(const char *argv0)
{
    fprintf(stderr,
            "syntax: %s <seconds> <prog> [arg]\n"
            "\n"
            "Iterated execution of <prog> for <seconds> reporting the number of iterations.\n",
            argv0);
    return 8;
}


int main(int argc, char const * const *argv)
{
    APIRET      rc;
    unsigned    cSeconds;
    PPIB        ppib;
    PTIB        ptib;
    char *      psz;
    char *      pszEnd;
    char        szArgs[8192];
    ULONG       ulEnd;
    ULONG       ul;
    unsigned    cIterations;

    /* Enough arguments? */
    if (argc <= 2)
        return syntax(argv[0]);

    /* It's easier to use the OS/2 facilities. */
    DosGetInfoBlocks(&ptib, &ppib);
    psz = ppib->pib_pchcmd;

    /* Skip the prog name and traditinal blanks before first arg */
    psz += strlen(psz) + 1;
    while (*psz == ' ' || *psz == '\t')
        psz++;

    /* Seconds. */
    cSeconds = strtoul(psz, &pszEnd, 0);
    if (pszEnd <= psz || cSeconds <= 0)
    {
        fprintf(stderr, "error: would you be so good as give me a decent argument?\n");
        return 8;
    }

    /* Skip the spaces. */
    psz = pszEnd;
    while (*psz == ' ' || *psz == '\t')
        psz++;

    /* Find the program name to execute - first word. (forget quoting) */
    psz = strcpy(szArgs, psz);
    while (*psz != ' ' && *psz != '\0')
        psz++;
    *psz++ = '\0';


    /*
     * Now iterate!
     */
    printf("Iterating '%s' for %d seconds...\n", szArgs, cSeconds);
    fflush(stdout);

    DosQuerySysInfo(QSV_MS_COUNT, QSV_MS_COUNT, &ulEnd, sizeof(ulEnd));
    ul = ulEnd;
    ulEnd += 1000 * cSeconds;
    while (ul < ulEnd)
    {
        RESULTCODES res;
        rc = DosExecPgm(NULL, 0, EXEC_SYNC, szArgs, NULL, &res, szArgs);
        if (rc)
            break;
        cIterations++;
        /* check the time */
        DosQuerySysInfo(QSV_MS_COUNT, QSV_MS_COUNT, &ul, sizeof(ul));
    }
    if (rc)
        printf("error: DosExecPgm failed with rc=%d.\n", rc);
    printf("Executed %d iterations.\n", cIterations);

    return rc;
}

