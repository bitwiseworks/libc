/* $Id: lstat.c 2579 2006-03-08 00:03:46Z bird $ *//* lstat.c (libc) -- Copyright (c) 2003 knut st. osmundsen */
/** @file
 *
 * lstat().
 *
 * Copyright (c) 2004 knut st. osmundsen <bird@innotek.de>
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
#include <sys/stat.h>
#include <emx/time.h>
#include <InnoTekLIBC/backend.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_MISC
#include <InnoTekLIBC/logstrict.h>


int _STD(lstat)(const char *name, struct stat *buffer)
{
    LIBCLOG_ENTER("name=%p:{%s} buffer=%p\n", (void *)name, name, (void *)buffer);
    int rc = __libc_Back_fsSymlinkStat(name, buffer);
    if (!rc)
    {
        if (!_tzset_flag)
            tzset();
        _loc2gmt(&buffer->st_atime, -1);
        _loc2gmt(&buffer->st_mtime, -1);
        _loc2gmt(&buffer->st_ctime, -1);
        _loc2gmt(&buffer->st_birthtime, -1);
        LIBCLOG_RETURN_INT(0);
    }
    errno = -rc;
    LIBCLOG_ERROR_RETURN_INT(-1);
}


