/* $Id: $ */
/** @file
 *
 * Simple symlink testcase.
 *
 * Copyright (c) 2006 knut st. osmundsen <bird@anduin.net>
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
#include <io.h>
#include <sys/fcntl.h>
#include <sys/stat.h>
#include <errno.h>

int CompareINodes(int fd, const char *pszPath, const char *pszSymlink)
{
    struct stat FdStat;
    if (!fstat(fd, &FdStat))
    {
        struct stat PathStat;
        if (!stat(pszPath, &PathStat))
        {
            if (    FdStat.st_dev == PathStat.st_dev
                &&  FdStat.st_ino == PathStat.st_ino)
                return 0;
            printf("symlink-1: \"%s\" is not \"%s\" \n", pszPath, pszSymlink);
        }
        else
            printf("symlink-1: failed to stat \"%s\". errno=%d %s\n",
                   pszPath, errno, strerror(errno));
    }
    else
        printf("symlink-1: failed to fstat \"%s\" opened thru \"%s\". errno=%d %s\n",
               pszPath, pszSymlink, errno, strerror(errno));
    return 1;
}

int main(int argc, char **argv)
{
    int cErrors = 0;
    int fd;
    int rc;

    /*
     * Create a symlink to the executable and open it.
     */
    unlink("symlink-1-self");
    errno = 0;
    rc = symlink(argv[0], "symlink-1-self");
    if (rc)
    {
        printf("symlink-1: failed to create symlink \"symlink-1-self\", errno=%d %s\n", errno, strerror(errno));
        return 1;
    }

    errno = 0;
    fd = open("symlink-1-self", O_RDONLY);
    if (fd >= 0)
    {
        cErrors += CompareINodes(fd, argv[0], "symlink-1-self");
        close(fd);
    }
    else
        printf("symlink-1: failed to open ourself using symlink-1-self! errno=%d %s\n", errno, strerror(errno));

    /*
     * Link to the root of the disk.
     */
    unlink("symlink-1-root");
    errno = 0;
    rc = symlink("/", "symlink-1-root");
    if (!rc)
    {
        errno = 0;
        fd = open("symlink-1-root", O_RDONLY);
        if (fd >= 0)
        {
            cErrors += CompareINodes(fd, "/", "symlink-1-root");
            close(fd);
        }
        else
            printf("symlink-1: failed to open the root folder using symlink-1-root! errno=%d %s\n", errno, strerror(errno));
    }
    else
    {
        printf("symlink-1: failed to create symlink \"symlink-1-root\", errno=%d %s\n", errno, strerror(errno));
    }

    /*
     * Clean up.
     */
#if 1
    unlink("symlink-1-self");
    unlink("symlink-1-root");
#endif
    if (!cErrors)
        printf("symlink-1: SUCCESS\n");
    else
        printf("symlink-1: FAILURE - %d errors\n", cErrors);
    return !!cErrors;
}
