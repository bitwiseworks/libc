/* $Id: os2_select.c 1764 2005-01-17 01:56:43Z bird $ */
/** @file
 *
 * os2_select().
 *
 * Copyright (c) 2003-2005 knut st. osmundsen <bird-srcspam@anduin.net>
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

int os2_select(int *s, int nreads, int nwrites, int nexcepts, long timeout)
{
    LIBCLOG_ENTER("s=%p nreads=%d nwrites=%d nexcepts=%d timeout=%ld\n", (void *)s, nreads, nwrites, nexcepts, timeout);
    int     cSockets;
    int    *aSockets;
    int     i;
    int     rc;

    /*
     * Convert the sockets.
     */
    cSockets = nreads + nwrites + nexcepts;
    aSockets = alloca(cSockets * sizeof(int));
    if (!aSockets)
    {
        __libc_TcpipSetErrno(ENOMEM);
        LIBCLOG_RETURN_INT(-1);
    }
    for (i = 0; i < cSockets; i++)
    {
        PLIBCSOCKETFH   pFHSocket = __libc_TcpipFH(s[i]);
        if (!pFHSocket)
        {
            __libc_TcpipSetErrno(EBADF);
            LIBCLOG_RETURN_INT(-1);
        }
        aSockets[i] = pFHSocket->iSocket;
    }

    /*
     * Do the call with new array.
     */
    rc = __libsocket_os2_select(aSockets, nreads, nwrites, nexcepts, timeout);
    if (rc >= 0)
    {
        /*
         * Sync back the non-ready indicators (-1) to the passed in table.
         */
        for (i = 0; i < cSockets; i++)
            if (aSockets[i] == -1)
                s[i] = -1;
        LIBCLOG_RETURN_INT(rc);
    }
    __libc_TcpipUpdateErrno();
    LIBCLOG_RETURN_INT(-1);
}

