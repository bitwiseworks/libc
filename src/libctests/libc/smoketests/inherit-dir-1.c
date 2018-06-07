/* $Id: inherit-dir-1.c 2322 2005-09-25 10:38:15Z bird $ */
/** @file
 *
 * LIBC - directory handle inheritance testcase no.1.
 *
 * Copyright (c) 2005 knut st. osmundsen <bird@anduin.net>
 *
 *
 * This file is part of InnoTek LIBC.
 *
 * InnoTek LIBC is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * InnoTek LIBC is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with InnoTek LIBC; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */


/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/fcntl.h>
#include <process.h>


int main(int argc, char **argv)
{
    /*
     * Child for child.
     */
    if (argc == 3)
    {
        if (strcmp(argv[1], "child"))
            return 1;

        int fh = atoi(argv[2]);
        if (!fchdir(fh))
            return 0;
        printf("inherit-dir-1: spawn child: fchdir(%d) failed, errno=%d %s\n", fh, errno, strerror(errno));
        return 1;
    }

    /*
     * Negative tests.
     */
    int rc = fchdir(0);
    if (!rc || errno != ENOTDIR)
    {
        printf("inherit-dir-1: FAILURE - fchdir(0) -> %d errno=%d, expected -1 and %d\n", rc, errno, ENOTDIR);
        return 1;
    }
    rc = fchdir(-1);
    if (!rc || errno != EBADF)
    {
        printf("inherit-dir-1: FAILURE - fchdir(0) -> %d errno=%d, expected -1 and %d\n", rc, errno, EBADF);
        return 1;
    }

    /*
     * Open current directory.
     */
    int fh = open(".", O_RDONLY);
    if (fh < 0)
    {
        printf("inherit-dir-1: failed to open '.', errno=%d %s\n", errno, strerror(errno));
        return 1;
    }
    rc = fchdir(fh);
    if (rc)
    {
        printf("inherit-dir-1: FAILURE - fchdir(%d) -> %d errno=%d!\n", fh, rc, errno);
        return 1;
    }


    /*
     * Fork child.
     */
    pid_t pid = fork();
    if (!pid)
    {
        if (!fchdir(fh))
            return 0;
        printf("inherit-dir-1: fork child: fchdir(%d) failed, errno=%d %s\n", fh, errno, strerror(errno));
        return 1;
    }
    if (pid < 0)
    {
        printf("inherit-dir-1: FAILED - fork() -> errno=%d %s\n", errno, strerror(errno));
        return 1;
    }
    while (waitpid(pid, NULL, 0) != pid)
        /* nothing */;

    /*
     * Spawn child.
     */
    char sz[32];
    snprintf(sz, sizeof(sz), "%d", fh);
    errno = 0;
    rc = spawnlp(P_WAIT, argv[0], argv[0], "child", sz, NULL);
    if (!rc)
        printf("inherit-dir-1: SUCCESS\n", rc, errno);
    else
        printf("inherit-dir-1: FAILED - rc=%d errno=%d\n", rc, errno);

    return !!rc;
}
