/* $Id: lseek1.c 830 2003-10-12 14:00:12Z bird $ */
/** @file
 *
 * Testcase for lseek which might be subject to bugs in DosSetFilePtrL().
 *
 * Copyright (c) 2003 knut st. osmundsen <bird-srcspam@anduin.net>
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with This program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */


#include <unistd.h>
#include <sys/fcntl.h>
#include <errno.h>



int main(int argc, char **argv)
{
    int     rc = 0;
    int     fh;
    off_t   cb;
    off_t   off;


    fh = open(argv[0], O_RDONLY, 0755);
    if (fh < 0)
    {
        printf("error: Failed to open '%s', errno=%d\n", argv[0], errno);
        return 1;
    }

    cb = lseek(fh, 0, SEEK_END);
    if (cb < 0)
    {
        printf("error: Failed to seek to end of file errno=%d\n", errno);
        rc++;
    }

    cb = tell(fh);
    if (cb < 0)
    {
        printf("error: Failed to tell file position, errno=%d\n", errno);
        rc++;
    }

    off = lseek(fh, -cb / 2, SEEK_END);
    if (off < 0)
    {
        printf("error: Failed to seek to middle of file from the end, errno=%d\n", errno);
        rc++;
    }

    off = lseek(fh, cb / 4, SEEK_CUR);
    if (off < 0)
    {
        printf("error: Failed to seek a quarter ahead, errno=%d\n", errno);
        rc++;
    }

    off = lseek(fh, -cb / 3, SEEK_CUR);
    if (off < 0)
    {
        printf("error: Failed to seek a third backward, errno=%d\n", errno);
        rc++;
    }

    off = lseek(fh, cb / 3, SEEK_SET);
    if (off < 0)
    {
        printf("error: Failed to seek to a third into the file, errno=%d\n", errno);
        rc++;
    }

    close(fh);

    /* summary */
    if (rc)
        printf("testcase failed\n", errno);
    else
        printf("testcase succeeded\n", errno);
    return rc;
}
