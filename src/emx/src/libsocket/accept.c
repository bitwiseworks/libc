/* $Id: $ */
/** @file
 *
 * accept().
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
#include <os2emx.h>
#include <InnoTekLIBC/sharedpm.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_SOCKET
#include <InnoTekLIBC/logstrict.h>
#include "socket.h"

int accept(int socket, struct sockaddr *addr, int *addrlen)
{
    LIBCLOG_ENTER("socket=%d socketaddr=%p addrlen=%p:{%d}\n",
                  socket, (void *)addr, (void *)addrlen, addrlen ? *addrlen : -1);
    PLIBCSOCKETFH   pFHSocket = __libc_TcpipFH(socket);
    int             s;
    if (pFHSocket)
    {
        __LIBSOCKET_SAFEADDR SafeAddr;
        if (__libsocket_safe_addr_pre(addr, addrlen, &SafeAddr) == 0)
        {
            s = __libsocket_accept(pFHSocket->iSocket, SafeAddr.pAddr, SafeAddr.pcbAddr);
            if (s >= 0)
            {
                if (__libsocket_safe_addr_post(addr, addrlen, &SafeAddr, 1) == 0)
                {
                    int             fh;
                    PLIBCSOCKETFH   pFH = TCPNAMEG(AllocFH)(s, &fh);
                    if (pFH)
                        LIBCLOG_RETURN_INT(fh);
                    
                    __libsocket_soclose(s);
                    __libc_TcpipSetSocketErrno();
                }        
                else
                    __libsocket_soclose(s);
            }
            else
            {
                __libc_TcpipUpdateErrno();
                __libsocket_safe_addr_post(addr, addrlen, &SafeAddr, 0);
            }
        }
    }
    LIBCLOG_RETURN_INT(-1);
}

