/* $Id: link.c 2254 2005-07-17 12:25:44Z bird $ */
/** @file
 *
 * link()
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
#include <sys/syslimits.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_IO
#include <InnoTekLIBC/logstrict.h>



/**
 * Hardlinks a file.
 *
 * This is stub. It always fails.
 *
 * @returns 0 on success.
 * @returns -1 and errno on failure.
 * @param   oldpath     Path to the old (or current if you prefere) file.
 * @param   newpath     Path to the new file.
 */
int	 _STD(link)(const char *oldpath, const char *newpath)
{
    LIBCLOG_ENTER("oldpath=%p:{%s} newpath=%p:{%s}\n", (void *)oldpath, oldpath, (void *)newpath, newpath);

    /*
     * Validate input.
     */
    struct stat st;
    int rc = stat(oldpath, &st);
    if (rc)
        LIBCLOG_RETURN_INT(rc);

    rc = stat(newpath, &st);
    if (!rc)
    {
        errno = EEXIST;
        LIBCLOG_ERROR_RETURN_INT(-1);
    }
    if (errno != ENOENT)
        LIBCLOG_ERROR_RETURN_INT(-1);

    if (strlen(newpath) >= PATH_MAX)
    {
        errno = ENAMETOOLONG;
        LIBCLOG_ERROR_RETURN_INT(-1);
    }

    errno = ENOSYS;
    LIBCLOG_ERROR_RETURN_INT(-1);
}

