/* $Id: $ */
/** @file
 *
 * Testcase open-1, tests O_EXCL.
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
     * Get a non-existing temp filename.
     */
    char szTmp[L_tmpnam];
    int fd;
    for (;;)
    {
        if (!tmpnam(szTmp))
            return !!printf("open-1: FAILURE - tmpnam failed!\n");
        fd = open(szTmp, O_RDWR | O_CREAT | O_EXCL, 0666);
        if (fd >= 0)
            break;

        int err = errno;
        struct stat st;
        if (    fd < 0
            &&  stat(szTmp, &st))
            return !!printf("open-1: FAILURE - open O_EXCL failed. file='%s' errno=%d %s\n",
                            szTmp, err, strerror(err));
        /* racing someone, retry. */
    }

    /*
     * Try open it again.
     */
    int fd2 = open(szTmp, O_RDWR | O_CREAT | O_EXCL, 0666);
    if (fd2 != -1)
    {
        cErrors++;
        printf("open-1: FAILURE - reopen with O_EXCL succeeded! fd2=%d\n", fd2);
        close(fd2);
    }
    else if (errno != EEXIST)
    {
        cErrors++;
        int err = errno;
        printf("open-1: FAILURE - errno=%d (%s) != %d (EEXIST)!\n", err, strerror(err), EEXIST);
    }

    close(fd);

    fd = open(szTmp, O_RDWR | O_CREAT | O_EXCL, 0666);
    if (fd != -1)
    {
        cErrors++;
        printf("open-1: FAILURE - open existing file with O_EXCL succeeded! fd2=%d\n", fd2);
        close(fd);
    }
    else if (errno != EEXIST)
    {
        cErrors++;
        int err = errno;
        printf("open-1: FAILURE - errno=%d (%s) != %d (EEXIST)!\n", err, strerror(err), EEXIST);
    }

    /*
     * Check that normal open works.
     */
    fd = open(szTmp, O_RDWR | O_CREAT, 0666);
    if (fd >= 0)
        close(fd);
    else
    {
        cErrors++;
        int err = errno;
        printf("open-1: FAILURE - open(%s, O_RDWR | O_CREAT, 0666) failed! errno=%d (%s)\n", szTmp, err, strerror(err));
    }

    /*
     * Cleanup.
     */
    if (unlink(szTmp))
    {
        cErrors++;
        int err = errno;
        printf("open-1: FAILURE - unlink(%s) failed! errno=%d (%s)\n", szTmp, err, strerror(err));
    }

    /*
     * Summary.
     */
    if (!cErrors)
        printf("open-1: SUCCESS\n");
    else
        printf("open-1: FAILURE - %d errors\n", cErrors);
    return !!cErrors;
}
