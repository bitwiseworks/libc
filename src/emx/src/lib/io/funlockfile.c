/* $Id: funlockfile.c 2254 2005-07-17 12:25:44Z bird $ */
/** @file
 *
 * LIBC - funlockfile()
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
#include "libc-alias.h"
#include <sys/fmutex.h>
#include <emx/io.h>
#include <stdio.h>
#include <errno.h>


/**
 * Unlocks a file stream.
 * The stream must previously have been locked by calling flockfile().
 *
 * @param   pStream     The file stream to unlock.
 */
void _STD(funlockfile)(FILE *pStream)
{
    STREAM_UNLOCK(pStream);
}

