/* $Id: asprintf.c 2128 2005-07-01 02:15:24Z bird $ */
/** @file
 *
 * LIBC - asprintf.
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
#include "libc-alias.h"
#include <stdio.h>
#include <stdarg.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_STREAM
#include <InnoTekLIBC/logstrict.h>

/**
 * Allocating sprintf.
 *
 * @returns number of formatted bytes excluding the terminating zero.
 * @returns -1 on and errno on failure.
 * @param   ppsz        Where to store the allocated string pointer.
 *                      On failure this will be NULL.
 * @param   pszFormat   Format string.
 * @param   ...         Format arguments.
 */
int _STD(asprintf)(char **ppsz, const char *pszFormat, ...)
{
    LIBCLOG_ENTER("ppsz=%p pszFormat=%p:{%s}\n", (void *)ppsz, (void *)pszFormat, pszFormat);
    va_list args;
    va_start(args, pszFormat);
    int cch = vasprintf(ppsz, pszFormat, args);
    va_end(args);
    LIBCLOG_RETURN_MSG(cch, "ret %d ppsz=%p\n", cch, ppsz ? *ppsz : (void *)0xdeadbeef);
}

