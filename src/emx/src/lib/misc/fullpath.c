/* $Id: $ */
/** @file
 * EMX _fullpath().
 */

/*
 * Copyright (c) 2012 knut st. osmundsen <bird-kBuild-spamx@anduin.net>
 *
 * This file is part of kLibC.
 *
 * kLibC is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * kLibC is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with kLibC; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

/*******************************************************************************
* Header Files                                                                 *
*******************************************************************************/
#include "libc-alias.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <InnoTekLIBC/backend.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_IO
#include <InnoTekLIBC/logstrict.h>


/**
 * This is the PC version of realpath with an EMX twist.
 *
 * The EMX twist is the return value.  VisualAge for C++ and Visual C++ returns
 * a char pointer and allow dynamic buffer allocation.  EMX returns 0 or -1.
 *
 * @returns 0 on success, -1 on failure and errno is set.  ERANGE is used to
 *          indicate insufficient buffer space.
 * @param   pszDst            The destination buffer.
 * @param   pszSrc            The relative path.
 * @param   cbDst             The size of the destination buffer (including the
 *                            terminator character).
 */
int _fullpath(char *pszDst, const char *pszSrc, int cbDst)
{
    int rc = __libc_Back_fsPathResolve(pszSrc, pszDst, cbDst, __LIBC_BACKFS_FLAGS_RESOLVE_FULL_MAYBE);
    if (rc == 0)
    {
        while ((pszDst = strchr(pszDst, '/')) != NULL)
            *pszDst++ = '\\';
        return 0;
    }

    /* failed */
    errno = -rc;
    if (cbDst)
        *pszDst = '\0';
    return -1;
}

