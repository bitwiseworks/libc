/* $Id: fpathconf.c 3695 2011-03-15 23:30:51Z bird $ */
/** @file
 *
 * LIBC - fpathconf().
 *
 * Copyright (c) 2011 knut st. osmundsen <bird@innotek.de>
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
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_BACK_FS
#include <InnoTekLIBC/logstrict.h>


long _STD(fpathconf)(int fh, int iName)
{
    LIBCLOG_ENTER("fh=%d iName=%d\n", fh, iName);
    long lValue;
    int rc = __libc_Back_ioPathConf(fh, iName, &lValue);
    if (!rc)
        LIBCLOG_RETURN_LONG(lValue);
    errno = -rc;
    LIBCLOG_ERROR_RETURN_LONG(-1L);
}

