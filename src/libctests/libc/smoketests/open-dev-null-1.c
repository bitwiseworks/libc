/* $Id: open-dev-null-1.c 2055 2005-06-19 08:04:00Z bird $ */
/** @file
 *
 * Simple open() testcase.
 *
 * Copyright (c) 2004 knut st. osmundsen <bird@anduin.net>
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


#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <sys/fcntl.h>

int main()
{
    int fh = open("/dev/null", O_RDWR);
    if (fh < 0)
    {
        printf("open-1: FAILURE - errno=%d\n", errno);
        return 1;
    }
    close(fh);

    fh = open("/dev/null", O_RDONLY);
    if (fh < 0)
    {
        printf("open-1: FAILURE - O_RD errno=%d\n", errno);
        return 1;
    }
    close(fh);

    fh = open("/dev/null", O_WRONLY);
    if (fh < 0)
    {
        printf("open-1: FAILURE - O_WR errno=%d\n", errno);
        return 1;
    }
    close(fh);

    printf("open-1: SUCCESS\n");
    return 0;
}

