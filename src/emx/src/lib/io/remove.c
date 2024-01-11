/* $Id: $ */
/** @file
 *
 * remove().
 *
 * Copyright (c) 2005 knut st. osmundsen <bird@anduin.net>
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
#include <stdio.h>
#include <errno.h>
#include <InnoTekLIBC/backend.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_IO
#include <InnoTekLIBC/logstrict.h>


/**
 * For directory works as rmdir(), for the rest, as unlink().
 *
 * @returns 0 on success.
 * @returns -1 and errno on failure.
 * @param   pszPath     File/dir/whatever to remove.
 */
int _STD(remove)(const char *pszPath)
{
    LIBCLOG_ENTER("pszFile=%p:{%s}\n", pszPath, pszPath);
    int rc = __libc_Back_fsUnlink(pszPath);
    if (!rc)
        LIBCLOG_RETURN_INT(0);
    if (rc == -EISDIR)
        rc = __libc_Back_fsDirRemove(pszPath);
    if (!rc)
        LIBCLOG_RETURN_INT(0);
    errno = -rc;
    LIBCLOG_ERROR_RETURN_INT(-1);
}
