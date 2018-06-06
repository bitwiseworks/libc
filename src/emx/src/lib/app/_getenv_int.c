/* $Id: _getenv_int.c 1983 2005-05-08 11:51:26Z bird $ */
/** @file
 *
 * LIBC - _getenv_int
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


/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#include "libc-alias.h"
#include <stdlib.h>


/**
 * Gets a long long value from an environment variable.
 *
 * @return 0 on success.
 * @return -1 on failure.
 *
 * @param   pszEnvVar       The name of the environment variable.
 * @param   pll             Where to store the result.
 * @remark  This is a LIBC extension.
 */
int _getenv_longlong(const char *pszEnvVar, long long *pll)
{
    const char *pszValue = getenv(pszEnvVar);
    if (pszValue)
    {
        char *pszType;
        long long ll = strtoll(pszValue, &pszType, 0);
        if (    pszType != pszValue
            &&  (!pszType[0] || !pszType[1]))
        {
            switch (pszType[0])
            {
                case 't': case 'T': ll *= 1024LL*1024LL*1024LL*1024LL; break;
                case 'g': case 'G': ll *= 1024*1024*1024; break;
                case 'm': case 'M': ll *= 1024*1024; break;
                case 'k': case 'K': ll *= 1024; break;
                case '\0': break;
                default:
                    return -1;
            }
            *pll = ll;
            return 0;
        }
    }
    return -1;
}


/**
 * Gets a long value from an environment variable.
 *
 * @return 0 on success.
 * @return -1 on failure.
 *
 * @param   pszEnvVar       The name of the environment variable.
 * @param   pl              Where to store the result.
 * @remark  This is a LIBC extension.
 */
int _getenv_long(const char *pszEnvVar, long *pl)
{
    long long ll;
    if (!_getenv_longlong(pszEnvVar, &ll))
    {
        *pl = (long)ll;
        return 0;
    }
    return -1;
}


/**
 * Gets an integer value from an environment variable.
 *
 * @return 0 on success.
 * @return -1 on failure.
 *
 * @param   pszEnvVar       The name of the environment variable.
 * @param   pi              Where to store the result.
 * @remark  This is a LIBC extension.
 */
int _getenv_int(const char *pszEnvVar, int *pi)
{
    long long ll;
    if (!_getenv_longlong(pszEnvVar, &ll))
    {
        *pi = (int)ll;
        return 0;
    }
    return -1;
}

