/* $Id: _impsockhandle.c 1454 2004-09-04 06:22:38Z bird $ */
/** @file
 *
 * _getsockhandle()
 *
 * Copyright (c) 2003 knut st. osmundsen <bird-srcspam@anduin.net>
 *
 *
 * This file is part of Innotek LIBC.
 *
 * Innotek LIBC is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Innotek LIBC is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with Innotek LIBC; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#include "libc-alias.h"
#include <errno.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/fcntl.h>
#include <emx/io.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_SOCKET
#include <InnoTekLIBC/logstrict.h>
#include "socket.h"


/**
 * Imports an socket handle from the OS/2 handle space.
 *
 * @returns LIBC socket handle.
 * @returns -1 on failure and errno set to something sensible.
 * @param   os2socket   OS/2 socket handle.
 * @param   flags       Reserved for the future by EM - ignored.
 */
int _impsockhandle(int os2socket, int flags)
{
    LIBCLOG_ENTER("os2socket=%d flags=%#x\n", os2socket, flags);
    PLIBCSOCKETFH   pFH;
    int             iType;
    int             cb;
    int             fh;

    /*
     * Verify the socket.
     */
    cb = sizeof(iType);
    if (__libsocket_getsockopt(os2socket, SOL_SOCKET, SO_TYPE, (char*)&iType, &cb) < 0)
    {
        __libc_TcpipSetErrno(EBADF);
        LIBCLOG_RETURN_INT(-1);
    }

    /*
     * Allocate LIBC handle.
     */
    pFH = TCPNAMEG(AllocFH)(os2socket, &fh);
    if (!pFH)
        LIBCLOG_RETURN_INT(-1);
    LIBCLOG_RETURN_INT(fh);
}

