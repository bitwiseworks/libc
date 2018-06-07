/* $Id: lockf.c 689 2003-09-11 01:16:40Z bird $
 *
 * lockf implementation.
 *
 * Copyright (c) 2003 InnoTek Systemberatung GmbH
 * Author: knut st. osmundsen <bird-srcspam@anduin.net>
 *
 *
 * This file is part of Innotek LIBC.
 *
 * Innotek LIBC is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Innotek LIBC is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with Innotek LIBC; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include "libc-alias.h"
#include <sys/fcntl.h>
#include <unistd.h>
#include <io.h>
#include <errno.h>
#include <emx/io.h>

int _STD(lockf)(int fd, int request, off_t size)
{
    struct flock    flck;
    int             fcntl_request;

    /* init the struct with defaults */
    flck.l_len = size;
    flck.l_pid = 0;
    flck.l_start = 0;
    flck.l_whence = SEEK_CUR;
    fcntl_request = F_SETLK;

    /* set/do request specifics. */
    switch (request)
    {
        /* unlock locked section */
        case F_ULOCK:
            flck.l_type = F_UNLCK; break;
        /* lock a section for exclusive use */
        case F_LOCK:
            flck.l_type = F_WRLCK;
            fcntl_request = F_SETLKW;
            break;
        /* test and lock a section for exclusive use */
        case F_TLOCK:
            flck.l_type = F_WRLCK;
            break;
        /* test a section for locks by other procs */
        case F_TEST:
            flck.l_type = F_WRLCK;
            if (_fcntl(fd, F_GETLK, &flck) == -1)
                return -1;
            if (    flck.l_type == F_UNLCK
                ||  flck.l_pid == getpid())
                return 0;
            errno = EAGAIN;
            return -1;

        default:
            errno = EINVAL;
            return -1;
    }

    /* do work */
    return fcntl(fd, fcntl_request, &flck);
}

