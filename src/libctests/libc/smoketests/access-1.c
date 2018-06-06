/* $Id: access-1.c 2055 2005-06-19 08:04:00Z bird $ */
/** @file
 *
 * Testcase for access*().
 *
 * Copyright (c) 2004 knut st. osmundsen <bird-srcspam@anduin.net>
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


int main(int argc, char **argv)
{
    int cErrors = 0;
    char szDir[260];

    /*
     * Check access to the .exe file.
     */
    if (access(argv[0], R_OK))
    {
        printf("access: R_OK check on .exe failed\n");
        cErrors++;
    }
    if (access(argv[0], F_OK))
    {
        printf("access: F_OK check on .exe failed\n");
        cErrors++;
    }
    if (!access(argv[0], W_OK))
    {
        printf("access: W_OK check on .exe failed - expected ignoring\n");
        //cErrors++;
    }
    if (!access(argv[0], W_OK|R_OK))
    {
        printf("access: W_OK check on .exe failed - expected ignoring\n");
        //cErrors++;
    }

    /*
     * Check the current directory.
     */
    if (access(".", R_OK))
    {
        printf("access: R_OK check on . failed\n");
        cErrors++;
    }
    if (access(".", F_OK))
    {
        printf("access: F_OK check on . failed\n");
        cErrors++;
    }
    if (access(".", W_OK))
    {
        printf("access: W_OK check on . failed\n");
        cErrors++;
    }
    if (access(".", W_OK|R_OK))
    {
        printf("access: W_OK check on . failed\n");
        cErrors++;
    }

    /*
     * Check the current directory.
     */
    if (access("./", R_OK))
    {
        printf("access: R_OK check on ./ failed\n");
        cErrors++;
    }
    if (access("./", F_OK))
    {
        printf("access: F_OK check on ./ failed\n");
        cErrors++;
    }
    if (access("./", W_OK))
    {
        printf("access: W_OK check on ./ failed\n");
        cErrors++;
    }
    if (access("./", W_OK|R_OK))
    {
        printf("access: W_OK check on ./ failed\n");
        cErrors++;
    }


    /*
     * Check the current directory.
     */
    getcwd(&szDir[0], sizeof(szDir));
    if (access(&szDir[0], R_OK))
    {
        printf("access: R_OK check on . failed\n");
        cErrors++;
    }
    if (access(&szDir[0], F_OK))
    {
        printf("access: F_OK check on . failed\n");
        cErrors++;
    }
    if (access(&szDir[0], W_OK))
    {
        printf("access: W_OK check on . failed\n");
        cErrors++;
    }
    if (access(&szDir[0], W_OK|R_OK))
    {
        printf("access: W_OK check on . failed\n");
        cErrors++;
    }


    /*
     * Report results.
     */
    if (cErrors)
        printf("access: %d failures\n", cErrors);
    else
        printf("access: succeeded\n");
    return cErrors ? 1 : 0;
}
