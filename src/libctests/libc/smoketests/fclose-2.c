/* $Id: $ */
/** @file
 *
 * fclose
 *
 * Copyright (c) 2006 knut st. osmundsen <bird@innotek.de>
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


#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/wait.h>


int main(int argc, char **argv)
{
    int rc = 0;
    int rc2;
    pid_t pid;
    if (argc == 2 && !strcmp(argv[1], "!child!"))
    {
        fprintf(stderr, "fclose-2: child %d\n", getpid());
        errno = 0;
        rc2 = fflush(stdout);
        if (rc2)
        {
            fprintf(stderr, "fclose-2: fflush failed! rc2=%d errno=%d %s\n", rc2, errno, strerror(errno));
            rc = 1;
        }
        errno = 0;
        rc2 = fclose(stdout);
        if (rc2 != -1 || errno != EBADF)
        {
            fprintf(stderr, "fclose-2: fclose #1 failed! rc2=%d errno=%d %s\n", rc2, errno, strerror(errno));
            rc = 1;
        }
        /* libc & glibc fails on all the following, freebsd doesn't */
        errno = 0;
        rc2 = fclose(stdout);
        if (rc2 != -1 || (errno && errno != EBADF))
        {
            fprintf(stderr, "fclose-2: fclose #2 failed! rc2=%d errno=%d %s\n", rc2, errno, strerror(errno));
            rc = 1;
        }
        errno = 0;
        if (dup2(STDERR_FILENO, STDOUT_FILENO) != STDOUT_FILENO)
        {
            fprintf(stderr, "fclose-2: dup2 failed! errno=%d %s\n", errno, strerror(errno));
            rc = 1;
        }
        errno = 0;
        rc2 = fclose(stdout);
        if (rc2 != -1 || (errno && errno != EBADF))
        {
            fprintf(stderr, "fclose-2: fclose #3 failed! rc2=%d errno=%d %s\n", rc2, errno, strerror(errno));
            rc = 1;
        }
        return rc;
    }

    pid = fork();
    if (!pid)
    {
        errno = 0;
        rc2 = fclose(stdout);
        if (rc2)
        {
            fprintf(stderr, "fclose-2: fclose(stdout) failed! rc2=%d errno=%d %s\n", rc2, errno, strerror(errno));
            return 1;
        }
        execl(argv[0], argv[0], "!child!", NULL);
        fprintf(stderr, "fclose-2: exec failed %d-%s\n", errno, strerror(errno));
        return rc;
    }

    if (pid > 0)
    {
        for (;;)
        {
            pid_t pidDone = waitpid(pid, &rc, 0);
            if (pidDone == pid)
            {
                rc = WIFEXITED(rc) ? WEXITSTATUS(rc) : rc;
                break;
            }
            if (pidDone < 0 && errno != EINTR)
            {
                fprintf(stderr, "fclose-2: faild waiting for %p, errno=%d (%s)\n", pid, errno, strerror(errno));
                rc = 1;
            }
        }
    }
    else
    {
        fprintf(stderr, "fclose-2: failed to fork child: %s\n", strerror(errno));
        rc = 1;
    }
    if (!rc)
        fprintf(stderr, "fclose-2: SUCCESS\n");
    else
        fprintf(stderr, "fclose-2: FAILURE - rc=%d\n", rc);
    return rc;
}

