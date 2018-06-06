/* $Id: getline.c 2259 2005-07-17 13:43:12Z bird $ */
/** @file
 *
 * LIBC - getline, GLIBC extension.
 *
 * Copyright (c) 2005 knut st. osmundsen <bird@anduin.net>
 *
 *
 * This file is part of InnoTek LIBC.
 *
 * InnoTek LIBC is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * InnoTek LIBC is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with InnoTek LIBC; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#define _GNU_SOURCE
#include "libc-alias.h"
#include <stdio.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_STREAM
#include <InnoTekLIBC/logstrict.h>


/**
 * Retrieves a line from the specified stream and puts it into
 * the buffer pointed to by *ppszLine, reallocating that if it's to small.
 *
 * @returns number of bytes read.
 * @returns -1 on failure, including EOF.
 * @param   ppszLine        Where to buffer pointer is stored. *ppszLine can of course be NULL.
 * @param   pcchLine        Size of the buffer pointed to by *ppszLine.
 * @param   pStream         The stream to read from.
 */
ssize_t _STD(getline)(char **ppszLine, size_t *pcchLine, FILE *pStream)
{
    LIBCLOG_ENTER("ppszLine=%p:{%p}, pcchLine=%p:{%u} pStream=%p\n",
                  (void *)ppszLine, (void *)*ppszLine, (void *)pcchLine, (int)*pcchLine, (void *)pStream);
    ssize_t rc = getdelim(ppszLine, pcchLine, '\n', pStream);
    LIBCLOG_RETURN_MSG(rc, "ret %d *ppszLine=%p *pcchLine=%u\n", rc, *ppszLine, (int)pcchLine);
}

