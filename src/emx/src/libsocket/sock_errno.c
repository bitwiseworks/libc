/* $Id: sock_errno.c 3799 2014-02-05 17:41:41Z ydario $
 *
 * Wrapper for sock_errno return values - both modes.
 *
 * Copyright (c) 2003 InnoTek Systemberatung GmbH
 * Author: knut st. osmundsen <bird@anduin.net>
 *
 * All Rights Reserved
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


/**
 * Get the last socket error number.
 * This wrapper is required to get match our error defines in errno.h. OS/2
 * tcpip uses the FreeBSD / M$C errno.h values with an offset of 10000.
 *
 * @returns libc compatible errno for last socket operation.
 */
int TCPCALL sock_errno(void)
{
    LIBCLOG_ENTER("void\n");
    /* get the OS/2 error. */
    int err = os2_sock_errno();
    if (err > EOS2_TCPIP_OFFSET && err < EOS2_TCPIP_OFFSET + 1000)
        err -= EOS2_TCPIP_OFFSET;
    LIBCLOG_RETURN_INT(err);
}

