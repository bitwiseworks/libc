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
#include <errno.h>


int main(int argc, char **argv)
{
    int rc = 0;
    int fd = -1;
    pid_t pid = 0;
    FILE *pFile = stderr;
    int   fChild = 0;

#define CHECK(expr) \
        do { int rc2 = expr; if (rc2) { fprintf(pFile, "fclose-1: %d: %s failed rc2=%d\n", __LINE__, #expr, rc2); rc = 1; } } while (0)
#define CHECKERRNO(expr, err) \
        do { int rc2; \
             errno = 0; \
             rc2 = expr; \
             if (rc2 != -1) { fprintf(pFile, "fclose-1: %d: %s didn't fail! rc2=%d\n", __LINE__, #expr, rc2); rc = 1; } \
             else if (errno != err) { fprintf(pFile, "fclose-1: %d: %s expected errno=%d got %d: %s\n", __LINE__, #expr, err, errno, strerror(errno)); rc = 1; } \
        } while (0)

    if (argc == 2 && !strcmp(argv[1], "!child!"))
    {
        fChild = 1;
        fprintf(pFile, "fclose-1: child %d\n", getpid());
    }
    else
    {
        fd = dup(STDERR_FILENO);
        if (fd <= 0)
        {
            fprintf(stdout, "fclose-1: FAILED to dup stderr.\n");
            return 1;
        }
        pFile = fdopen(fd, "w");
        if (!pFile)
        {
            fprintf(stdout, "fclose-1: FAILED to fdopen the duplicated stderr.\n");
            return 1;
        }
        CHECK(fflush(stdout));
        CHECK(fflush(stderr));

        CHECK(fclose(stdout));
        CHECKERRNO(fclose(stdout), 0 /* unspecified, follow glibc. */);

        pid = fork();
    }
    if (!pid)
    {
        CHECK(fflush(stdout));
        CHECK(fflush(stderr));
        CHECKERRNO(fclose(stdout), EBADF);
        CHECK(fflush(stdout));

        CHECKERRNO(fprintf(stdout, "fail0"), EBADF);
        if (fd >= 0)
            CHECK(dup2(fd, STDOUT_FILENO) != STDOUT_FILENO ? -1 : 0);
        CHECKERRNO(fprintf(stdout, "fail1"), EBADF);
        CHECK(fflush(stdout));
        /*CHECKERRNO(freopen(NULL, "w", stdout) ? 0 : -1, EBADF);*/

        if (!fChild)
        {
            CHECK(close(STDOUT_FILENO));
            execl(argv[0], argv[0], "!child!", NULL);
            CHECK(-1/*exec failed*/);
        }
        exit(rc);
    }
    else if (pid > 0)
    {
        for (;;)
        {
            pid_t pidDone = waitpid(pid, &rc, 0);
            if (pidDone == pid)
                break;
            if (pidDone < 0 && errno != EINTR)
            {
                fprintf(pFile, "fclose-1: faild waiting for %p, errno=%d (%s)\n", pid, errno, strerror(errno));
                rc = 1;
            }
        }
    }
    else
    {
        fprintf(pFile, "fclose-1: failed to fork child: %s\n", strerror(errno));
        rc = 1;
    }
    if (!rc)
        fprintf(pFile, "fclose-1: SUCCESS\n");
    else
        fprintf(pFile, "fclose-1: FAILURE - rc=%d\n", rc);
    return rc;
}

