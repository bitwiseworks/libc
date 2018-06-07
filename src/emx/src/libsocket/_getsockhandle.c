/* $Id: _getsockhandle.c 1454 2004-09-04 06:22:38Z bird $ */
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
#include <emx/io.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_SOCKET
#include <InnoTekLIBC/logstrict.h>
#include "socket.h"


/**
 * Gets the OS/2 socket handle.
 *
 * @returns OS/2 socket handle.
 * @returns -1 on failure and errno set to EBADF or BNOSYS.
 * @param   socket  LIBC socket handle.
 */
int _getsockhandle(int socket)
{
    LIBCLOG_ENTER("socket=%d\n", socket);
    PLIBCSOCKETFH   pFHSocket = __libc_TcpipFH(socket);
    if (pFHSocket)
        LIBCLOG_RETURN_INT(pFHSocket->iSocket);
    LIBCLOG_RETURN_INT(-1);
}
