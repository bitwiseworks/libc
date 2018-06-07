/* $Id: fork.c 1454 2004-09-04 06:22:38Z bird $ */
/** @file
 *
 * Performance testcase for fork().
 *
 * Copyright (c) 2004 knut st. osmundsen <bird-srcspam@anduin.net>
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
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>


int main(int argc, char **argv)
{
    pid_t   pid;
    int     i;
    int     cTimes = 10;
    struct timeval  tvStart;
    struct timeval  tvEnd;
    struct timeval  tv;

    /* args */
    if (argc > 1)
        if (!(cTimes = atol(argv[1])))
            cTimes = 10;

    /* profiling */
    gettimeofday(&tvStart, 0);
    for (i = 0; i < cTimes; i++)
    {
        int rc;
        pid = fork();
        if (!pid)
            exit(0);
        else if (pid < 0)
        {
            printf("fork failed. errno=%d i=%d\n", errno, i);
            break;
        }
        waitpid(pid, &rc, 0);
    }
    gettimeofday(&tvEnd, 0);

    /* calc duration */
    tv = tvEnd;
    tv.tv_usec -= tvStart.tv_usec;
    if (tv.tv_usec < 0)
    {
        tv.tv_usec += 1000000;
        tv.tv_sec--;
    }
    tv.tv_sec -= tvStart.tv_sec;

    /* print result. */
    printf("%ld.%06ld - %d times.\n", tv.tv_sec, tv.tv_usec, i);

    return 0;
}
