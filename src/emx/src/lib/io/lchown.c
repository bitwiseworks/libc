/* $Id: lchown.c 3817 2014-02-19 01:40:34Z bird $ */
/** @file
 * lchown()
 */

/*
 * Copyright (c) 2004-2014 knut st. osmundsen <bird-srcspam@anduin.net>
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
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_IO
#include "libc-alias.h"
#include <unistd.h>
#include <errno.h>
#include <InnoTekLIBC/backend.h>
#include <InnoTekLIBC/logstrict.h>


/**
 * Change the owner and group of a symbolic link or file.
 *
 * @returns 0 on success.
 * @returns -1 and errno on failure.
 * @param   pszPath Path to the file to change owner/group of, symbolic link in
 *                  the final component is not followed.
 * @param   uid     The user id of the new owner, pass -1 to not change it.
 * @param   gid     The group id of the new group, pass -1 to not change it.
 */
int	 _STD(lchown)(const char *pszPath, uid_t uid, gid_t gid)
{
    LIBCLOG_ENTER("pszPath=%p:{%s} uid=%ld gid=%ld\n", (void *)pszPath, pszPath, (long)uid, (long)gid);
    int rc = __libc_Back_fsSymlinkOwnerSet(pszPath, uid, gid);
    if (!rc)
        LIBCLOG_RETURN_INT(0);
    errno = -rc;
    LIBCLOG_ERROR_RETURN_INT(-1);
}

