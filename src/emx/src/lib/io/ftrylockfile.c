/* $Id: ftrylockfile.c 2090 2005-06-27 03:30:23Z bird $ */
/** @file
 *
 * LIBC - ftrylockfile()
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
 * Tries to locks a file stream.
 * Call funlockfile() to release the lock.
 *
 * @returns 0 if the lock was aquired.
 * @returns -1 if the lock was not aquired.
 * @param   pStream     The file stream to attempt locking.
 */
int _STD(ftrylockfile)(FILE *pStream)
{
    if (STREAM_LOCK_NOWAIT(pStream))
        return 0;
    return -1;
}

