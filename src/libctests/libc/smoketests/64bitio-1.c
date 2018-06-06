/* $Id: 64bitio-1.c 2055 2005-06-19 08:04:00Z bird $ */
/** @file
 *
 * 64-bit I/O testing (microscopical part of it).
 *
 *
 * Copyright (c) 2004 InnoTek Systemberatung GmbH
 * Author: knut st. osmundsen <bird-srcspam@anduin.net>
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
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <limits.h>
#include <io.h>

#define FILENAME "2gb.file"
#define SIZE     (0x8000ffffULL)

int main(int argc, char **argv)
{
    int         cErrors = 0;
    FILE       *pFile;
    struct stat s;
    int         rc;
    const char *pszFilename = FILENAME;
    if (argc > 2)
        pszFilename = argv[1];

    /*
     * Check for 64bit support.
     */
    if (sizeof(off_t) != sizeof(long long))
    {
        printf("64bitio: sizeof(off_t) != sizeof(long long) (%d != %d)\n",
               sizeof(off_t), sizeof(long long));
        return 1;
    }
    if (sizeof(off_t) == sizeof(long))
    {
        printf("64bitio: sizeof(off_t) == sizeof(long) (%d)\n",
               sizeof(long));
        return 1;
    }
    if (OFF_MAX <= LONG_MAX)
    {
        printf("64bitio: OFF_MAX <= LONG_MAX; LONG_MAX=%lld OFF_MAX=%lld\n",
               (long long)LONG_MAX, (long long)OFF_MAX);
        return 1;
    }
    #if OFF_MAX > LONG_MAX
    #else
    printf("64bitio: #if OFF_MAX > LONG_MAX failed\n");
    return 1;
    #endif
    #if LONG_MAX < OFF_MAX
    #else
    printf("64bitio: #if LONG_MAX < OFF_MAX failed\n");
    return 1;
    #endif

    /*
     * Handle based tests.
     */
    pFile = fopen(pszFilename, "w");
    if (!pFile)
    {
        printf("64bitio: couldn't open file\n");
        return 1;
    }

    rc = chsize(fileno(pFile), SIZE);
    if (!rc)
    {
        /* remember to delete file! */
        if (!fstat(fileno(pFile), &s))
        {
            if (s.st_size != SIZE)
            {
                printf("64bitio: fstat reports wrong filesize. %lld != %lld\n",
                       s.st_size, SIZE);
                cErrors++;
            }
        }
        else
        {
            printf("64bitio: fstat failed. errno=%d\n", errno);
            cErrors++;
        }

        fclose(pFile);

        /*
         * Tests on filename.
         */
        if (!stat(pszFilename, &s))
        {
            if (s.st_size != SIZE)
            {
                printf("64bitio: stat reports wrong filesize. %lld != %lld\n",
                       s.st_size, SIZE);
                cErrors++;
            }
        }
        else
        {
            printf("64bitio: stat failed. errno=%d\n", errno);
            cErrors++;
        }
    }
    else
    {
        printf("64bitio: failed to extend file to 2.01GB. running on JFS? errno=%d %s\n", errno, strerror(errno));
        cErrors++;
        fclose(pFile);
    }

    /*
     * Kill it.
     */
    if (unlink(pszFilename))
    {
        printf("64bitio: unlink failed! errno=%d\n", errno);
        cErrors++;
    }

    if (cErrors)
        printf("64bitio: %d errors\n", cErrors);
    else
        printf("64bitio: success\n", cErrors);
    return !!cErrors;
}

