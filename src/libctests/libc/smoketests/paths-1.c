/* $Id: paths-1.c 2055 2005-06-19 08:04:00Z bird $ */
/** @file
 *
 * Some testing of path/dir/file related functions.
 *
 * Copyright (c) 2004 knut st. osmundsen <bird@innotek.de>
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



/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <limits.h>



int main(int argc, char **argv)
{
    int     rcRet = 0;
    int     rc;
    char   *psz;
    char    szPath[PATH_MAX];
    struct stat s1,s2;


    /* a bit of existing stuff */
    psz = realpath(argv[0], szPath);
    if (psz)
    {
        rc = lstat(szPath, &s1);
        if (rc)
            printf("lstat(%s) failed errno=%d\n", szPath, errno), rcRet++;
        rc = open(szPath, O_RDONLY);
        if (rc > 0)
            close(rc);
        else
            printf("open(%s) failed errno=%d\n", szPath, errno), rcRet++;
    }
    else
        printf("realpath(%s) failed errno=%d\n", argv[0], errno), rcRet++;

    rc = lstat(".", &s1);
    if (!rc)
    {
        rc = stat(".", &s2);
        if (!rc)
        {
           /* compare the two. */
        }
        else
            printf("stat(%s) failed errno=%d\n", ".", errno), rcRet++;
    }
    else
        printf("lstat(%s) failed errno=%d\n", ".", errno), rcRet++;


    /* test ENOENT */
    rc = lstat(strcpy(szPath, "/........./........./....../foo"), &s1);
    if (rc != -1 || errno != ENOENT)
        printf("stat(%s) failed rc=%d errno=%d - expected %d (ENOENT)\n", szPath, rc, errno, ENOENT), rcRet++;
    rc = open(szPath, O_RDONLY);
    if (rc != -1 || errno != ENOENT)
        printf("open(%s) failed rc=%d errno=%d - expected %d (ENOENT)\n", szPath, rc, errno, ENOENT), rcRet++;

    /* test ENOTDIR */
    if (!realpath(argv[0], szPath))
        strcpy(szPath, argv[0]);
    rc = lstat(strcat(szPath, "/parent-is-afile"), &s1);
    if (rc != -1 || errno != ENOTDIR)
        printf("stat(%s) failed rc=%d errno=%d - expected %d (ENOTDIR)\n", szPath, rc, errno, ENOTDIR), rcRet++;
    rc = open(szPath, O_RDONLY);
    if (rc != -1 || errno != ENOTDIR)
        printf("open(%s) failed rc=%d errno=%d - expected %d (ENOTDIR)\n", szPath, rc, errno, ENOTDIR), rcRet++;

    /*
     * Report result.
     */
    if (!rcRet)
        printf("paths: SUCCESS\n", rcRet);
    else
        printf("paths: FAILURE - %d errors\n", rcRet);
    return !!rcRet;
}
