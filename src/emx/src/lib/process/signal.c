/* $Id: $ */
/** @file
 *
 * LIBC - BSD Style signal().
 *
 * Copyright (c) 2004 knut st. osmundsen <bird-srcspam@anduin.net>
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
#include <signal.h>
#include <InnoTekLIBC/backend.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_SIGNAL
#include <InnoTekLIBC/logstrict.h>


/**
 * Default signal() style is BSD.
 *
 * @returns pointer to previous signal handler.
 * @param   iSignalNo   Signal number to set the handler for.
 * @param   pfnHandler  Pointer to the new signal handler.
 */
void (*_STD(signal)(int iSignalNo, void (*pfnHandler)(int)))(int)
{
    LIBCLOG_ENTER("iSignalNo=%d pfnHandler=%p\n", iSignalNo, (void*)pfnHandler);
    void (*pfnRet)(int) = bsd_signal(iSignalNo, pfnHandler);
    if (pfnRet != SIG_ERR)
        LIBCLOG_RETURN_P(pfnRet);
    LIBCLOG_ERROR_RETURN_P(pfnRet);
}

