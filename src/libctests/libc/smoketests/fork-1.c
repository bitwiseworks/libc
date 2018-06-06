/* $Id: fork-1.c 2290 2005-08-20 22:32:05Z bird $ */
/** @file
 *
 * Massive forking.
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

/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/time.h>

/*******************************************************************************
*   Global Variables                                                           *
*******************************************************************************/
static pid_t s_apidChildren[75];
static unsigned s_cChildren = 0;
static int s_cErrors = 0;

/*char ach[1024*1024*8] = {0}; - test memcpy */

static void reap(void)
{
    /*printf("fork-1: info: reaping s_cChildren=%d...\n", s_cChildren);*/
    int iStatus = -1;
    pid_t pid;
    while ((pid = wait4(-1, &iStatus, WNOHANG, NULL)) > 0)
    {
        if (!WIFEXITED(iStatus) || WEXITSTATUS(iStatus))
        {
            printf("fork-1: child %d -> iStatus=%#x\n", pid, iStatus);
            s_cErrors++;
        }

        unsigned i = s_cChildren;
        while (i-- > 0)
        {
            if (s_apidChildren[i] == pid)
            {
                s_apidChildren[i] = s_apidChildren[--s_cChildren];
                break;
            }
        }

        if (i >= sizeof(s_apidChildren) / sizeof(s_apidChildren[0]))
        {
            printf("fork-1: error! pid=%d not found. (s_cChildren=%d)\n", pid, s_cChildren);
            s_cErrors++;
        }
    }
}

int main(int argc, char **argv)
{
    int i;
    hrtime_t StartTime = gethrtime();
    for (i = 0; i < 2000; i++)
    {
        int pid = fork();
        switch (pid)
        {
            case -1:
                printf("Error errno=%d %s\n", errno, strerror(errno));
                s_cErrors++;
                break;
            case 0:
                //printf("I'm child %#x i=%d.\n", getpid(), i);
                exit(0);
                return 0;

            default:
                s_apidChildren[s_cChildren++] = pid;
                break;
        }
        if (!(i % 100))
        {
            printf("fork-1: info: forked %d i=%d s_cChildren=%d\n", pid, i, s_cChildren);
            fflush(stdout); /* this is necessary if output is redirected, the child will inherit the buffer elsewise.. */
        }

        /*
         * Reap until there are free slots again.
         */
        if (s_cChildren >= 42 || (i % 519) > 488)
            reap();
        while (s_cChildren >= sizeof(s_apidChildren) / sizeof(s_apidChildren[0]))
        {
            usleep(1000);
            reap();
        }
    }
    hrtime_t Elapsed = gethrtime() - StartTime;
    printf("fork-1: Elapsed: %lld (%llx)\n", (long long)Elapsed, (long long)Elapsed);

    /*
     * Wait for the rest of the children.
     */
    int cLoops = 5*20;
    while (s_cChildren > 0 && cLoops-- > 0)
    {
        usleep(50000);
        reap();
    }
    for (i = 0; i < s_cChildren; i++)
    {
        printf("fork-1: error! pid=%d did not complete in time.\n", s_apidChildren[i]);
        s_cErrors++;
    }

    if (!s_cErrors)
        printf("fork-1: SUCCESS\n");
    else
        printf("fork-1: FAILURE - %d errors\n", s_cErrors);
    return !!s_cErrors;
}

