/* $Id: recvmsg.c 2516 2006-02-04 12:26:03Z bird $ */
/** @file
 *
 * recvmsg().
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
#include <netinet/in.h>
#include <sys/fcntl.h>
#include <emx/io.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_SOCKET
#include <InnoTekLIBC/logstrict.h>
#include "socket.h"

extern void touch(void *base, unsigned long count); /* from emxdll.h */

ssize_t recvmsg(int socket, struct msghdr *msg, int flags)
{
    LIBCLOG_ENTER("socket=%d msg=%p flags=%d\n", socket, msg, flags);
    PLIBCSOCKETFH   pFHSocket = __libc_TcpipFH(socket);
    if (pFHSocket)
    {
        int rc;
        /*
         * OS/2 TCP/IP stack installs its own exception handler and returns
         * errno 14 (EFAULT) if the buffer is not committed, not letting other
         * exception handlers run. This breaks e.g. LIBCx mmap implementation.
         * Touch all pages to trigger those handlers, if any.
         *
         * NOTE: that this slightly breaks BSD compatibility (but not POSIX) as
         * EFAULT won't be returned in cases where buf is an invalid pointer -
         * the application will simply crash instead. See #123 for more details.
         */
        if (msg && msg->msg_iov)
        {
            for (int i = 0; i < msg->msg_iovlen; ++i)
                if (msg->msg_iov[i].iov_base)
                    touch(msg->msg_iov[i].iov_base, msg->msg_iov[i].iov_len);
        }
        rc = __libsocket_recvmsg(pFHSocket->iSocket, msg, flags);
        if (rc >= 0)
        {
            /* Workaround for missing msg_namelen update. Required if a IPV6 sized buffer is given.
             * (This problem hasn't been explored well enough, so this workaround is rather cautious.)
             */
            if (msg)
            {
                struct sockaddr_in *pAddr = (struct sockaddr_in *)msg->msg_name;
#ifndef TCPV40HDRS
                if (pAddr && msg->msg_namelen > pAddr->sin_len)
                    msg->msg_namelen = pAddr->sin_len;
#else

                if (    pAddr
                    &&  pAddr->sin_family == AF_INET
                    &&  msg->msg_namelen > sizeof(struct sockaddr_in))
                    msg->msg_namelen = sizeof(struct sockaddr_in);
#endif

            }
            LIBCLOG_RETURN_INT(rc);
        }
        __libc_TcpipUpdateErrno();
    }

    LIBCLOG_RETURN_INT(-1);
}


