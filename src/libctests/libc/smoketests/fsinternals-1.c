/* $Id: fsinternals-1.c 2055 2005-06-19 08:04:00Z bird $ */
/** @file
 *
 * LIBC - Path resolver testcase - highly internal.
 *
 * Copyright (c) 2005 knut st. osmundsen <bird@anduin.net>
 *
 *
 * This file is part of kBuild.
 *
 * kBuild is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * kBuild is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with kBuild; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include "sys/b_fs.h"

/**
 * Do one call, check and report the result.
 */
static int testit(const char *pszPath, unsigned fFlags, int rcCorrect)
{
    char    szNativePath[300];
    int     rc;

    memset(szNativePath, 0xAA, sizeof(szNativePath));
    szNativePath[sizeof(szNativePath) - 1] = '\0';
    errno = 0;
    rc = __libc_back_fsResolve(pszPath, fFlags, szNativePath, NULL);
    if (errno != 0)
        printf("FAILURE: flags=%#02x path='%s' -> rc=%d errno=%d szNativePath='%s' ERRNO CHANGED!\n", fFlags, pszPath, rc, errno, szNativePath);
    else if (!rcCorrect)
    {
        /*
         * Positive test.
         */
        if (!rc)
        {
            if (!strchr(szNativePath, 0xAA))
            {
                printf("SUCCESS: flags=%#02x path='%s' -> szNativePath='%s'\n", fFlags, pszPath, szNativePath);
                return 0;
            }
            else
                printf("FAILURE: flags=%#02x path='%s' -> rc=%d szNativePath='%s' PADDING!\n", fFlags, pszPath, rc, szNativePath);
        }
        else
            printf("FAILURE: flags=%#02x path='%s' -> rc=%d szNativePath='%s' (failed)\n", fFlags, pszPath, rc, szNativePath);
    }
    else
    {
        /*
         * Negative test.
         */
        if (rc == rcCorrect)
        {
            printf("SUCCESS: flags=%#02x path='%s' -> rc=%d (negative)\n", fFlags, pszPath, rc);
            return 0;
        }
        else
            printf("FAILURE: flags=%#02x path='%s' -> rc=%d not %d! (negative)\n", fFlags, pszPath, rc, rcCorrect);
    }
    return 1;
}


int main (int argc, char *argv[])
{
    int     rcRet = 0;
    struct stat s;

    rcRet += testit(".", BACKFS_FLAGS_RESOLVE_FULL, -ENOENT);
    rcRet += testit(".", BACKFS_FLAGS_RESOLVE_FULL | BACKFS_FLAGS_RESOLVE_DIR, 0);
    rcRet += testit(".", BACKFS_FLAGS_RESOLVE_FULL | BACKFS_FLAGS_RESOLVE_DIR_MAYBE, 0);
    rcRet += testit("/", BACKFS_FLAGS_RESOLVE_FULL, 0);
    if (!stat("/tmp", &s))
    {
        rcRet += testit("/tmp", BACKFS_FLAGS_RESOLVE_FULL, 0);
        rcRet += testit("/tmp/", BACKFS_FLAGS_RESOLVE_FULL, -ENOENT);
        rcRet += testit("/tmp/", BACKFS_FLAGS_RESOLVE_FULL | BACKFS_FLAGS_RESOLVE_DIR, 0);
        rcRet += testit("/tmp", BACKFS_FLAGS_RESOLVE_PARENT, 0);
        rcRet += testit("/tmp/", BACKFS_FLAGS_RESOLVE_PARENT, 0);
        rcRet += testit("/tmp/", BACKFS_FLAGS_RESOLVE_PARENT | BACKFS_FLAGS_RESOLVE_DIR, 0);
        rcRet += testit("/tmp/NoSuchSubDir", BACKFS_FLAGS_RESOLVE_PARENT, 0);
        rcRet += testit("/tmp/NoSuchSubDir/", BACKFS_FLAGS_RESOLVE_PARENT, -ENOENT);
        rcRet += testit("/tmp/NoSuchSubDir/", BACKFS_FLAGS_RESOLVE_PARENT | BACKFS_FLAGS_RESOLVE_DIR, 0);
    }
    else
        printf("skipping /tmp tests - no such directory\n");
    rcRet += testit("/nosuchdir", BACKFS_FLAGS_RESOLVE_PARENT, 0);
    rcRet += testit("/nosuchdir/nosuchsubdir", BACKFS_FLAGS_RESOLVE_PARENT, -ENOENT);

    return !!rcRet;
}

