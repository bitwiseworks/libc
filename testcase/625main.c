/* $Id: 625main.c 1334 2004-04-05 19:45:58Z bird $
 *
 * TZ (tzset++) testcase.
 *
 * InnoTek Systemberatung GmbH confidential
 *
 * Copyright (c) 2003 InnoTek Systemberatung GmbH
 * Author: knut st. osmundsen <bird-srcspam@anduin.net>
 *
 * All Rights Reserved
 *
 */
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static struct times
{
    time_t   time;
}   aTimes[7] =
{
   0,                                       /* 1970-01-01 00:00:00 */
   13046400,                                /* 1970-06-01 00:00:00 */
   -(10*365+3-31-29)*24*3600 - 12*3600,     /* 1960-03-01 12:00:00 */
   -0x7fffffff + (365*24*60*60),            /* 1901-12-14 20:45:52 */
   0x7fffffff - (24*60*60),                 /* 2038-01-18 03:14:07 */
   31*365*24*3600 + 132,                    /* 2000-12-23 16:01:12 */
   (33*365 + 172)*24*3600 + 132,            /* 2000-12-23 16:01:12 */
};

static struct testcase
{
    const char *pszTZ;
    const char *pszTZRes;
    const char *apszRes[9];
}   aTests[] =
{
    {
        "TZ=PST8",
        "timezone=28800  daylight=0  tzname='PST','",
        {
            "1969-12-31 16:00:00  3  364  not-dst(0)",
            "1970-05-31 16:00:00  0  150  not-dst(0)",
            "1960-02-29 04:00:00  1   59  not-dst(0)",
            "1902-12-13 12:45:53  6  346  not-dst(0)",
            "2038-01-17 19:14:07  0   16  not-dst(0)",
            "2000-12-23 16:02:12  6  357  not-dst(0)",
            "2003-06-13 16:02:12  5  163  not-dst(0)",
        }
    },
    {
        "TZ=UCT0",
        "timezone=0      daylight=0  tzname='UCT','",
        {
            "1970-01-01 00:00:00  4    0  not-dst(0)",
            "1970-06-01 00:00:00  1  151  not-dst(0)",
            "1960-02-29 12:00:00  1   59  not-dst(0)",
            "1902-12-13 20:45:53  6  346  not-dst(0)",
            "2038-01-18 03:14:07  1   17  not-dst(0)",
            "2000-12-24 00:02:12  0  358  not-dst(0)",
            "2003-06-14 00:02:12  6  164  not-dst(0)",
        }
    },
    {
        "TZ=PST8PDT",
        "timezone=28800  daylight=1  tzname='PST','PDT'",
        {
            "1969-12-31 16:00:00  3  364  not-dst(0)",
            "1970-05-31 17:00:00  0  150  dst(1)",
            "1960-02-29 04:00:00  1   59  not-dst(0)",
            "1902-12-13 12:45:53  6  346  not-dst(0)",
            "2038-01-17 19:14:07  0   16  not-dst(0)",
            "2000-12-23 16:02:12  6  357  not-dst(0)",
            "2003-06-13 17:02:12  5  163  dst(1)",
        }
    },
    {
        "TZ=CET-1CDT",
        "timezone=-3600  daylight=1  tzname='CET','CDT'",
        {
            "1970-01-01 01:00:00  4    0  not-dst(0)",
            "1970-06-01 02:00:00  1  151  dst(1)",
            "1960-02-29 13:00:00  1   59  not-dst(0)",
            "1902-12-13 21:45:53  6  346  not-dst(0)",
            "2038-01-18 04:14:07  1   17  not-dst(0)",
            "2000-12-24 01:02:12  0  358  not-dst(0)",
            "2003-06-14 02:02:12  6  164  dst(1)",
        }
    },
    {
#ifndef UNIX_TZ
        "TZ=CET-1CEDT,3,-1,0,7200,10,-1,0,10800,3600",
#else
        /*"TZ=CET-1CEDT,M3.-1.0/2,M10.-1.0/3", */
        "TZ=CET-1CEDT,M3.4.0/2,M10.4.0/3",
#endif
        "timezone=-3600  daylight=1  tzname='CET','CEDT'",
        {
            "1970-01-01 01:00:00  4    0  not-dst(0)",
            "1970-06-01 02:00:00  1  151  dst(1)",
            "1960-02-29 13:00:00  1   59  not-dst(0)",
            "1902-12-13 21:45:53  6  346  not-dst(0)",
            "2038-01-18 04:14:07  1   17  not-dst(0)",
            "2000-12-24 01:02:12  0  358  not-dst(0)",
            "2003-06-14 02:02:12  6  164  dst(1)",
        }
    },
    {
#ifndef UNIX_TZ
        "TZ=AEST-10AEDT,10,-1,0,7200,3,-1,0,7200,3600",
#else
        /*"TZ=AEST-10AEDT,M10.-1.0/2,M3.-1.0/2", */
        "TZ=AEST-10AEDT,M10.4.0/2,M3.4.0/2",
#endif
        "timezone=-36000  daylight=1  tzname='AEST','AEDT'",
        {
            "1970-01-01 11:00:00  4    0  dst(1)",
            "1970-06-01 10:00:00  1  151  not-dst(0)",
            "1960-02-29 23:00:00  1   59  dst(1)",
            "1902-12-14 07:45:53  0  347  dst(1)",
            "2038-01-18 14:14:07  1   17  dst(1)",
            "2000-12-24 11:02:12  0  358  dst(1)",
            "2003-06-14 10:02:12  6  164  not-dst(0)",
        }
    },
    {
#ifndef UNIX_TZ
        "TZ=EST5EDT,4,1,0,7200,10,-1,0,7200,3600",
#else
        "TZ=EST5EDT,M4.1.0/2,M10.4.0/3",
#endif
        "timezone=18000  daylight=1  tzname='EST','EDT'",
        {
            "1969-12-31 19:00:00  3  364  not-dst(0)",
            "1970-05-31 20:00:00  0  150  dst(1)",
            "1960-02-29 07:00:00  1   59  not-dst(0)",
            "1902-12-13 15:45:53  6  346  not-dst(0)",
            "2038-01-17 22:14:07  0   16  not-dst(0)",
            "2000-12-23 19:02:12  6  357  not-dst(0)",
            "2003-06-13 20:02:12  5  163  dst(1)",
        }
    }

};

int doTests(void)
{
    int i;
    int rc;
    char sz[512];

    for (rc = i = 0; i < sizeof(aTests) / sizeof(aTests[0]); i++)
    {
        int j;
        putenv(aTests[i].pszTZ);
        tzset();
        sprintf(sz, "timezone=%-5d  daylight=%d  tzname='%s','%s'",
#ifndef FREEBSD
#ifdef __IBMC__
                _timezone,
#else
                timezone,
#endif
                daylight,
#else
                -1, -1,
#endif
                tzname[0],
                tzname[1]);
#ifndef FREEBSD
        if (!strncmp(sz, aTests[i].pszTZRes, strlen(aTests[i].pszTZRes)))
#endif
            printf("%s\n  %s - ok\n", aTests[i].pszTZ, sz);
#ifndef FREEBSD
        else
        {
            printf("%s\n  %s - mismatch!!! %s\n", aTests[i].pszTZ, sz, aTests[i].pszTZRes);
            rc++;
        }
#endif

        for (j = 0; j < sizeof(aTimes) / sizeof(aTimes[0]); j++)
        {
            struct tm  *pTm = localtime(&aTimes[j].time);
            sprintf(sz, "%04d-%02d-%02d %02d:%02d:%02d  %d  %3d  %s(%d)",
                   pTm->tm_year + 1900, pTm->tm_mon + 1, pTm->tm_mday,
                   pTm->tm_hour, pTm->tm_min, pTm->tm_sec, pTm->tm_wday, pTm->tm_yday,
                   pTm->tm_isdst ? "dst" : "not-dst", pTm->tm_isdst);
            if (!strcmp(sz, aTests[i].apszRes[j]))
                printf("%12d: %s - ok\n", aTimes[j].time, sz);
            else
            {
                printf("%12d: %s - mismatch!! %s\n", aTimes[j].time, sz, aTests[i].apszRes[j]);
                rc++;
            }
        }
    }
    return rc;
}

void printtz(void)
{
    const char *pszTZ = getenv("TZ");
    int         i;

    tzset();
    printf("TZ=%s\n    timezone=%-5d\t daylight=%d\ttzname='%s','%s'\n",
           pszTZ,
#ifndef FREEBSD
    #ifdef __IBMC__
           _timezone,
    #else
           timezone,
    #endif
           daylight,
#else
           -1, -1,
#endif
           tzname[0],
           tzname[1]);
    for (i = 0; i < sizeof(aTimes) / sizeof(aTimes[0]); i++)
    {
        struct tm  *pTm = localtime(&aTimes[i].time);
        printf("%12d: %04d-%02d-%02d %02d:%02d:%02d  %d  %3d  %s(%d)\n",
               (int)aTimes[i].time, pTm->tm_year + 1900, pTm->tm_mon + 1, pTm->tm_mday,
               pTm->tm_hour, pTm->tm_min, pTm->tm_sec, pTm->tm_wday, pTm->tm_yday,
               pTm->tm_isdst ? "dst" : "not-dst", pTm->tm_isdst);
    }
}

int main()
{
    int rcRet;

    printtz();
    putenv("TZ=");
    printtz();

    /* execute testcases. */
    rcRet = doTests();

    /* results */
    if (!rcRet)
        printf("Successfully executed return struct testcase (#625).\n");
    else
        printf("625main: %d failures.\n", rcRet);
    return rcRet;
}

