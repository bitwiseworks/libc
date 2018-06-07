/* $Id: SafeDosSetDateTime.c 828 2003-10-10 23:38:11Z bird $ */
/** @file
 *
 * SafeDosSetDateTime().
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



ULONG APIENTRY SafeDosSetDateTime(__const__ DATETIME *pdt);
ULONG APIENTRY SafeDosSetDateTime(__const__ DATETIME *pdt)
{
    ULONG       rc;
    DATETIME    dt1;
    PDATETIME   pdt1 = NULL;

    if (pdt)
    {
        dt1 = *pdt;
        pdt1 = &dt1;
    }

    rc = DosSetDateTime(pdt1);

    return rc;
}

