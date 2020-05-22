/* $Id: $ */
/** @file
 *
 * LIBC - utimes.
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
#include <sys/time.h>
#include <errno.h>
#include <InnoTekLIBC/backend.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_IO
#include <InnoTekLIBC/logstrict.h>


/**
 * Sets the access and modification times of a file.
 *
 * @returns 0 on success.
 * @returns -1 and errno on failure.
 *
 * @param   pszPath     The file which times to set.
 * @param   paTimes     Pointer to an array containing the access and modification times in that order.
 *                      If NULL current time is used.
 */
int _STD(utimes)(const char *pszPath, const struct timeval *paTimes)
{
    LIBCLOG_ENTER("pszPath=%p:{%s} paTimes=%p:{{.tv_sec=%d, .tv_usec=%ld}, {.tv_sec=%d, .tv_usec=%ld}}\n",
                  (void *)pszPath, pszPath, (void *)paTimes,
                  paTimes ? paTimes[0].tv_sec : ~0, paTimes ? paTimes[0].tv_usec : ~0,
                  paTimes ? paTimes[1].tv_sec : ~0, paTimes ? paTimes[1].tv_usec : ~0);
    int rc = __libc_Back_fsFileTimesSet(pszPath, paTimes);
    if (!rc)
        LIBCLOG_RETURN_INT(0);
    errno = -rc;
    LIBCLOG_ERROR_RETURN_INT(-1);
}

