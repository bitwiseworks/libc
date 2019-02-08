/* $Id: SafeDosOpenL.c 3942 2014-12-26 19:15:42Z bird $ */
/** @file
 *
 * SafeDosOpenL()
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

#define INCL_BASE
#include <os2.h>
#include "safe.h"

ULONG APIENTRY SafeDosOpenL(PCSZ pszFileName, PHFILE phFile, PULONG pulAction,
    LONGLONG llFileSize, ULONG ulAttribute, ULONG ulOpenFlags, ULONG ulOpenMode,
    PEAOP2 pEABuf);

ULONG APIENTRY SafeDosOpenL(PCSZ pszFileName, PHFILE phFile, PULONG pulAction,
    LONGLONG llFileSize, ULONG ulAttribute, ULONG ulOpenFlags, ULONG ulOpenMode,
    PEAOP2 pEABuf)
{
    /** @todo pEABuf */
    ULONG   rc, ul1;
    PULONG  pul1 = NULL;
    HFILE   hf1;
    PHFILE  phf1 = NULL;
    SAFE_PCSZ(pszFileName);

    if (phFile)
    {
        hf1 = *phFile;
        phf1 = &hf1;
    }
    if (pulAction)
    {
        ul1 = *pulAction;
        pul1 = &ul1;
    }

    rc = DosOpenL(SAFE_PCSZ_USE(pszFileName), phf1, pul1, llFileSize, ulAttribute,
                  ulOpenFlags, ulOpenMode, pEABuf);

    if (phFile)
        *phFile = hf1;
    if (pulAction)
        *pulAction = ul1;

    SAFE_PCSZ_DONE(pszFileName);
    return rc;
}

