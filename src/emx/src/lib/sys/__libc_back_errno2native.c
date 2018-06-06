/* $Id: __libc_back_errno2native.c 3693 2011-03-15 23:24:03Z bird $ *//* $Id: __libc_back_errno2native.c 3693 2011-03-15 23:24:03Z bird $ */
/** @file
 *
 * kNIX - Error convertion from errno.h to OS/2.
 *
 * Copyright (c) 2011 knut st. osmundsen <bird-src-spam@anduin.net>
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
#define INCL_DOSERRORS
#include <os2emx.h>
#include <errno.h>
#include "syscalls.h"


/**
 * Converts an errno.h error code to a native one.
 *
 * @returns native error code.
 * @param   rc  Native error code.
 * @remarks Round trip with __libc_back_native2errno usually isn't lossless
 *          because errno is way more limited than the native error codes.
 */
int __libc_back_errno2native(int rc)
{
    switch (rc)
    {
        case 0:             return NO_ERROR;
        case ENOENT:        return ERROR_FILE_NOT_FOUND;
        case ENAMETOOLONG:  return ERROR_FILENAME_EXCED_RANGE;
        case EINVAL:        return ERROR_INVALID_PARAMETER;
        /** @todo convert more - lazy bird. */
        default:            return ERROR_GEN_FAILURE;
    }
}

