/* $Id: valloc.c 2254 2005-07-17 12:25:44Z bird $ */
/** @file
 *
 * valloc
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
#define _GNU_SOURCE
#include "libc-alias.h"
#include <stdlib.h>
#include <malloc.h>
#include <sys/param.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_HEAP
#include <InnoTekLIBC/logstrict.h>

/**
 * Allocate page aligned memory.
 *
 * @returns Pointer to the allocated heap block on success.
 * @returns NULL on failure.
 * @param   cb      Number of bytes to allocate.
 * @deprecated
 */
void	*_STD(valloc)(size_t cb)
{
    LIBCLOG_ENTER("cb=%d\n", cb);
    void *pv;
    int rc = posix_memalign(&pv, PAGE_SIZE, cb);
    if (!rc)
        LIBCLOG_RETURN_P(pv);
    LIBCLOG_RETURN_P(NULL);
}

