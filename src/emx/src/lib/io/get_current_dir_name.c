/* $Id: get_current_dir_name.c 2259 2005-07-17 13:43:12Z bird $ */
/** @file
 *
 * LIBC - get_current_dir_name, GLIB Extension.
 *
 * Copyright (c) 2005 knut st. osmundsen <bird@anduin.net>
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
#define _GNU_SOURCE
#include "libc-alias.h"
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#define __LIBC_LOG_GROUP    __LIBC_LOG_GRP_IO
#include <InnoTekLIBC/logstrict.h>

/**
 * Get the current directory or the value of the env.var. PWD if it's correct.
 *
 * @returns current directory, caller must free() the value.
 */
char * _STD(get_current_dir_name)(void)
{
    LIBCLOG_ENTER("\n");
    const char *psz = getenv("PWD");
    if (psz)
    {
        struct stat s1;
        struct stat s2;
        if (    !stat(psz, &s1)
            &&  !stat(".", &s2)
            &&  s1.st_dev == s2.st_dev
            &&  s1.st_ino == s2.st_ino)
        {
            char *pszRet = strdup(psz);
            LIBCLOG_RETURN_MSG(pszRet, "ret %p:{%s} (PWD)\n", (char *)pszRet, pszRet);
        }
        LIBCLOG_MSG("PWD='%s' is invalid\n", psz);
    }

    char *pszRet = getcwd(NULL, 0);
    LIBCLOG_RETURN_MSG(pszRet, "ret %p:{%s}\n", (char *)pszRet, pszRet);
}

