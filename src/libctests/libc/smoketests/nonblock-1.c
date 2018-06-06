/* $Id: $ */
/** @file
 *
 * Testcase for non-blocking sockets.
 *
 * Copyright (c) 2005 knut st. osmundsen <bird@innotek.de>
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
#include <sys/socket.h>
#include <sys/fcntl.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>


int main(int argc, char **argv)
{
    char    abBuf[128];
    ssize_t cb;
    int pair[2];
    int rc;
    int fFlags;

    /*
     * Create a socket pair.
     */
    rc = socketpair(AF_LOCAL, SOCK_STREAM, 0, &pair[0]);
    if (rc < 0)
    {
        printf("nonblock-1: socketpair failed, rc=%d errno=%d: %s\n", rc, errno, strerror(errno));
        return 1;
    }

    /*
     * Make one end non-blocking.
     */
    rc = fcntl(pair[0], F_GETFL, 0);
    if (rc < 0)
    {
        printf("nonblock-1: F_GETFL failed, rc=%d errno=%d: %s\n", rc, errno, strerror(errno));
        return 1;
    }
    fFlags = rc;
    printf("nonblock-1: %d FL=%#x\n", pair[0], rc);

    rc = fcntl(pair[0], F_SETFL, fFlags | O_NONBLOCK);
    if (rc < 0)
    {
        printf("nonblock-1: F_SETFL(%#x) failed, rc=%d errno=%d: %s\n", fFlags | O_NONBLOCK, rc, errno, strerror(errno));
        return 1;
    }

    /*
     * Try read from the non-blocking end.
     */
    /** @todo this should be done with a timer pending or a parent watching. */
    cb = read(pair[0], abBuf, sizeof(abBuf));
    if (cb != -1 || errno != EWOULDBLOCK)
    {
        printf("nonblock-1: read didn't return -1 and errno=%d! cb=%zd errno=%d: %s\n", 
               cb, EWOULDBLOCK, errno, strerror(errno));
        return 1;
    }

    printf("nonblock-1: success\n");
    return 0;
}

