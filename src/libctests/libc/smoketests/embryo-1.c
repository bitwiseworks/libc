/* $Id: embryo-1.c 2225 2005-07-06 04:34:23Z bird $ */
/** @file
 *
 * LIBC TESTCASE - Embryo reaping for non-libc children.
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


#include <process.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>


int main(int argc, char **argv)
{
    int i;
    if (argc != 2)
    {
        printf("syntax: %s <non-libc-prog>\n", argv[0]);
        return 1;
    }

    for (i = 0; i < 10000; i++)
    {
        int iState = 0;
        pid_t pid;
        if ((i % 100) == 0)
        {
            printf("embryo-1: i=%d\n", i);
            fflush(stdout);
        }
        pid = spawnv(P_NOWAIT, argv[1], &argv[1]);
        //printf("i=%d pid=%d\n", i, pid); fflush(stdout);
        if (pid < 0)
        {
            printf("embryo-1: i=%d spawn failed, errno=%d (%s)\n", i, errno, strerror(errno));
            return 1;
        }
        wait(&iState);
    }

    printf("embryo-1: done\n");
    //sleep(10);
    //printf("embryo-1: exitting\n");
    return 0;
}
