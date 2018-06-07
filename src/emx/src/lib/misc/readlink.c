/* $Id: readlink.c 2254 2005-07-17 12:25:44Z bird $ */
/** @file
 *
 * readlink()
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
#include <string.h>
#include <unistd.h>
#include <InnoTekLIBC/backend.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_IO
#include <InnoTekLIBC/logstrict.h>


/**
 * Read value of a symbolic (soft) link
 *
 * This is a weird api which doesn't terminate the string
 * and will not warn nor fail the buffer is too small. Take care!
 *
 * @returns The number of bytes written to the buffer.
 * @returns -1 and errno on failure.
 * @param   path    Path to the symbolic link.
 * @param   buf     Where to store the link. The result is not '\0' terminated.
 * @param   bufsize Size the the buffer pointed to by buf.
 */
int	 _STD(readlink)(const char *path, char *buf, int bufsize)
{
    LIBCLOG_ENTER("path=%p:{%s} buf=%p bufsize=%d\n", (void *)path, path, (void *)buf, bufsize);
    int rc = __libc_Back_fsSymlinkRead(path, buf, bufsize);
    if (rc >= 0)
        LIBCLOG_RETURN_INT(rc);
    errno = -rc;
    LIBCLOG_ERROR_RETURN_INT(-1);
}

