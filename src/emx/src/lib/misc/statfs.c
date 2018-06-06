/* $Id: statfs.c 2254 2005-07-17 12:25:44Z bird $ */
/** @file
 *
 * statfs - BSD Interface.
 *
 * Copyright (c) 2004 knut st. osmundsen <bird-srcspam@anduin.net>
 *
 *
 * This file is part of InnoTek LIBC.
 *
 * InnoTek LIBC is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * InnoTek LIBC is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with InnoTek LIBC; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */


/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#include "libc-alias.h"
#include <errno.h>
#include <sys/types.h>
#include <sys/mount.h>
#include <InnoTekLIBC/backend.h>
#define __LIBC_LOG_GROUP    __LIBC_LOG_GRP_MISC
#include <InnoTekLIBC/logstrict.h>


/**
 * Get file system statistics
 *
 * @returns 0 on success.
 * @returns -1 and errno on failure.
 * @param   path    The path name of any file within the mounted file system.
 * @param   buf     Where to store the statistics.
 * @remark  The statfs() system call first appeared in 4.4BSD.
 */
int _STD(statfs)(const char *path, struct statfs *buf)
{
    LIBCLOG_ENTER("path=%p:{%s} buf=%p\n", (void *)path, path, (void *)buf);
    int rc = __libc_Back_fsStat(path, buf);
    if (!rc)
        LIBCLOG_RETURN_INT(0);
    errno = -rc;
    LIBCLOG_ERROR_RETURN_INT(-1);
}

