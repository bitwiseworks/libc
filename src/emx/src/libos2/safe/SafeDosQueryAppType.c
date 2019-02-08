/* $Id: SafeDosQueryAppType.c 3942 2014-12-26 19:15:42Z bird $ */
/** @file
 *
 * SafeDosQueryAppType()
 *
 * Copyright (c) 2007 knut st. osmundsen <bird-src-spam@anduin.net>
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with This program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#define INCL_BASE
#include <os2.h>
#include "safe.h"


ULONG APIENTRY SafeDosQueryAppType(PCSZ pszName, PULONG pulFlags);
ULONG APIENTRY SafeDosQueryAppType(PCSZ pszName, PULONG pulFlags)
{
    ULONG   rc;
    ULONG   ful1;
    PULONG  pful1 = NULL;
    SAFE_PCSZ(pszName);

    if (pulFlags)
    {
        ful1 = *pulFlags;
        pful1 = &ful1;
    }

    rc = DosQueryAppType(SAFE_PCSZ_USE(pszName), pful1);

    if (pulFlags)
        *pulFlags = ful1;

    SAFE_PCSZ_DONE(pszName);
    return rc;
}

