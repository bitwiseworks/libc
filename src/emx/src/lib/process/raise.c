/* $Id: $ */
/** @file
 *
 * LIBC - raise().
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
#include <process.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_SIGNAL
#include <InnoTekLIBC/logstrict.h>


/**
 * Raise a signal in the current thread.
 *
 * According to the standards this should only raise the signal in the
 * current thread, however, FreeBSD libc and GLIBC both forwards this
 * call to kill(). We do the same to be compatible. :-)
 *
 * @returns 0 on success.
 * @returns -1 on failure, errno set to EINVAL.
 * @param   iSignalNo   Signal to raise.
 */
int _STD(raise)(int iSignalNo)
{
    LIBCLOG_ENTER("iSignalNo=%d\n", iSignalNo);
    int rc = kill(getpid(), iSignalNo);
    if (!rc)
        LIBCLOG_RETURN_INT(rc);
    LIBCLOG_ERROR_RETURN_INT(rc);
}

