/* $Id: $ */
/** @file
 *
 * Testcase open-2, tests O_CREAT + O_RDONLY.
 *
 * Copyright (c) 2006 knut st. osmundsen <bird@innotek.de>
 *
 *
 * This file is part of kLIBC.
 *
 * kLIBC is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * kLIBC is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with kLIBC; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */



#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/fcntl.h>
#include <sys/stat.h>


int main()
{
    int cErrors = 0;

    /*
     * Open a non-existing temp filename.
     */
    char szTmp[L_tmpnam];
    int fd;
    for (;;)
    {
        if (!tmpnam(szTmp))
            return !!printf("open-2: FAILURE - tmpnam failed!\n");
        fd = open(szTmp, O_RDONLY | O_CREAT | O_EXCL, 0666);
        if (fd >= 0)
            break;

        int err = errno;
        struct stat st;
        if (    fd < 0
            &&  stat(szTmp, &st))
            return !!printf("open-2: FAILURE - open O_EXCL failed. file='%s' errno=%d %s\n",
                            szTmp, err, strerror(err));
        /* racing someone, retry. */
    }

    /*
     * Check that it's actually a read-only handle.
     */
    if (write(fd, "this is read-only", sizeof("this is read-only") - 1) != -1)
    {
        cErrors++;
        printf("open-2: FAILURE - could write to read only file descriptor!\n");
    }
    else if (errno != EBADF) //???
    {
        cErrors++;
        int err = errno;
        printf("open-2: FAILURE - write(read-only-fd) didn't return EBADF but %d %s\n", err, strerror(err));
    }

    /*
     * Open it again, now with write access.
     */
    int fd2 = open(szTmp, O_RDWR | O_CREAT, 0666);
    if (fd2 == -1)
    {
        cErrors++;
        int err = errno;
        printf("open-2: FAILURE - open(\"%s\", O_RDWD | O_CREAT, 0666) failed: %d %s\n", 
               szTmp, err, strerror(err));
    }
    close(fd2);
    close(fd);

    /*
     * Cleanup.
     */
    if (unlink(szTmp))
    {
        cErrors++;
        int err = errno;
        printf("open-2: FAILURE - unlink(%s) failed! errno=%d (%s)\n", szTmp, err, strerror(err));
    }

    /*
     * Summary.
     */
    if (!cErrors)
        printf("open-2: SUCCESS\n");
    else
        printf("open-2: FAILURE - %d errors\n", cErrors);
    return !!cErrors;
}
