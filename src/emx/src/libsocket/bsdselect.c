/* $Id: bsdselect.c 1995 2005-06-05 08:02:48Z bird $ */
/** @file
 *
 * bsdselect().
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
#include <sys/time.h>
#include <emx/io.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_SOCKET
#include <InnoTekLIBC/logstrict.h>
#include "socket.h"

int bsdselect(int nfds, struct fd_set *readfds, struct fd_set *writefds, struct fd_set *exceptfds, struct timeval *tv)
{
    LIBCLOG_ENTER("nfds=%d readfds=%p writefds=%p exceptfds=%p tv=%p={tv_sec=%d, tv_usec=%ld}\n",
                  nfds, (void *)readfds, (void *)writefds, (void *)exceptfds, (void *)tv,
                  tv ? tv->tv_sec : -1, tv ? tv->tv_usec : -1);
    int rc = TCPNAMEG(bsdselect)(nfds, readfds, writefds, exceptfds, tv);
    LIBCLOG_RETURN_INT(rc);
}

