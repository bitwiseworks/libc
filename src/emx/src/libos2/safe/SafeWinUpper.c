/* $Id: SafeWinUpper.c 3386 2007-06-10 11:56:39Z bird $ */
/** @file
 *
 * SafeWinUpper()
 *
 * Copyright (c) 2003 knut st. osmundsen <bird-srcspam@anduin.net>
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

#define INCL_PM
#include <os2.h>
#include "safe.h"

ULONG APIENTRY SafeWinUpper(HAB hab, ULONG idcp, ULONG idcc, PSZ psz);
ULONG APIENTRY SafeWinUpper(HAB hab, ULONG idcp, ULONG idcc, PSZ psz)
{
    ULONG rc;
    if (SAFE_IS_HIGH(psz))
    {
        size_t cch = strlen((char *)psz);
        char *pszTmp = _lmalloc(cch + 3);
        if (pszTmp)
        {
            memcpy(pszTmp, psz, cch + 1);
            pszTmp[cch + 1] = '\0';
            pszTmp[cch + 2] = '\0';
            rc = WinUpper(hab, idcp, idcc, (PSZ)pszTmp);
            if (rc > 0)
                memcpy(psz, pszTmp, rc <= cch ? rc + 1 : rc);
            free(pszTmp);
        }
        else
        {
            PSZ pszStart = psz;
            while (*psz)
            {
                PSZ pszNext = WinNextChar(hab, idcp, idcc, psz);
                if (pszNext - psz == 1)
                    *psz = WinUpperChar(hab, idcp, idcc, *psz);
                else if (pszNext - psz == 2)
                    *(PUSHORT)psz = WinUpperChar(hab, idcp, idcc, *(PUSHORT)psz); /* a wild guess. */
                else
                    break;
                psz = pszNext;
            }
            rc = psz - pszStart;
        }
    }
    else
        rc = WinUpper(hab, idcp, idcc, psz);
    return rc;
}

