/* $Id: $ */
/** @file
 *
 * bind().
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
#include <sys/socket.h>
#include <sys/fcntl.h>
#include <emx/io.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_SOCKET
#include <InnoTekLIBC/logstrict.h>
#include "socket.h"

int bind(int socket, const struct sockaddr *addr, int addrlen)
{
    LIBCLOG_ENTER("socket=%d addr=%p addrlen=%d\n", socket, addr, addrlen);
    int             rc = -1;
    PLIBCSOCKETFH   pFHSocket = __libc_TcpipFH(socket);
    if (pFHSocket)
    {
        __LIBSOCKET_SAFEADDR SafeAddr;
        if (__libsocket_safe_addr_pre(addr, &addrlen, &SafeAddr) == 0)
        {
            rc = __libsocket_bind(pFHSocket->iSocket, SafeAddr.pAddr, addrlen);
            if (rc < 0)
                __libc_TcpipUpdateErrno();
            __libsocket_safe_addr_free(&SafeAddr);
        }
    }
    LIBCLOG_RETURN_INT(rc);
}

