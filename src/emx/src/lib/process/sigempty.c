/* $Id: $ */
/** @file
 *
 * LIBC - sigemptyset()
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
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_SIGNAL
#include <InnoTekLIBC/logstrict.h>


/**
 * Empties (i.e. deletes all signals) a signal set.
 *
 * @returns 0 indicating success.
 * @param   pSigSet     Pointer to signal set to empty.
 */
int _STD(sigemptyset)(sigset_t *pSigSet)
{
    LIBCLOG_ENTER("pSigSet=%p\n", (void*)pSigSet);

    /*
     * Empty signal set.
     */
    __SIGSET_EMPTY(pSigSet);
    LIBCLOG_RETURN_INT(0);
}
