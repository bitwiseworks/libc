/* $Id: rmdir-1.c 3352 2007-05-07 02:53:18Z bird $ */
/** @file
 *
 * Testcase for rmdir bug #163.
 *
 * Copyright (c) 2007 knut st. osmundsen <bird-src-spam@anduin.net>
 *
 *
 * This file is part of kLIBC.
 *
 * kLIBC is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * kLIBC is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with kLIBC; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>
#include <sys/stat.h>
#include <string.h>


/* this testcase is a bit backwards... */
int main(int argc, char **argv)
{
    struct stat s;
    int rc = 0;
    char szBuf[PATH_MAX];

#define LINK_NAME                  "rmdir-1-broken-link"
#define DOES_NOT_EXIST             "rmdir-1-does-not-exist"
#define DIRECTORY_THAT_DOES_EXIST  "rmdir-1-directory-that-does-exist"

    /* cleanup previous runs and check preconditions. */
    unlink(LINK_NAME);
    rmdir(DIRECTORY_THAT_DOES_EXIST);
    if (!stat(DOES_NOT_EXIST, &s))
    {
        printf("rmdir-1: FAILURE - '%s' exists!\n", DOES_NOT_EXIST);
        return 1;
    }
    if (    stat(argv[0], &s)
        ||  !S_ISREG(s.st_mode)
        )
    {
        printf("rmdir-1: FAILURE - '%s' deosn't exist or isn't a regular file! (argv[0])\n", argv[0]);
        return 1;
    }

    /* 1st test */
    if (symlink(DOES_NOT_EXIST, LINK_NAME))
    {
        printf("rmdir-1: FAILURE - failed to create symlink '%s', errno=%d (%s)\n", LINK_NAME, errno, strerror(errno));
        return 1;
    }    
    errno = 0;
    if (rmdir(LINK_NAME) != -1)
    {
        printf("rmdir-1: FAILURE - rmdir succeeded unexpectedly.\n");
        rc++;
    }
    else if (errno != ENOTDIR)
    {
        printf("rmdir-1: FAILURE - expected ENOTDIR got %d (%s)\n", errno, strerror(errno));
        rc++;
    }
    else
        printf("rmdir-1: broken symlink test suceeded.\n");        
    unlink(LINK_NAME);
   
    /* 2nd test */ 
    errno = 0;
    if (rmdir(DOES_NOT_EXIST) != -1)
    {
        printf("rmdir-1: FAILURE - rmdir succeeded unexpectedly. (2)\n");
        rc++;
    }
    else if (errno != ENOENT)
    {
        printf("rmdir-1: FAILURE - expected ENOENT got %d (%s)\n", errno, strerror(errno));
        rc++;
    }
    else 
        printf("rmdir-1: non-existing directory test succeeded.\n");
        
    /* 3rd test */
    unlink(LINK_NAME);
    if (symlink(DIRECTORY_THAT_DOES_EXIST, LINK_NAME))
    {
        printf("rmdir-1: failed to create symlink '%s', errno=%d (%s).\n", LINK_NAME, errno, strerror(errno));
        return 1;
    }
    mkdir(DIRECTORY_THAT_DOES_EXIST, 0777);
    if (   stat(DIRECTORY_THAT_DOES_EXIST,&s)
        || !S_ISDIR(s.st_mode))
    {
        printf("rmdir-1: '%s' isn't a directory like we expected. giving up.\n", DIRECTORY_THAT_DOES_EXIST);
        return 1;
    }
    errno = 0;
    if (rmdir(LINK_NAME) != -1)
    {
         printf("rmdir-1: FAILURE - rmdir(symlink) shall fail but it didn't.\n");
         rc++;
    }
    else if (errno != ENOTDIR)
    {
         printf("rmdir-1: FAILURE - expected ENOTDIR but got errno=%d (%s) (3rd test)\n", errno, strerror(errno));
         rc++;
    }
    else
    {
         errno = 0;
         if (rmdir(DIRECTORY_THAT_DOES_EXIST) != 0)
         {
             printf("rmdir-1: FAILURE - failed to delete '%s', errno=%d (%s)\n", DIRECTORY_THAT_DOES_EXIST, errno, strerror(errno));
             rc++;
         }
         else
             printf("rmdir-1: successfully removed directory.\n");
    }
    rmdir(DIRECTORY_THAT_DOES_EXIST);
    unlink(LINK_NAME);    

    /* 4th test */
    if (rmdir(DIRECTORY_THAT_DOES_EXIST "/" DIRECTORY_THAT_DOES_EXIST) != -1)
    {
         printf("rmdir-1: FAILURE - rmdir('%s') shall fail but it didn't.\n", DIRECTORY_THAT_DOES_EXIST "/" DIRECTORY_THAT_DOES_EXIST);
         rc++;
    }
    else if (errno != ENOENT)
    {
         printf("rmdir-1: FAILURE - expected ENOENT but got errno=%d (%s) (4rd test)\n", errno, strerror(errno));
         rc++;
    }
    else
    {
        errno = 0;
        if (    rmdir(DIRECTORY_THAT_DOES_EXIST "/" DIRECTORY_THAT_DOES_EXIST "/" DIRECTORY_THAT_DOES_EXIST) != -1
            ||  errno != ENOENT)
        {
             printf("rmdir-1: FAILURE - rmdir('%s') succeeded or didn't fail with ENOENT. errno=%d (%s)\n", 
                    DIRECTORY_THAT_DOES_EXIST "/" DIRECTORY_THAT_DOES_EXIST "/" DIRECTORY_THAT_DOES_EXIST, 
                    errno, strerror(errno));
             rc++;
        }
        else
            printf("rmdir-1: removal of non-existing directory inside a non-existing directory failed like it should.\n");
    }

    /* 5th test */
    strcat(strcpy(szBuf, argv[0]), "/parent-is-a-file");
    if (rmdir(szBuf) != -1)
    {
         printf("rmdir-1: FAILURE - rmdir('%s') shall fail but it didn't.\n", szBuf);
         rc++;
    }
    else if (errno != ENOTDIR)
    {
         printf("rmdir-1: FAILURE - expected ENOTDIR but got errno=%d (%s) (5th test)\n", errno, strerror(errno));
         rc++;
    }
    else
        printf("rmdir-1: removal of directory with a file in the path failed like it should.\n");

    /* summary */
    if (!rc)
        printf("rmdir-1: SUCCESS\n");
    else
        printf("rmdir-1: FAILURE - %d errors\n", rc);
    
    return !!rc;
}

