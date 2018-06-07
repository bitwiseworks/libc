/* $Id: SafeDosCreatePipe.c 828 2003-10-10 23:38:11Z bird $ */
/** @file
 *
 * SafeDosCreatePipe()
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


ULONG APIENTRY SafeDosCreatePipe(PHFILE phfReadHandle, PHFILE phfWriteHandle, ULONG ulPipeSize);
ULONG APIENTRY SafeDosCreatePipe(PHFILE phfReadHandle, PHFILE phfWriteHandle, ULONG ulPipeSize)
{
    ULONG   rc;
    HFILE   h1, h2;
    PHFILE  ph1 = NULL;
    PHFILE  ph2 = NULL;

    if (phfReadHandle)
    {
        h1 = *phfReadHandle;
        ph1 = &h1;
    }
    if (phfWriteHandle)
    {
        h2 = *phfWriteHandle;
        ph2 = &h2;
    }

    rc = DosCreatePipe(ph1, ph2, ulPipeSize);

    if (phfReadHandle)
        *phfReadHandle = h1;
    if (phfWriteHandle)
        *phfWriteHandle = h2;

    return rc;
}

