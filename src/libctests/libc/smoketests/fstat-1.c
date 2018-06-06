/* $Id: fstat-1.c 2310 2005-08-28 05:56:42Z bird $ */
/** @file
 *
 * LIBC Testcase - fstat().
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


#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/fcntl.h>
#include <sys/socket.h>


static void display(struct stat *pStat, const char *pszFilename, int rc)
{
    printf("fstat-1: %s: rc=%d errno=%d st_mode=0%o st_ino=%#llx st_dev=%#lx st_rdev=%#lx st_size=%#llx\n",
           pszFilename, rc, errno,
           pStat->st_mode, (long long)pStat->st_ino, (long)pStat->st_dev, (long)pStat->st_rdev, (long long)pStat->st_size);
}

int main(int argc, char **argv)
{
    int cErrors = 0;
    int rc;
    struct stat s;
    int fh;

    /* /dev/null */
    const char *pszFilename = "/dev/null";
    fh = open(pszFilename, O_RDONLY);
    if (fh >= 0)
    {
        bzero(&s, sizeof(s));
        rc = fstat(fh, &s);
        display(&s, pszFilename, rc);
        if (!rc)
        {
            if (!S_ISCHR(s.st_mode))
            {
                printf("fstat-1: %s: not character device!\n", pszFilename);
                cErrors++;
            }
        }
        else
        {
            printf("fstat-1: %s: fstat failed, errno=%d!\n", pszFilename, errno);
            cErrors++;
        }
        close(fh);
        rc = fstat(fh, &s);
        if (rc != -1 || errno != EBADF)
        {
            printf("fstat-1: close bug, rc=%d errno=%d (%s)\n", rc, errno, strerror(errno));
            cErrors++;
        }
        errno = 0;
    }

    /* the executable */
    pszFilename = argv[0];
    fh = open(pszFilename, O_RDONLY);
    if (fh >= 0)
    {
        bzero(&s, sizeof(s));
        rc = fstat(fh, &s);
        display(&s, pszFilename, rc);
        if (!rc)
        {
            if (!S_ISREG(s.st_mode))
            {
                printf("fstat-1: %s: not a regular file!\n", pszFilename);
                cErrors++;
            }
        }
        else
        {
            printf("fstat-1: %s: fstat failed, errno=%d!\n", pszFilename, errno);
            cErrors++;
        }
        close(fh);
    }

    /* anonymouse pipe */
    pszFilename = "pipe";
    int fhs[2];
    rc = pipe(fhs);
    if (!rc)
    {
        bzero(&s, sizeof(s));
        rc = fstat(fhs[0], &s);
        display(&s, pszFilename, rc);
        if (!rc)
        {
            if (!S_ISFIFO(s.st_mode))
            {
                printf("fstat-1: %s: not socket!\n", pszFilename);
                cErrors++;
            }
        }
        else
        {
            printf("fstat-1: %s: fstat failed, errno=%d!\n", pszFilename, errno);
            cErrors++;
        }

        bzero(&s, sizeof(s));
        rc = fstat(fhs[1], &s);
        display(&s, pszFilename, rc);
        if (!rc)
        {
            if (!S_ISFIFO(s.st_mode))
            {
                printf("fstat-1: %s: not socket!\n", pszFilename);
                cErrors++;
            }
        }
        else
        {
            printf("fstat-1: %s: fstat failed, errno=%d!\n", pszFilename, errno);
            cErrors++;
        }
        close(fhs[0]);
        rc = fstat(fhs[0], &s);
        if (rc != -1 || errno != EBADF)
        {
            printf("fstat-1: close bug, rc=%d errno=%d (%s)\n", rc, errno, strerror(errno));
            cErrors++;
        }
        close(fhs[1]);
        rc = fstat(fhs[1], &s);
        if (rc != -1 || errno != EBADF)
        {
            printf("fstat-1: close bug, rc=%d errno=%d (%s)\n", rc, errno, strerror(errno));
            cErrors++;
        }
        errno = 0;
    }

    /* socket. */
    pszFilename = "socket";
    fh = socket(AF_INET, SOCK_STREAM, 0);
    if (fh >= 0)
    {
        bzero(&s, sizeof(s));
        rc = fstat(fh, &s);
        display(&s, pszFilename, rc);
        if (!rc)
        {
            if (!S_ISSOCK(s.st_mode))
            {
                printf("fstat-1: %s: not socket!\n", pszFilename);
                cErrors++;
            }
        }
        else
        {
            printf("fstat-1: %s: fstat failed, errno=%d (%s)!\n", pszFilename, errno, strerror(errno));
            cErrors++;
        }
        close(fh);
        rc = fstat(fh, &s);
        if (rc != -1 || errno != EBADF)
        {
            printf("fstat-1: close bug, rc=%d errno=%d (%s)\n", rc, errno, strerror(errno));
            cErrors++;
        }
    }

    /* current directory */
    pszFilename = ".";
    fh = open(pszFilename, O_RDONLY);
    if (fh >= 0)
    {
        bzero(&s, sizeof(s));
        rc = fstat(fh, &s);
        display(&s, pszFilename, rc);
        if (!rc)
        {
            if (!S_ISDIR(s.st_mode))
            {
                printf("fstat-1: %s: not character device!\n", pszFilename);
                cErrors++;
            }
        }
        else
        {
            printf("fstat-1: %s: fstat failed, errno=%d!\n", pszFilename, errno);
            cErrors++;
        }
        close(fh);
    }

    /*
     * Report result.
     */
    if (!cErrors)
        printf("fstat-1: SUCCESS\n");
    else
        printf("fstat-1: FAILURE - %d errors\n", cErrors);
    return !!cErrors;
}

