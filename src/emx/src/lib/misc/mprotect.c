/* $Id: mprotect.c 2254 2005-07-17 12:25:44Z bird $ */
/** @file
 *
 * LIBC mprotect().
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
#include <sys/mman.h>
#include <sys/param.h>
#include <InnoTekLIBC/backend.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_MMAN
#include <InnoTekLIBC/logstrict.h>



/**
 * Changes the memory protection attributes of a page range.
 *
 * @returns 0 on success.
 * @returns -1 and errno on failure.
 * @param   pv      Start of the page range.
 * @param   cb      Size of the page range.
 * @param   fFlags  PROT_NONE or any combination of PROT_READ, PROT_WRITE and PROT_EXEC.
 */
int	_STD(mprotect)(const void *pv, size_t cb, int fFlags)
{
    LIBCLOG_ENTER("pv=%p cb=%#x fFlags=%#x\n", pv, cb, fFlags);

    /*
     * BSD is ignoring the specifications and allows non-aligned
     * addresses and sizes. We do the same thing.
     */
    size_t off = (uintptr_t)pv & PAGE_MASK;
    pv = (char *)pv - off;
    cb += off;
    cb = (cb + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);

    /*
     * Call the backend to perform the action.
     */
    int rc = __libc_Back_mmanProtect((void *)pv, cb, fFlags);
    if (!rc)
        LIBCLOG_RETURN_INT(0);
    errno = rc;
    LIBCLOG_ERROR_RETURN_INT(-1);
}

