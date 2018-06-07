/* $Id: fwide.c 2121 2005-06-30 23:23:44Z bird $ */
/** @file
 *
 * LIBC - fwide
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
#include <wchar.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_STREAM
#include <InnoTekLIBC/logstrict.h>


/**
 * Sets/gets the stream orientation.
 *
 * @returns > 0 if wide char orientation.
 * @returns < 0 if byte orientation.
 * @returns 0 if no orientation.
 * @param   pStream         The stream.
 * @param   iOrientation    The new orientation, same as return value.
 */
int _STD(fwide)(FILE *pStream, int iOrientation)
{
    LIBCLOG_ENTER("pStream=%p iOrientation=%d\n", (void *)pStream, iOrientation);
    int iRet = -1;
#if 0 /** @todo LIBC07 */
    STREAM_LOCK(pStream);
    if (iOrientation != 0 && pStream->__iOrientation == 0)
        pStream->__iOrientation = iOrientation > 0 ? 1 : -1;
    iRet =  pStream->__iOrientation;
    STREAM_UNLOCK(pStream);
#endif
    LIBCLOG_RETURN_INT(iRet);
}
