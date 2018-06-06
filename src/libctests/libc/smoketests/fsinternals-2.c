/* $Id: fsinternals-2.c 2055 2005-06-19 08:04:00Z bird $ */
/** @file
 *
 * LIBC SYS Backend - testcase for the Unix fsResolver.
 *
 * Copyright (c) 2005 knut st. osmundsen <bird@anduin.net>
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

/**
 * Commandline for simple standalone testing:
 * gcc -D STANDALONE_TEST -o fsResolver.exe -Zomf fsResolver.c ..\..\..\..\src\lib\sys\fs.c -I \Coding\gcc2\tree\obj\OS2\DEBUG\emx -I ..\..\..\..\src\include\ ..\..\..\..\src\lib\sys\seterrno.c
 */

/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#include "sys/b_fs.h"
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

int __libc_gfNoUnix = 0;

char g_aszCleanupDirs[PATH_MAX][5];


static int isValid(const char *pszDir)
{
    if (strchr(pszDir, '\\'))
    {
        printf("bad slashing in '%s'!\n", pszDir);
        return 0;
    }
    if (*pszDir == '/')
    {
        if (pszDir[1] != '/')
        {
            printf("Root slash in '%s'! - don't execute this test in a chroot env!\n", pszDir);
            return 0;
        }
        if (pszDir[2] == '/')
        {
            printf("Too many slashes for UNC in '%s'!\n", pszDir);
            return 0;
        }
    }
    else
    {
        if (pszDir[0] < 'A' || pszDir[0] > 'Z')
        {
            printf("Driver letter is not upper case in '%s'!\n", pszDir);
            return 0;
        }
        if (pszDir[1] != ':')
        {
            printf("Driver letter is not upper case in '%s'!\n", pszDir);
            return 0;
        }
        if (pszDir[2] != '/')
        {
            printf("No root slash in '%s'!\n", pszDir);
            return 0;
        }
    }

    int cch = strlen(pszDir);
    if (cch > 3 && pszDir[cch - 1] == '/')
    {
        printf("Trailing slash in '%s'!\n", pszDir);
        return 0;
    }

    return 1;
}



static int tstDir(const char *pszDir)
{
    char szPathCorrect[PATH_MAX] = {0};
    char szPath1[PATH_MAX];
    char szPath2[PATH_MAX];
    char szPath3[PATH_MAX];
    int rc;

    /*
     * Get the correct path using chdir.
     */
    getcwd(szPath1, sizeof(szPath1));
    if (_chdir2(pszDir))
    {
        printf("failed to chdir to %s, errno=%d\n", pszDir, errno);
        return 1;
    }
    getcwd(szPathCorrect, sizeof(szPathCorrect));
    memset(szPath2, 0xff, sizeof(szPath2)); szPath2[0] = '\0';
    rc = __libc_back_fsResolve(".", BACKFS_FLAGS_RESOLVE_FULL | BACKFS_FLAGS_RESOLVE_DIR, szPath2, NULL);
    if (_chdir2(szPath1))
    {
        printf("failed to chdir to %s, errno=%d\n", szPath1, errno);
        return 1;
    }
    if (!isValid(szPathCorrect))
    {
        printf("Invalid getcwd() return '%s'\n", szPathCorrect);
        return 1;
    }
    if (rc || strcmp(szPath2, szPathCorrect))
    {
        printf("Resolving '.' failed. got rc=%d '%s', expected '%s'\n", rc, szPath2, szPathCorrect);
        return 1;
    }

    /*
     * Check slash trailing and dots and such stuff.
     */
    memset(szPath2, 0xff, sizeof(szPath2)); szPath2[0] = '\0';
    strcat(strcpy(szPath1, pszDir), "\\");
    rc = __libc_back_fsResolve(szPath1, BACKFS_FLAGS_RESOLVE_FULL | BACKFS_FLAGS_RESOLVE_DIR, szPath2, NULL);
    if (rc || strcmp(szPath2, szPathCorrect))
    {
        printf("Resolving '%s' failed. got rc=%d '%s', expected '%s'\n", szPath1, rc, szPath2, szPathCorrect);
        return 1;
    }

    memset(szPath2, 0xff, sizeof(szPath2)); szPath2[0] = '\0';
    strcat(strcpy(szPath1, pszDir), "\\.\\./.\\\\./.\\.///.\\./.\\\\////");
    rc = __libc_back_fsResolve(szPath1, BACKFS_FLAGS_RESOLVE_FULL | BACKFS_FLAGS_RESOLVE_DIR, szPath2, NULL);
    if (rc || strcmp(szPath2, szPathCorrect))
    {
        printf("Resolving '%s' failed. got rc=%d '%s', expected '%s'\n", szPath1, rc, szPath2, szPathCorrect);
        return 1;
    }

    /*
     * Quick symlink check.
     */
    memset(szPath2, 0xff, sizeof(szPath2)); szPath2[0] = '\0';
    strcat(strcpy(szPath1, pszDir), "\\");
    rc = __libc_back_fsResolve(szPath1, BACKFS_FLAGS_RESOLVE_FULL_SYMLINK | BACKFS_FLAGS_RESOLVE_DIR, szPath2, NULL);
    if (rc || strcmp(szPath2, szPathCorrect))
    {
        printf("Resolving '%s' failed. got rc=%d '%s', expected '%s'\n", szPath1, rc, szPath2, szPathCorrect);
        return 1;
    }

    /*
     * Try non-existing filename/dir (as for create).
     */
    memset(szPath2, 0xff, sizeof(szPath2)); szPath2[0] = '\0';
    strcat(strcpy(szPath1, szPathCorrect), "/my-not-Existing-file---");
    rc = __libc_back_fsResolve(szPath1, BACKFS_FLAGS_RESOLVE_FULL, szPath2, NULL);
    if (rc != -ENOENT)
    {
        printf("(negative) Resolving '%s' failed!. Got rc=%d and '%s' expected rc=%d\n", szPath1, rc, szPath2, -ENOENT);
        return 1;
    }

    memset(szPath2, 0xff, sizeof(szPath2)); szPath2[0] = '\0';
    strcat(strcpy(szPath1, szPathCorrect), "/my-not-Existing-file---");
    rc = __libc_back_fsResolve(szPath1, BACKFS_FLAGS_RESOLVE_PARENT, szPath2, NULL);
    strcat(strcpy(szPath3, szPathCorrect), szPathCorrect[3] != '\0' ? "/my-not-Existing-file---" : "my-not-Existing-file---");
    if (rc || strcmp(szPath2, szPath3))
    {
        printf("Resolving '%s' failed. got rc=%d '%s', expected '%s'\n", szPath1, rc, szPath2, szPath3);
        return 1;
    }

    memset(szPath2, 0xff, sizeof(szPath2)); szPath2[0] = '\0';
    strcat(strcpy(szPath1, szPathCorrect), "/my-not-Existing-file---/");
    rc = __libc_back_fsResolve(szPath1, BACKFS_FLAGS_RESOLVE_FULL | BACKFS_FLAGS_RESOLVE_DIR, szPath2, NULL);
    strcat(strcpy(szPath3, szPathCorrect), szPathCorrect[3] != '\0' ? "/my-not-Existing-file---" : "my-not-Existing-file---");
    if (rc != -ENOENT)
    {
        printf("(negative) Resolving '%s' failed. got rc=%d '%s', expected rc=%d\n", szPath1, rc, szPath2, -ENOTDIR);
        return 1;
    }

    memset(szPath2, 0xff, sizeof(szPath2)); szPath2[0] = '\0';
    strcat(strcpy(szPath1, szPathCorrect), "/my-not-Existing-file---/");
    rc = __libc_back_fsResolve(szPath1, BACKFS_FLAGS_RESOLVE_PARENT | BACKFS_FLAGS_RESOLVE_DIR, szPath2, NULL);
    strcat(strcpy(szPath3, szPathCorrect), szPathCorrect[3] != '\0' ? "/my-not-Existing-file---" : "my-not-Existing-file---");
    if (rc || strcmp(szPath2, szPath3))
    {
        printf("Resolving '%s' failed. got rc=%d '%s', expected '%s'\n", szPath1, rc, szPath2, szPath3);
        return 1;
    }

    memset(szPath2, 0xff, sizeof(szPath2)); szPath2[0] = '\0';
    strcat(strcpy(szPath1, szPathCorrect), "/my-not-Existing-file---/");
    rc = __libc_back_fsResolve(szPath1, BACKFS_FLAGS_RESOLVE_FULL_MAYBE | BACKFS_FLAGS_RESOLVE_DIR, szPath2, NULL);
    strcat(strcpy(szPath3, szPathCorrect), szPathCorrect[3] != '\0' ? "/my-not-Existing-file---" : "my-not-Existing-file---");
    if (rc || strcmp(szPath2, szPath3))
    {
        printf("Resolving '%s' failed. got rc=%d '%s', expected '%s'\n", szPath1, rc, szPath2, szPath3);
        return 1;
    }

    memset(szPath2, 0xff, sizeof(szPath2)); szPath2[0] = '\0';
    strcat(strcpy(szPath1, szPathCorrect), "/my-not-Existing-file---/");
    rc = __libc_back_fsResolve(szPath1, BACKFS_FLAGS_RESOLVE_FULL_MAYBE | BACKFS_FLAGS_RESOLVE_DIR_MAYBE, szPath2, NULL);
    strcat(strcpy(szPath3, szPathCorrect), szPathCorrect[3] != '\0' ? "/my-not-Existing-file---" : "my-not-Existing-file---");
    if (rc || strcmp(szPath2, szPath3))
    {
        printf("Resolving '%s' failed. got rc=%d '%s', expected '%s'\n", szPath1, rc, szPath2, szPath3);
        return 1;
    }

    memset(szPath2, 0xff, sizeof(szPath2)); szPath2[0] = '\0';
    strcat(strcpy(szPath1, szPathCorrect), "/my-not-Existing-file---/");
    rc = __libc_back_fsResolve(szPath1, BACKFS_FLAGS_RESOLVE_FULL_SYMLINK_MAYBE | BACKFS_FLAGS_RESOLVE_DIR_MAYBE, szPath2, NULL);
    strcat(strcpy(szPath3, szPathCorrect), szPathCorrect[3] != '\0' ? "/my-not-Existing-file---" : "my-not-Existing-file---");
    if (rc || strcmp(szPath2, szPath3))
    {
        printf("Resolving '%s' failed. got rc=%d '%s', expected '%s'\n", szPath1, rc, szPath2, szPath3);
        return 1;
    }

    memset(szPath2, 0xff, sizeof(szPath2)); szPath2[0] = '\0';
    strcat(strcpy(szPath1, szPathCorrect), "/\\//my-not-Existing-file---");
    rc = __libc_back_fsResolve(szPath1, BACKFS_FLAGS_RESOLVE_FULL_MAYBE, szPath2, NULL);
    strcat(strcpy(szPath3, szPathCorrect), szPathCorrect[3] != '\0' ? "/my-not-Existing-file---" : "my-not-Existing-file---");
    if (rc || strcmp(szPath2, szPath3))
    {
        printf("Resolving '%s' failed. got rc=%d '%s', expected '%s'\n", szPath1, rc, szPath2, szPath3);
        return 1;
    }

    memset(szPath2, 0xff, sizeof(szPath2)); szPath2[0] = '\0';
    strcat(strcpy(szPath1, szPathCorrect), "/my-not-Existing-file---");
    rc = __libc_back_fsResolve(szPath1, BACKFS_FLAGS_RESOLVE_FULL_SYMLINK_MAYBE, szPath2, NULL);
    strcat(strcpy(szPath3, szPathCorrect), szPathCorrect[3] != '\0' ? "/my-not-Existing-file---" : "my-not-Existing-file---");
    if (rc || strcmp(szPath2, szPath3))
    {
        printf("Resolving '%s' failed. got rc=%d '%s', expected '%s'\n", szPath1, rc, szPath2, szPath3);
        return 1;
    }

    memset(szPath2, 0xff, sizeof(szPath2)); szPath2[0] = '\0';
    strcat(strcpy(szPath1, szPathCorrect), "//my-not-Existing-file---");
    rc = __libc_back_fsResolve(szPath1, BACKFS_FLAGS_RESOLVE_PARENT, szPath2, NULL);
    strcat(strcpy(szPath3, szPathCorrect), szPathCorrect[3] != '\0' ? "/my-not-Existing-file---" : "my-not-Existing-file---");
    if (rc || strcmp(szPath2, szPath3))
    {
        printf("Resolving '%s' failed. got rc=%d '%s', expected '%s'\n", szPath1, rc, szPath2, szPath3);
        return 1;
    }

#if 0
    /* negative */
    memset(szPath2, 0xff, sizeof(szPath2)); szPath2[0] = '\0';
    strcat(strcpy(szPath1, szPathCorrect), "/my-not-Existing-file---/");
    rc = __libc_back_fsResolve(szPath1, BACKFS_FLAGS_RESOLVE_PARENT, szPath2, NULL);
    if (rc != -ENOTDIR)
    {
        printf("(negative) Resolving '%s' failed. got rc=%d '%s', expected rc=%d\n", szPath1, rc, szPath2, -ENOTDIR);
        return 1;
    }
#endif

    /*
     * Create a directory and play with casing and 'maybe' expansion.
     */
    strcat(strcpy(szPath3, szPathCorrect), "/--Testase--fsResolver--TmpDir--");
    rc = mkdir(szPath3, 0777);
    if (rc)
    {
        printf("Failed to create subdir '%s', errno=%d\n", szPath3, errno);
        return 1;
    }
    strcpy(g_aszCleanupDirs[0], szPath3);

    /* mismatching casing. */
    memset(szPath2, 0xff, sizeof(szPath2)); szPath2[0] = '\0';
    strcat(strcpy(szPath1, szPathCorrect), "/--TESTASE--FSRESOLVER--TMPDIR--");
    rc = __libc_back_fsResolve(szPath1, BACKFS_FLAGS_RESOLVE_FULL | BACKFS_FLAGS_RESOLVE_DIR, szPath2, NULL);
    if (rc || strcmp(szPath2, szPath3))
    {
        printf("Resolving '%s' failed. got rc=%d '%s', expected '%s'\n", szPath1, rc, szPath2, szPath3);
        return 1;
    }

    memset(szPath2, 0xff, sizeof(szPath2)); szPath2[0] = '\0';
    strcat(strcpy(szPath1, szPathCorrect), "/--TESTASE--FSRESOLVER--TMPDIR--");
    rc = __libc_back_fsResolve(szPath1, BACKFS_FLAGS_RESOLVE_FULL_SYMLINK | BACKFS_FLAGS_RESOLVE_DIR, szPath2, NULL);
    if (rc || strcmp(szPath2, szPath3))
    {
        printf("Resolving '%s' failed. got rc=%d '%s', expected '%s'\n", szPath1, rc, szPath2, szPath3);
        return 1;
    }

    memset(szPath2, 0xff, sizeof(szPath2)); szPath2[0] = '\0';
    strcat(strcpy(szPath1, szPathCorrect), "/--TESTASE--FSRESOLVER--TMPDIR--");
    rc = __libc_back_fsResolve(szPath1, BACKFS_FLAGS_RESOLVE_FULL_MAYBE | BACKFS_FLAGS_RESOLVE_DIR, szPath2, NULL);
    if (rc || strcmp(szPath2, szPath3))
    {
        printf("Resolving '%s' failed. got rc=%d '%s', expected '%s'\n", szPath1, rc, szPath2, szPath3);
        return 1;
    }

    memset(szPath2, 0xff, sizeof(szPath2)); szPath2[0] = '\0';
    strcat(strcpy(szPath1, szPathCorrect), "/--TESTASE--FSRESOLVER--TMPDIR--");
    rc = __libc_back_fsResolve(szPath1, BACKFS_FLAGS_RESOLVE_FULL_SYMLINK_MAYBE | BACKFS_FLAGS_RESOLVE_DIR, szPath2, NULL);
    if (rc || strcmp(szPath2, szPath3))
    {
        printf("Resolving '%s' failed. got rc=%d '%s', expected '%s'\n", szPath1, rc, szPath2, szPath3);
        return 1;
    }

    /* dirslash */
    memset(szPath2, 0xff, sizeof(szPath2)); szPath2[0] = '\0';
    strcat(strcpy(szPath1, szPathCorrect), "/--TESTASE--FSRESOLVER--TMPDIR--/");
    rc = __libc_back_fsResolve(szPath1, BACKFS_FLAGS_RESOLVE_FULL_SYMLINK_MAYBE | BACKFS_FLAGS_RESOLVE_DIR, szPath2, NULL);
    if (rc || strcmp(szPath2, szPath3))
    {
        printf("Resolving '%s' failed. got rc=%d '%s', expected '%s'\n", szPath1, rc, szPath2, szPath3);
        return 1;
    }

    memset(szPath2, 0xff, sizeof(szPath2)); szPath2[0] = '\0';
    strcat(strcpy(szPath1, szPathCorrect), "/--TESTASE--FSRESOLVER--TMPDIR--/");
    rc = __libc_back_fsResolve(szPath1, BACKFS_FLAGS_RESOLVE_FULL_SYMLINK_MAYBE | BACKFS_FLAGS_RESOLVE_DIR_MAYBE, szPath2, NULL);
    if (rc || strcmp(szPath2, szPath3))
    {
        printf("Resolving '%s' failed. got rc=%d '%s', expected '%s'\n", szPath1, rc, szPath2, szPath3);
        return 1;
    }

#if 0
    /* negative */
    memset(szPath2, 0xff, sizeof(szPath2)); szPath2[0] = '\0';
    strcat(strcpy(szPath1, szPathCorrect), "/--TESTASE--FSRESOLVER--TMPDIR--/");
    rc = __libc_back_fsResolve(szPath1, BACKFS_FLAGS_RESOLVE_FULL_SYMLINK, szPath2, NULL);
    if (rc != -ENOTDIR)
    {
        printf("(negative) Resolving '%s' failed. got rc=%d '%s', expected rc=%d\n", szPath1, rc, szPath2, -ENOTDIR);
        return 1;
    }

    memset(szPath2, 0xff, sizeof(szPath2)); szPath2[0] = '\0';
    strcat(strcpy(szPath1, szPathCorrect), "/--TESTASE--FSRESOLVER--TMPDIR--");
    rc = __libc_back_fsResolve(szPath1, BACKFS_FLAGS_RESOLVE_FULL_SYMLINK, szPath2, NULL);
    if (rc != -ENOTDIR)
    {
        printf("(negative) Resolving '%s' failed. got rc=%d '%s', expected rc=%d\n", szPath1, rc, szPath2, -ENOTDIR);
        return 1;
    }
#endif

    /* more later */
    return 0;
}


static void cleanup(void)
{
    int i = sizeof(g_aszCleanupDirs) / sizeof(g_aszCleanupDirs[0]);
    while (i-- > 0)
        if (g_aszCleanupDirs[i])
            rmdir(g_aszCleanupDirs[i]);
}


int main(int argc, char **argv)
{
    int cErrors = 0;
    if (argc == 1)
    {
        cErrors += tstDir(".");
        cleanup();
    }
    else
    {
        int i;
        for (i = 1; i < argc; i++)
        {
            cErrors += tstDir(argv[i]);
            cleanup();
        }
    }

    if (!cErrors)
        printf("fsResolver: SUCCESS\n");
    else
        printf("fsResolver: FAILURE - %d errors\n", cErrors);

    return !!cErrors;
}
