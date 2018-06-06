/* $Id: mknod.c 2254 2005-07-17 12:25:44Z bird $ */
/** @file
 *
 * mknod()
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
#include <sys/stat.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_IO
#include <InnoTekLIBC/logstrict.h>



/**
 * Create a filesystem node.
 *
 * @returns 0 on success.
 * @returns -1 and errno on failure.
 * @param   path        Path to the old
 * @param   mode        The filemode and type.
 * @param   dev         Device node id if applies.
 */
int	 _STD(mknod)(const char *path, mode_t mode, dev_t dev)
{
    LIBCLOG_ENTER("path=%p:{%s} mode=%x dev=%x\n", (void *)path, path, mode, dev);

    /*
     * This call can create
     */
    int rc;
    switch (S_IFMT & mode)
    {
        case S_IFDIR:
            rc = mkdir(path, mode);
            break;
        case S_IFIFO:
            rc = mkfifo(path, mode);
            break;
#if 0
        case S_IFSOCK:
            /* check if right prefix. */
            break;
#endif
        case S_IFCHR:
        case S_IFBLK:
        default:
            errno = ENOSYS;
            LIBCLOG_ERROR_RETURN(-1, "ret -1 - Mode not supported/implemented %x!\n", mode);

    }
    if (rc >= 0)
        LIBCLOG_RETURN_INT(rc);
    LIBCLOG_ERROR_RETURN_INT(rc);
}

