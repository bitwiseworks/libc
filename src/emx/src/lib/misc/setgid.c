/* $Id: $ */
/** @file
 *
 * LIBC - setgid().
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
#include <unistd.h>
#include <InnoTekLIBC/backend.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_PROCESS
#include <InnoTekLIBC/logstrict.h>

/**
 * Sets the real group id of the current process.
 * If there user is privilegde all three group ids will be set.
 *
 * Non privilegde users may only set it to the real group id or the saved group id.
 *
 * @returns 0 on success.
 * @returns -1 and errno on error.
 * @param   gid     The new real group id.
 *                  For superusers also the new effective and saved group id.
 */
int _STD(setgid)(gid_t gid)
{
    LIBCLOG_ENTER("gid=%d (%#x)\n", gid, gid);
    int rc = __libc_Back_processSetGid(gid);
    if (!rc)
        LIBCLOG_RETURN_INT(0);
    errno = -rc;
    LIBCLOG_ERROR_RETURN_INT(-1);
}

