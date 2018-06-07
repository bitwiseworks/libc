/* $Id: $ */
/** @file
 *
 * rename.
 *
 * Copyright (c) 2004 knut st. osmundsen <bird@anduin.net>
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
 * Renames a file or directory.
 *
 * @returns 0 on success.
 * @returns -1 and errno on failure.
 * @param   pszPathOld      Old file path.
 * @param   pszPathNew      New file path.
 *
 * @remark OS/2 doesn't preform the deletion of the pszPathNew atomically.
 */
int _STD(rename)(const char *pszPathOld, const char *pszPathNew)
{
    LIBCLOG_ENTER("pszPathOld=%p:{%s} pszPathNew=%p:{%s}\n", (void *)pszPathOld, pszPathOld, (void *)pszPathNew, pszPathNew);
    int rc = __libc_Back_fsRename(pszPathOld, pszPathNew);
    if (!rc)
        LIBCLOG_RETURN_INT(0);
    errno = -rc;
    LIBCLOG_ERROR_RETURN_INT(-1);
}

