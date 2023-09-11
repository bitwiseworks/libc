/* $Id: $ */
/** @file
 *
 * recv().
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
#include <InnoTekLIBC/libc.h>
#include "socket.h"

ssize_t recv(int socket, void *buf, size_t len, int flags)
{
    LIBCLOG_ENTER("socket=%d buf=%p len=%d flags=%#x\n", socket, buf, len, flags);
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
        if (buf)
            __libc_touch(buf, len);
        rc = __libsocket_recv(pFHSocket->iSocket, buf, len, flags);
        if (rc >= 0)
            LIBCLOG_RETURN_INT(rc);
        __libc_TcpipUpdateErrno();
    }

    LIBCLOG_RETURN_INT(-1);
}


