/* $Id: getfsstat.c 2254 2005-07-17 12:25:44Z bird $ */
/** @file
 *
 * getfsstat - BSD Interface.
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
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_MISC
#include <InnoTekLIBC/logstrict.h>


/**
 * Get list of all mounted file systems.
 *
 * @returns Number of statfs structs.
 * @returns If buf is NULL the number of mounted file systems are returned.
 * @returns -1 and errno set on failure.
 *
 * @param   buf     Where to store the statfs buffers describing mounted file systems.
 * @param   bufsize The size of buf, in bytes.
 * @param   flags   Flags, have no meaning on OS/2. Valid flag is MNT_NOWAIT.
 *
 * @remark The getfsstat() system call first appeared in 4.4BSD.
 */
int	_STD(getfsstat)(struct statfs *buf, long bufsize, int flags)
{
    LIBCLOG_ENTER("buf=%p bufsize=%ld flags=%#x\n", (void *)buf, bufsize, flags);
    int rc = __libc_Back_fsStats(buf, bufsize / sizeof(struct statfs), flags);
    if (rc >= 0)
        LIBCLOG_RETURN_INT(rc);
    errno = -rc;
    LIBCLOG_ERROR_RETURN_INT(-1);
}

