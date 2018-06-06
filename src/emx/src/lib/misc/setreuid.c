/* $Id: setreuid.c 2254 2005-07-17 12:25:44Z bird $ */
/** @file
 *
 * LIBC - setreuid().
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
 * Sets the real and the effective user ids of the current process.
 *
 * Non privilegde users may only set them to the real user id, the effective user id
 * or the saved user id.
 *
 * @returns 0 on success.
 * @returns -1 and errno on error.
 * @param   ruid    New real user id. Ignore if -1.
 * @param   euid    New effective user id. Ignore if -1.
 */
int _STD(setreuid)(uid_t ruid, uid_t euid)
{
    LIBCLOG_ENTER("ruid=%d (%#x) euid=%d (%#x)\n", ruid, ruid, euid, euid);
    int rc = __libc_Back_processSetUidAll(ruid, euid, -1);
    if (!rc)
        LIBCLOG_RETURN_INT(0);
    errno = -rc;
    LIBCLOG_ERROR_RETURN_INT(-1);
}

