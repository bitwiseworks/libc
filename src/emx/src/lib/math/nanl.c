/* $Id: $ */
/** @file
 *
 * kLIBC - nanl().
 *
 * Copyright (c) 2006 knut st. osmundsen <bird@innotek.de>
 *
 *
 * This file is part of kLIBC.
 *
 * kLIBC is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * kLIBC is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with kLIBC; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */


/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#include "libc-alias.h"
#include <math.h>
#include <string.h>
#include <stdlib.h>


/**
 * Return quiet NaN.
 * 
 * @returns a quiet NaN with the content indicated by pszTag (if available).
 * 
 * @param   pszTag      If NULL the returned value is strtold("NAN", NULL).
 *                      Other wise the returned value is strtold("NAN(pszTag)", NULL).
 */
long double _STD(nanl)(const char *pszTag)
{
    long double rc;
    if (!pszTag)
    {
        static int volatile         s_fCacheValid = 0;
        static long double volatile s_lrdCached;
        if (s_fCacheValid)
            rc = s_lrdCached;
        else
        {
            rc = strtold("NAN", NULL);
            s_lrdCached = rc;
            s_fCacheValid = 1;
        }
    }
    else
    {
        /* Create NAN(pszTag) and call strtod. */
        const size_t    cchTag = strlen(pszTag);
        char           *pszNan = alloca(sizeof("NAN()") + cchTag);
        memcpy(pszNan, "NAN(", sizeof("NAN(") - 1);
        memcpy(pszNan + sizeof("NAN(") - 1, pszTag, cchTag);
        memcpy(pszNan + sizeof("NAN(") - 1 + cchTag, ")", sizeof(")"));
        rc = strtold(pszNan, NULL);
    }

    return rc;
}

