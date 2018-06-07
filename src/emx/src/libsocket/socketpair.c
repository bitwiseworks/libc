/* $Id: socketpair.c 3747 2012-03-04 18:45:55Z bird $
 *
 * socketpair for BSD 4.3 stack.
 *
 * InnoTek Systemberatung GmbH confidential
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
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/fcntl.h>
#include <emx/io.h>
#include <os2emx.h>
#include <InnoTekLIBC/sharedpm.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_SOCKET
#include <InnoTekLIBC/logstrict.h>
#include "socket.h"


/**
 * Socket pair.
 * @returns 0 on success.
 * @returns -1 on failure.
 * @param   af      Socket familiy, AF_UNIX(/AF_OS2/AF_LOCAL) only.
 * @param   type    Socket type, SOCK_STREAM only.
 * @param   flags   Protocol type. 0 only.
 * @param   osfd    Pointer to an array of two ints.
 *                  The resulting socket pair is put into the array upon successful return.
 */
int     TCPCALL socketpair(int af, int type, int flags, int *osfd)
{
    LIBCLOG_ENTER("af=%d type=%d flags=%#x osfd=%p\n", af, type, flags, (void *)osfd);
#ifdef TCPV40HDRS
    int     rc;
    int     sock1 = -1;
    int     sock1accept = -1;
    int     sock2 = -1;
    struct timeval      tv;
    struct sockaddr_un  un;

    /* validate input. */
    if (af != AF_UNIX)
    {
        socket(0xfffdadb, type, 0); /*EAFNOSUPPORT;*/
        LIBCLOG_RETURN_INT(-1);
    }
    if (type != SOCK_STREAM)
    {
        socket(af, 0x1230fd, 0); /*EPROTOTYPE;*/
        LIBCLOG_RETURN_INT(-1);
    }

    /* make sure we crash here if parameter is invalid. */
    osfd[0] = -1;
    osfd[1] = -1;

    /* create the two sockets. */
    sock1 = socket(af, type, flags);
    if (sock1 < 0)
        goto failure;

    sock2 = socket(af, type, flags);
    if (sock1 < 0)
        goto failure;

    /* Make unique socket name and perform bind. */
    memset(&un, 0, sizeof(un));
    un.sun_family = AF_UNIX;
    gettimeofday(&tv, NULL);
    sprintf(un.sun_path, "\\socket\\%dgcc%ld%ld", sock2, tv.tv_sec, tv.tv_usec);
    rc = bind(sock1, (struct sockaddr*)&un, sizeof(un));
    if (rc)
        goto failure;

    /* listen */
    rc = listen(sock1, 5);
    if (rc)
        goto failure;

    /* let socket 2 connect to it */
    rc = connect(sock2, (struct sockaddr*)&un, sizeof(un));
    if (rc)
        goto failure;

    /* accept connect. */
    sock1accept = accept(sock1, 0, 0);
    if (sock1accept < 0)
        goto failure;
    soclose(sock1);
    sock1 = -1;

    /* we're done, just need to fill in the return values. */
    osfd[0] = sock1accept;
    osfd[1] = sock2;
    LIBCLOG_RETURN_MSG(0, "ret 0 osfd[0]=%d osfd[1]=%d\n", osfd[0], osfd[1]);

failure:
    if (sock1 >= 0)
        soclose(sock1);
    if (sock1accept >= 0)
        soclose(sock1accept);
    if (sock2 >= 0)
        soclose(sock2);
    LIBCLOG_RETURN_INT(-1);

#else
    int             rc;
    int             fh1;
    int             fh2;
    int             aNativeFDs[2] = {-1, -1};
    PLIBCSOCKETFH   pFH1;

    /* Make sure we crash here if parameter is invalid. */
    osfd[0] = -1;
    osfd[1] = -1;

    /* Call the native API. */
    rc = __libsocket_socketpair(af, type, flags, aNativeFDs);
    if (rc < 0)
    {
        __libc_TcpipUpdateErrno();
        LIBCLOG_RETURN_INT(-1);
    }

    /*
     * Allocate LIBC sockets.
     */
    pFH1 = TCPNAMEG(AllocFH)(aNativeFDs[0], &fh1);
    if (pFH1)
    {
        PLIBCSOCKETFH   pFH2 = TCPNAMEG(AllocFH)(aNativeFDs[1], &fh2);
        if (pFH2)
        {
            osfd[0] = fh1;
            osfd[1] = fh2;

            LIBCLOG_RETURN_MSG(0, "ret 0 osfd[0]=%d osfd[1]=%d\n", osfd[0], osfd[1]);
        }
         
        __libsocket_soclose(aNativeFDs[1]);
        __libc_FHClose(fh1);
    }
    else
    {
        __libsocket_soclose(aNativeFDs[1]);
        __libsocket_soclose(aNativeFDs[0]);
    }

    LIBCLOG_RETURN_INT(-1);
#endif
}

