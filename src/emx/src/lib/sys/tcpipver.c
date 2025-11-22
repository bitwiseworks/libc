/* $Id: tcpipver.c 3806 2014-02-06 11:38:18Z ydario $ */
/** @file
 *
 * LIBC SYS Backend - TCP/IP Version Specific Code.
 *
 * This file is included from tcpipver43.c with TCPV40HDRS defined,
 * this might be a little bit confusing but it saves a lot of coding.
 *
 *
 * Copyright (c) 2004-2005 knut st. osmundsen <bird-srcspam@anduin.net>
 *
 *
 * This file is part of InnoTek LIBC.
 *
 * InnoTek LIBC is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * InnoTek LIBC is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with InnoTek LIBC; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */


/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#define INCL_TCPIP_ALLIOCTLS
#include "libc-alias.h"
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/sockio.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <netinet/if_ether.h>
#include <netinet/ip.h>
#include <netinet/ip_var.h>
#include <netinet/ip_icmp.h>
#include <netinet/tcp.h>
#include <netinet/tcp_var.h>
#include <netinet/udp.h>
#include <netinet/udp_var.h>
#include <net/route.h>

#include <sys/fcntl.h>
#include <sys/filio.h>
#include <sys/time.h>
#include <386/builtin.h>
#include <sys/smutex.h>
#include <emx/io.h>
#include <emx/startup.h>
#define INCL_BASE
#define INCL_FSMACROS
#define INCL_EXAPIS
#include <os2emx.h>
#include <InnoTekLIBC/sharedpm.h>
#include <InnoTekLIBC/tcpip.h>
#include <InnoTekLIBC/fork.h>
#include <InnoTekLIBC/thread.h>
#include <InnoTekLIBC/backend.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_SOCKET
#include <InnoTekLIBC/logstrict.h>

#include "b_fs.h"

/*******************************************************************************
*   Defined Constants And Macros                                               *
*******************************************************************************/
#ifdef TCPV40HDRS

#define MY_FD_SET(fd, set)      FD_SET(fd, set)
#define MY_FD_ISSET(fd, set)    FD_ISSET(fd, set)
#define MY_FD_ZERO(set, cb)     bzero(set, cb);
#define my_fd_set               fd_set

#else

#define MY_FD_SET(fd, set)      V5_FD_SET(fd, set)
#define	MY_FD_ISSET(fd, set)    V5_FD_ISSET(fd, set)
#define MY_FD_ZERO(set, cb)     bzero(set, cb);
#define my_fd_set               v5_fd_set

#endif


/*******************************************************************************
*   Structures and Typedefs                                                    *
*******************************************************************************/
#ifdef TCPV40HDRS
/* not included in the TCPV40HDRS - this is from the BSD4.4 stack headers
                                    and doesn't have to be right!!! */
struct arpreq_tr {
        struct  sockaddr arp_pa;                /* protocol address */
        struct  sockaddr arp_ha;                /* hardware address */
        long    arp_flags;                     /* flags */
        u_short arp_rcf;                        /* token ring routing control field */
        u_short arp_rseg[8];                    /* token ring routing segments */
};

#define MAX_IN_MULTI      16*IP_MAX_MEMBERSHIPS      /* 320 max per os2 */
struct  addrreq  {                              /* get multicast addresses */
        char    ifr_name[IFNAMSIZ];
        struct  sockaddr ifr_addrs;
        u_long  maddr[MAX_IN_MULTI];
};

#pragma pack(1)
struct  oarptab {
        struct  in_addr at_iaddr;       /* internet address */
        u_char  at_enaddr[6];           /* ethernet address */
        u_char  at_timer;               /* minutes since last reference */
        u_char  at_flags;               /* flags */
#ifdef KERNEL
        struct  mbuf *at_hold;          /* last packet until resolved/timeout */
#else
        void * at_hold;
#endif
        u_short at_rcf;                 /* token ring routing control field */
        u_short at_rseg[8];             /* token ring routing segments */
        u_long  at_millisec;            /* TOD milliseconds of last update */
        u_short at_interface;           /* interface index */
};
#pragma pack()

#pragma pack(1)
struct  statatreq {
        u_long addr;
        short interface;
        u_long mask;
        u_long broadcast;
};
#pragma pack()

#endif

/*******************************************************************************
*   Internal Functions                                                         *
*******************************************************************************/
static int TCPNAME(ops_Close)(PLIBCFH pFH, int fh);
static int TCPNAME(ops_Read)(PLIBCFH pFH, int fh, void *pvBuf, size_t cbRead, size_t *pcbRead);
static int TCPNAME(ops_Write)(PLIBCFH pFH, int fh, const void *pvBuf, size_t cbWrite, size_t *pcbWritten);
static int TCPNAME(ops_Duplicate)(PLIBCFH pFH, int fh, int *pfhNew);
static int TCPNAME(ops_FileControl)(PLIBCFH pFH, int fh, int iIOControl, int iArg, int *prc);
static int TCPNAME(ops_IOControl)(PLIBCFH pFH, int fh, int iIOControl, int iArg, int *prc);
static int TCPNAME(ops_Select)(int cFHs, struct fd_set *pRead, struct fd_set *pWrite, struct fd_set *pExcept, struct timeval *tv, int *prc);
static int TCPNAME(ops_ForkChild)(struct __libc_FileHandle *pFH, int fh, __LIBC_PFORKHANDLE pForkHandle, __LIBC_FORKOP enmOperation);

static int TCPNAME(imp_soclose)(int s);
#ifndef TCPV40HDRS
static int TCPNAME(imp_ioctl)(int, int, char *);
#endif
static int TCPNAME(imp_os2_ioctl)(int, unsigned long, char *, int);
static int TCPNAME(imp_recv)(int, void *, int, int);
static int TCPNAME(imp_send)(int, const void *, int, int);
static _Bool TCPNAME(imp_removesocketfromlist)(int);
static void TCPNAME(imp_addsockettolist)(int);
#ifdef TCPV40HDRS
static int TCPNAME(imp_bsdselect)(int, fd_set *, fd_set *, fd_set *, struct timeval *);
#else
static int TCPNAME(imp_bsdselect)(int, v5_fd_set *, v5_fd_set *, v5_fd_set *, struct timeval *);
#endif

static void TCPNAME(Cleanup)(void);


/*******************************************************************************
*   Global Variables                                                           *
*******************************************************************************/
/** Head of sockets. */
static volatile PLIBCSOCKETFH   gpSocketsHead;
/** Spin mutex protecting the gpSocketsHead list. */
static _smutex                  gsmtxSockets;

/** Operations on sockets. */
static const __LIBC_FHOPS       gSocketOps =
{
#ifdef TCPV40HDRS
    enmFH_Socket43, /* Handle type. */
#else
    enmFH_Socket44, /* Handle type. */
#endif
    TCPNAME(ops_Close),
    TCPNAME(ops_Read),
    TCPNAME(ops_Write),
    TCPNAME(ops_Duplicate),
    TCPNAME(ops_FileControl),
    TCPNAME(ops_IOControl),
    TCPNAME(ops_Select),
    NULL /* fork parent */,
    TCPNAME(ops_ForkChild)
};


/** Close operation.
 * @returns 0 on success.
 * @returns OS/2 error code or negated errno on failure.
 * @param   pFH         Pointer to the handle structure to operate on.
 * @param   fh          It's associated filehandle.
 */
static int TCPNAME(ops_Close)(PLIBCFH pFH, int fh)
{
    LIBCLOG_ENTER("pFH=%p:{.iSocket=%d} fh=%d\n", (void *)pFH, ((PLIBCSOCKETFH)pFH)->iSocket, fh);
    PLIBCSOCKETFH   pSocketFH = (PLIBCSOCKETFH)pFH;
    int             rc;

    /*
     * Dereference it and see if closing is required.
     */
    rc = __libc_spmSocketDeref(pSocketFH->iSocket);
    if (rc < 0)
        rc = rc; /* error */
    else if (!rc)
    {
        /*
         * Noone is using the socket now, close it.
         */
        rc = TCPNAME(imp_soclose)(pSocketFH->iSocket);
        if (rc)
        {
            rc = -__libc_TcpipGetSocketErrno();
            if (rc != -EBADF && rc != -ENOTSOCK && rc != -EALREADY)
            {
                LIBC_ASSERTM_FAILED("undocument errno from soclose() rc=%d\n", rc);
                /* Add reference again. the socket is probably not closed. */
                __libc_spmSocketRef(pSocketFH->iSocket);
            }
            /** @todo deal with EALREADY, it will leave an unclosed and soon to be invalid handle behind... */
        }
    }
    else if (rc >> 16) /* (high word is process reference count) */
        /* The socket is duplicated within the current process, it will be close later when the other handle(s) is(/are) closed. */
        rc = 0;
    else
    {
        /*
         * Someone other process is using the socket, we'll remove it from the list of
         * sockets owned by this process to prevent it from being closed on exit. The other process(es)
         * will do the closing.
         *
         * If there is a thread waiting on a blocking socket in this process, it will not get
         * notified by removesocketfromlist and continue to wait forever. Work around this by
         * canceling the socket (note that soclose is not called since it is in use by another
         * process). This will cause EINTR to be returned for the socket and no further operation
         * will succeed since it will be gone from the kLIBC POV in this process by the time we
         * return. Other processes should not be affected (they should retry on EINTR).
         *
         * Please note that removesocketfromlist returns a boolean success indicator and probably no errno.
         */
        int iSavedTcpipErrno = TCPNAME(imp_sock_errno)();
        TCPNAME(imp_set_errno)(0);
        rc = !TCPNAME(imp_removesocketfromlist)(pSocketFH->iSocket);
        if (rc)
        {
            /*
             * @todo removesocketfromlist fails in some cases (e.g. when the child inheritance chain
             * is too long and complex). This might be a race in TCPIP32.DLL. Do not assert for now.
             */
#if 0
            LIBC_ASSERTM_FAILED("removesocketfromlist(%d) -> false! sock_errno()->%d\n", pSocketFH->iSocket, TCPNAME(imp_sock_errno)());
#else
            LIBCLOG_MSG("removesocketfromlist(%d) -> false! sock_errno()->%d\n", pSocketFH->iSocket, TCPNAME(imp_sock_errno)());
#endif
            rc = -EBADF; /* this is an internal error, it should *NOT* happen! */
        }
        TCPNAME(imp_set_errno)(iSavedTcpipErrno);
    }

    /*
     * Unlink from our list (rc list must correspond to __libc_FHClose!).
     */
    if (    !rc
        ||  rc == -EBADF
        ||  rc == -ENOTSOCK)
    {
        LIBCLOG_MSG("Unlinking pSocketFH=%p:{iSocket=%d} fh=%d\n", pSocketFH, pSocketFH->iSocket, fh);
        _smutex_request(&gsmtxSockets);
        if (pSocketFH->pNext)
            pSocketFH->pNext->pPrev = pSocketFH->pPrev;
        if (pSocketFH->pPrev)
            pSocketFH->pPrev->pNext = pSocketFH->pNext;
        else
            gpSocketsHead = pSocketFH->pNext;
        _smutex_release(&gsmtxSockets);
    }

    LIBCLOG_MIX0_RETURN_INT(rc);
}


/** Read operation.
 * @returns 0 on success.
 * @returns OS/2 error code or negated errno on failure.
 * @param   pFH         Pointer to the handle structure to operate on.
 * @param   fh          It's associated filehandle.
 * @param   pvBuf       Pointer to the buffer to read into.
 * @param   cbRead      Number of bytes to read.
 * @param   pcbRead     Where to store the count of bytes actually read.
 */
static int TCPNAME(ops_Read)(PLIBCFH pFH, int fh, void *pvBuf, size_t cbRead, size_t *pcbRead)
{
    LIBCLOG_ENTER("pFH=%p:{.iSocket=%d} fh=%d pvBuf=%p cbRead=%d pcbRead=%p\n",
                  (void *)pFH, ((PLIBCSOCKETFH)pFH)->iSocket, fh, pvBuf, cbRead, (void *)pcbRead);
    PLIBCSOCKETFH   pSocketFH = (PLIBCSOCKETFH)pFH;
    int             rc;
    int             fFlags = 0;
#if 0 /* doesn't work for 4.3 */
    if (pFH->fFlags & O_NONBLOCK)
        fFlags = MSG_DONTWAIT;
#endif
    rc = TCPNAME(imp_recv)(pSocketFH->iSocket, pvBuf, cbRead, fFlags);
    if (rc < 0)
    {
        *pcbRead = 0;
        rc = -__libc_TcpipGetSocketErrno();
        LIBCLOG_ERROR_RETURN_INT(rc);
    }

    *pcbRead = rc;
    LIBCLOG_RETURN_INT(0);
}

/** Write operation.
 * @returns 0 on success.
 * @returns OS/2 error code or negated errno on failure.
 * @param   pFH         Pointer to the handle structure to operate on.
 * @param   fh          It's associated filehandle.
 * @param   pvBuf       Pointer to the buffer which contains the data to write.
 * @param   cbWrite     Number of bytes to write.
 * @param   pcbWritten  Where to store the count of bytes actually written.
 */
static int TCPNAME(ops_Write)(PLIBCFH pFH, int fh, const void *pvBuf, size_t cbWrite, size_t *pcbWritten)
{
    LIBCLOG_ENTER("pFH=%p:{.iSocket=%d} fh=%d pvBuf=%p cbWrite=%d pcbWritten=%p\n",
                  (void *)pFH, ((PLIBCSOCKETFH)pFH)->iSocket, fh, pvBuf, cbWrite, (void *)pcbWritten);
    PLIBCSOCKETFH   pSocketFH = (PLIBCSOCKETFH)pFH;
    int             rc;
    int             fFlags = 0;
#if 0 /* doesn't work for 4.3 */
    if (pFH->fFlags & O_NONBLOCK)
        fFlags = MSG_DONTWAIT;
#endif
    rc = TCPNAME(imp_send)(pSocketFH->iSocket, pvBuf, cbWrite, fFlags);
    if (rc < 0)
    {
        *pcbWritten = 0;
        rc = -__libc_TcpipGetSocketErrno();
        LIBCLOG_ERROR_RETURN_INT(rc);
    }

    *pcbWritten = rc;
    LIBCLOG_RETURN_INT(0);
}


/** Duplicate handle operation.
 * @returns 0 on success, OS/2 error code on failure.
 * @param   pFH         Pointer to the handle structure to operate on.
 * @param   fh          It's associated filehandle.
 * @param   pfhNew      Where to store the duplicate filehandle.
 *                      The input value describe how the handle is to be
 *                      duplicated. If it's -1 a new handle is allocated.
 *                      Any other value will result in that value to be
 *                      used as handle. Any existing handle with that
 *                      value will be closed.
 */
static int TCPNAME(ops_Duplicate)(PLIBCFH pFH, int fh, int *pfhNew)
{
    PLIBCSOCKETFH  pFHSocket = (PLIBCSOCKETFH)pFH;
    LIBCLOG_ENTER("pFH=%p:{.iSocket=%d} fh=%d pfhNew=%p\n", (void *)pFH, pFHSocket->iSocket, fh, (void *)pfhNew);

    /*
     * Allocate a new filehandle matching the request and copy the data over.
     */
    PLIBCSOCKETFH  pFHNew;
    int rc = TCPNAMEG(AllocFHEx)(*pfhNew, pFHSocket->iSocket, O_RDWR | F_SOCKET, 0/*old*/, pfhNew, &pFHNew);
    if (!rc)
    {
        pFHNew->core.fFlags      = pFHSocket->core.fFlags;
        pFHNew->core.fFlags     &= ~((FD_CLOEXEC << __LIBC_FH_FDFLAGS_SHIFT) | O_NOINHERIT);
        pFHNew->core.iLookAhead  = pFHSocket->core.iLookAhead;
        pFHNew->core.Inode       = pFHSocket->core.Inode;
        pFHNew->core.Dev         = pFHSocket->core.Dev;
        pFHNew->core.pFsInfo     = pFHSocket->core.pFsInfo;
        LIBCLOG_RETURN_MSG(0, "ret 0 (0x0) *pfhNew=%d\n", *pfhNew);
    }

    LIBCLOG_ERROR_RETURN_INT(rc);
}


/** File Control operation.
 * @returns 0 on success.
 * @returns OS/2 error code or negated errno on failure.
 * @param   pFH         Pointer to the handle structure to operate on.
 * @param   fh          It's associated filehandle.
 * @param   iRequest    Which file file descriptior request to perform.
 * @param   iArg        Argument which content is specific to each
 *                      iRequest operation.
 * @param   prc         Where to store the value which upon success is
 *                      returned to the caller.
 */
static int TCPNAME(ops_FileControl)(PLIBCFH pFH, int fh, int iRequest, int iArg, int *prc)
{
    LIBCLOG_ENTER("pFH=%p:{.iSocket=%d} fh=%d iRequest=%#x iArg=%#x prc=%p\n",
                  (void *)pFH, ((PLIBCSOCKETFH)pFH)->iSocket, fh, iRequest, iArg, (void *)prc);
    int rc;

    /*
     * Service the request (very similar to __fcntl()!).
     */
    switch (iRequest)
    {
        /*
         * These can be forwarded handled as-if this was a normal file.
         */
        case F_GETFL:
        case F_GETFD:
        case F_SETFD:
            rc = __libc_Back_ioFileControlStandard(pFH, fh, iRequest, iArg, prc);
            break;

        /*
         * For this one we'll have to listen in to the O_NONBLOCK flag.
         */
        case F_SETFL:
        {
            if ((iArg ^ pFH->fFlags) & O_NONBLOCK)
            {
                PLIBCSOCKETFH   pSocketFH = (PLIBCSOCKETFH)pFH;
                int             fFlag = (iArg & O_NONBLOCK) != 0;
                rc = TCPNAME(imp_os2_ioctl)(pSocketFH->iSocket, FIONBIO, (char *)&fFlag, sizeof(int));
                if (!rc)
                {
                    rc = __libc_Back_ioFileControlStandard(pFH, fh, iRequest, iArg, prc);
                    if (rc)
                    {
                        /* undo change on failure. */
                        fFlag = (iArg & O_NONBLOCK) == 0;
                        TCPNAME(imp_os2_ioctl)(pSocketFH->iSocket, FIONBIO, (char *)&fFlag, sizeof(int));
                    }
                }
            }
            else
                rc = __libc_Back_ioFileControlStandard(pFH, fh, iRequest, iArg, prc);
            break;
        }

        /*
         * Other operations are not supported on sockets.
         */
        default:
            *prc = -1;
            LIBCLOG_ERROR_RETURN(-EINVAL, "Invalid or unsupported request %#x %#x\n", iRequest, iArg);
    }

    LIBCLOG_MIX0_RETURN_INT(rc);
}


/** I/O Control operation.
 * @returns 0 on success.
 * @returns OS/2 error code or negated errno on failure.
 * @param   pFH         Pointer to the handle structure to operate on.
 * @param   fh          It's associated filehandle.
 * @param   iIOControl  Which I/O control operation to perform.
 * @param   iArg        Argument which content is specific to each
 *                      iIOControl operation.
 * @param   prc         Where to store the value which upon success is
 *                      returned to the caller.
 */
static int TCPNAME(ops_IOControl)(PLIBCFH pFH, int fh, int iIOControl, int iArg, int *prc)
{
    LIBCLOG_ENTER("pFH=%p:{.iSocket=%d} fh=%d iIOControl=%#x iArg=%#x prc=%p\n",
                  (void *)pFH, ((PLIBCSOCKETFH)pFH)->iSocket, fh, iIOControl, iArg, (void *)prc);
    PLIBCSOCKETFH   pSocketFH = (PLIBCSOCKETFH)pFH;
    char           *pchArg = (char *)iArg;
    int             rc;
#ifndef TCPV40HDRS
    switch (__IOCLW(iIOControl))
    {
        case __IOCLW(SIOSTATARP):
            rc = TCPNAME(imp_os2_ioctl)(pSocketFH->iSocket, SIOSTATARP, pchArg, sizeof(struct oarptab));
            break;
        case __IOCLW(SIOSTATAT):
            /** this isn't really suitable for this ioctl interface!! */
            rc = TCPNAME(imp_os2_ioctl)(pSocketFH->iSocket, SIOSTATAT, pchArg, sizeof(struct statatreq) + 2);
            break;
        case __IOCLW(SIOSTATIF):
            rc = TCPNAME(imp_os2_ioctl)(pSocketFH->iSocket, SIOSTATIF, pchArg, sizeof(struct ifmib));
            break;
        case __IOCLW(SIOSTATIF42):
            /* What the h*** is the difference between the SIOSTATIF42 ioctl and SIOSTATIF?
               The docs doesn't make sense when looking in the headers... */
            rc = TCPNAME(imp_os2_ioctl)(pSocketFH->iSocket, SIOSTATIF42, pchArg, sizeof(struct ifmib));
            break;
        case __IOCLW(SIOSTATRT):
            rc = TCPNAME(imp_os2_ioctl)(pSocketFH->iSocket, SIOSTATRT, pchArg, sizeof(struct rtentries));
            break;
        case __IOCLW(SIOSTATSO):
            /** this isn't really suitable for this ioctl interface!! */
            rc = TCPNAME(imp_os2_ioctl)(pSocketFH->iSocket, SIOSTATSO, pchArg, sizeof(struct sockaddr));
            break;

        case __IOCLW(FIONBIO):
            rc = TCPNAME(imp_ioctl)(pSocketFH->iSocket, FIONBIO, pchArg);
            if (!rc)
            {
                unsigned fFlags = pSocketFH->core.fFlags;
                if (*(unsigned*)pchArg)
                    fFlags |= O_NONBLOCK;
                else
                    fFlags &= ~O_NONBLOCK;
                rc = __libc_FHSetFlags(pFH, fh, fFlags);
            }
            break;

        default:
            rc = TCPNAME(imp_ioctl)(pSocketFH->iSocket, iIOControl, pchArg);
            break;
    }

#else
    int     cb;

    /*
     * This ioctl interface requires a size, which thus is something we
     * must determin. :-/
     */
    switch (__IOCLW(iIOControl))
    {
        case __IOCLW(FIONREAD):
        case __IOCLW(FIOASYNC):
        case __IOCLW(SIOCATMARK):
        case __IOCLW(FIONBIO):
        case __IOCLW(SIOCSHIWAT):
        case __IOCLW(SIOCGHIWAT):
        case __IOCLW(SIOCSLOWAT):
        case __IOCLW(SIOCGLOWAT):
        case __IOCLW(SIOCSPGRP):
        case __IOCLW(SIOCGPGRP):
            cb = sizeof(int);
            break;

        case __IOCLW(SIOCSHOSTID):
        case __IOCLW(SIOCGNBNAME):
        case __IOCLW(SIOCSNBNAME):
        case __IOCLW(SIOCGNCBFN):
            cb = sizeof(long);
            break;

        case __IOCLW(FIONSTATUS):
            cb = sizeof(short) * 4;
            break;

        case __IOCLW(FIOBSTATUS):
            cb = sizeof(short);
            break;

        case __IOCLW(SIOCGIFADDR):
        case __IOCLW(SIOCGIFBRDADDR):
        case __IOCLW(SIOCGIFDSTADDR):
        case __IOCLW(SIOCGIFFLAGS):
        case __IOCLW(SIOCGIFMETRIC):
        case __IOCLW(SIOCGIFNETMASK):
        case __IOCLW(SIOCSIFADDR):
        case __IOCLW(SIOCSIFBRDADDR):
        case __IOCLW(SIOCSIFDSTADDR):
        case __IOCLW(SIOCSIFFLAGS):
        case __IOCLW(SIOCSIFMETRIC):
        case __IOCLW(SIOCSIFNETMASK):
        case __IOCLW(SIOCSIFALLRTB):
        case __IOCLW(SIOCSIF802_3):
        case __IOCLW(SIOCSIFNO802_3):
        case __IOCLW(SIOCSIFNOREDIR):
        case __IOCLW(SIOCSIFYESREDIR):
        case __IOCLW(SIOCSIFMTU):
        case __IOCLW(SIOCSIFFDDI):
        case __IOCLW(SIOCSIFNOFDDI):
        case __IOCLW(SIOCSRDBRD):
        case __IOCLW(SIOCADDMULTI):
        case __IOCLW(SIOCDELMULTI):
        case __IOCLW(SIOCMULTISBC):
        case __IOCLW(SIOCMULTISFA):
        case __IOCLW(SIOCDIFADDR):
        case __IOCLW(SIOCGUNIT):
        case __IOCLW(SIOCGIFVALID):
        case __IOCLW(SIOCGIFEFLAGS):
        case __IOCLW(SIOCSIFEFLAGS):
        case __IOCLW(SIOCGIFTRACE):
        case __IOCLW(SIOCSIFTRACE):
        case __IOCLW(SIOCSSTAT):
        case __IOCLW(SIOCGSTAT):
            cb = sizeof(struct ifreq);
            break;

        case __IOCLW(SIOCADDRT):
        case __IOCLW(SIOCDELRT):
            cb = sizeof(struct rtentry);
            break;

        case __IOCLW(SIOCSARP):
        case __IOCLW(SIOCGARP):
        case __IOCLW(SIOCDARP):
            cb = sizeof(struct arpreq);
            break;

        case __IOCLW(SIOCGIFCONF):
            cb = sizeof(struct ifconf);
            break;

        case __IOCLW(SIOCSARP_TR):
        case __IOCLW(SIOCGARP_TR):
            cb = sizeof(struct arpreq_tr);
            break;

        /* duplicate! Bug in BSD 4.3 stack I'd say.
        case __IOCLW(SIOCAIFADDR):
            cb = sizeof(struct ifaliasreq);
            break;
         */

        case __IOCLW(SIOCGIFBOUND):
            cb = sizeof(struct bndreq);
            break;

        case __IOCLW(SIOCGMCAST):
            cb = sizeof(struct addrreq);
            break;

        case __IOCLW(SIOSTATMBUF):
            cb = sizeof(struct mbstat);
            break;

        case __IOCLW(SIOSTATTCP):
            cb = sizeof(struct tcpstat);
            break;

        case __IOCLW(SIOSTATUDP):
            cb = sizeof(struct udpstat);
            break;

        case __IOCLW(SIOSTATIP):
            cb = sizeof(struct ipstat);
            break;

        case __IOCLW(SIOSTATSO):
            cb = sizeof(struct sockaddr);
            break;

        case __IOCLW(SIOSTATRT):
            cb = sizeof(struct rtentries);
            break;

        case __IOCLW(SIOFLUSHRT):
            cb = sizeof(long);
            break;

        case __IOCLW(SIOSTATICMP):
            cb = sizeof(struct icmpstat);
            break;

        case __IOCLW(SIOSTATIF):
            cb = sizeof(struct ifmib);
            break;

        case __IOCLW(SIOSTATAT):
            /** this isn't really suitable for this ioctl interface!! */
            cb = sizeof(struct statatreq) + 2;
            break;

        case __IOCLW(SIOSTATARP):
            cb = sizeof(struct oarptab);
            break;

        case __IOCLW(SIOSTATIF42):
            cb = sizeof(struct ifmib); /* docs are unclear here */
            break;

        case __IOCLW(SIOSTATCNTRT):
            cb = sizeof(int);
            break;

        case __IOCLW(SIOSTATCNTAT):
            cb = sizeof(int);
            break;

        default:
            cb = -1;
            break;

/** @todo There is a bunch of IOCtls in the BSD 4.3 stack
 *        for which I don't have the docs and don't know exact
 *        what sizes to use:
 *               FIOTCPCKSUM     ioc('f', 128)
 *               FIONURG         ioc('f', 121)
 *               SIOMETRIC1RT    ioc('r', 12)
 *               SIOMETRIC2RT    ioc('r', 13)
 *               SIOMETRIC3RT    ioc('r', 14)
 *               SIOMETRIC4RT    ioc('r', 15)
 *               SIOCREGADDNET   ioc('r', 12)
 *               SIOCREGDELNET   ioc('r', 13)
 *               SIOCREGROUTES   ioc('r', 14)
 *               SIOCFLUSHROUTES ioc('r', 15)
 *               SIOCSIFSETSIG   ioc('i', 25)
 *               SIOCSIFCLRSIG   ioc('i', 26)
 *               SIOCSIFBRD      ioc('i', 27)
 *               SIOCGIFLOAD     ioc('i', 27)
 *               SIOCSIFFILTERSRC ioc('i', 28)
 *               SIOCGIFFILTERSRC ioc('i',29)
 *               SIOCSIFSNMPSIG  ioc('i', 33)
 *               SIOCSIFSNMPCLR  ioc('i', 34)
 *               SIOCSIFSNMPCRC  ioc('i', 35)
 *               SIOCSIFPRIORITY ioc('i', 36)
 *               SIOCGIFPRIORITY ioc('i', 37)
 *               SIOCSIFFILTERDST ioc('i', 38)
 *               SIOCGIFFILTERDST ioc('i',39)
 *               SIOCSIFSPIPE     ioc('i',71)
 *               SIOCSIFRPIPE     ioc('i',72)
 *               SIOCSIFTCPSEG   ioc('i',73)
 *               SIOCSIFUSE576   ioc('i',74)
 */
    } /* size switch */

    /*
     * Do the call.
     */
    if (cb >= 0)
    {
        switch (__IOCLW(iIOControl))
        {
            case FIONBIO:
                rc = TCPNAME(imp_os2_ioctl)(pSocketFH->iSocket, iIOControl, pchArg, cb);
                if (rc)
                {
                    if (*(unsigned*)pchArg)
                        pSocketFH->core.fFlags |= O_NDELAY;
                    else
                        pSocketFH->core.fFlags &= ~O_NDELAY;
                }
                break;

            default:
                rc = TCPNAME(imp_os2_ioctl)(pSocketFH->iSocket, iIOControl, pchArg, cb);
                break;
        }
    }
    else
    {   /*  */
        *prc = -1;
        LIBCLOG_ERROR_RETURN_INT(-ENOSYS);
    }

#endif

    *prc = rc;
    if (rc < 0)
    {
        rc = -__libc_TcpipGetSocketErrno();
        LIBCLOG_ERROR_RETURN_INT(rc);
    }
    LIBCLOG_RETURN_INT(0);
}


/** Select operation.
 * The select operation is only performed if all handles have the same
 * select routine (the main worker checks this).
 *
 * @returns 0 on success.
 * @returns OS/2 error code or negated errno on failure.
 * @param   cFHs        Range of handles to be tested.
 * @param   pRead       Bitmap for file handles to wait upon to become ready for reading.
 * @param   pWrite      Bitmap for file handles to wait upon to become ready for writing.
 * @param   pExcept     Bitmap of file handles to wait on (error) exceptions from.
 * @param   tv          Timeout value.
 * @param   prc         Where to store the value which upon success is
 *                      returned to the caller.
 */
static int TCPNAME(ops_Select)(int cFHs, struct fd_set *pRead, struct fd_set *pWrite, struct fd_set *pExcept, struct timeval *tv, int *prc)
{
    LIBCLOG_ENTER("cFHs=%d pRead=%p pWrite=%p pExcept=%p tv=%p prc=%p\n",
                  cFHs, (void *)pRead, (void *)pWrite, (void *)pExcept, (void *)tv, (void *)prc);
    int rc;
    int saved_errno = errno;
    rc = TCPNAMEG(bsdselect)(cFHs, pRead, pWrite, pExcept, tv);
    *prc = rc;
    rc = rc >= 0 ? 0 : -errno;
    errno = saved_errno;
    if (rc >= 0)
        LIBCLOG_RETURN_INT(rc);
    LIBCLOG_ERROR_RETURN_INT(rc);
}


/** Fork notification - child context.
 * Only the __LIBC_FORK_OP_FORK_CHILD operation is forwarded atm.
 * If NULL it's assumed that no notifiction is needed.
 *
 * @returns 0 on success.
 * @returns OS/2 error code or negated errno on failure.
 * @param   pFH             Pointer to the handle structure to operate on.
 * @param   fh              It's associated filehandle.
 * @param   pForkHandle     The fork handle.
 * @param   enmOperation    The fork operation.
 */
static int TCPNAME(ops_ForkChild)(struct __libc_FileHandle *pFH, int fh, __LIBC_PFORKHANDLE pForkHandle, __LIBC_FORKOP enmOperation)
{
    LIBCLOG_ENTER("pFH=%p:{.iSocket=%d} fh=%d\n", (void *)pFH, ((PLIBCSOCKETFH)pFH)->iSocket, fh);
    PLIBCSOCKETFH   pSocketFH = (PLIBCSOCKETFH)pFH;
    int rc = __libc_spmSocketRef(pSocketFH->iSocket);
    LIBC_ASSERTM(rc > 0, "__libc_spmSocketRef(%d) -> rc=%d\n", pSocketFH->iSocket, rc);
    if ((rc >> 16) == 1)
        TCPNAME(imp_addsockettolist)(pSocketFH->iSocket);
    LIBCLOG_RETURN_INT(0);
    rc = rc;
}





/**
 * Allocates file handle for a socket.
 *
 * @returns Pointer to socket handle on success.
 * @returns NULL and errno on failure.
 * @param   iSocket     The socket to allocate handle for.
 * @param   pfh         Where to store the filename.
 */
PLIBCSOCKETFH TCPNAMEG(AllocFH)(int iSocket, int *pfh)
{
    LIBCLOG_ENTER("iSocket=%d\n", iSocket);
    PLIBCSOCKETFH pFH;
    int rc = TCPNAMEG(AllocFHEx)(-1, iSocket, O_RDWR | F_SOCKET, 1, pfh, &pFH);
    if (!rc)
        LIBCLOG_RETURN_P(pFH);
    errno = -rc;
    LIBCLOG_ERROR_RETURN_P(NULL);
}


/**
 * Allocates file handle for a socket - extended version.
 *
 * The socket is not removed from the socket list when calling this
 * allocation function. This is because it's used by the inherit
 * processor where such actions are unnecessary.
 *
 * @returns 0 on success.
 * @returns Negative error code (errno.h) on failure.
 * @param   fh          The requested file handle.
 *                      If -1 any file handle can be used.
 * @param   iSocket     The socket to allocate handle for.
 * @param   fFlags      LIBC file handle flags.
 * @param   fNew        If set we handle the socket as brand new setting the global
 *                      reference counter to 1 and remove it from the TCPIP cleanup
 *                      list for the current process.
 *                      If clear we only increment the global reference counter.
 * @param   pfh         Where to store the file handle.
 * @param   ppFHSocket  Where to store the allocated socket handle. Optional.
 */
int TCPNAMEG(AllocFHEx)(int fh, int iSocket, unsigned fFlags, int fNew, int *pfh, PLIBCSOCKETFH *ppFHSocket)
{
    LIBCLOG_ENTER("fh=%d iSocket=%d fFlags=%#x fNew=%d pfh=%p:{%d} ppFHSocket=%p\n", fh, iSocket, fFlags, fNew, (void *)pfh, pfh ? *pfh : -2, (void *)ppFHSocket);
    PLIBCFH         pFHLibc;
    int rc = __libc_FHAllocate(fh, fFlags, sizeof(LIBCSOCKETFH), &gSocketOps, &pFHLibc, pfh);
    if (!rc)
    {
        /*
         * Init the handle.
         */
        PLIBCSOCKETFH pFH = (PLIBCSOCKETFH)pFHLibc;
        pFH->iSocket = iSocket;

        /*
         * Insert into the list.
         */
        pFH->pPrev = NULL;
        _smutex_request(&gsmtxSockets);
        pFH->pNext = gpSocketsHead;
        if (pFH->pNext)
            gpSocketsHead->pPrev = pFH;
        gpSocketsHead = pFH;
        _smutex_release(&gsmtxSockets);

        /*
         * New: Set usage counter to 1.
         * Existing: Increment reference counter and add to socket list.
         */
        if (fNew)
        {
            rc = __libc_spmSocketNew(iSocket);
            LIBC_ASSERTM(!rc, "__libc_spmSocketNew(%d) -> rc=%d\n", iSocket, rc);
        }
        else
        {
            rc = __libc_spmSocketRef(iSocket);
            LIBC_ASSERTM(rc > 0, "__libc_spmSocketRef(%d) -> rc=%d\n", iSocket, rc);
            if ((rc >> 16) == 1) /* only first reference in this process */
                TCPNAME(imp_addsockettolist)(iSocket);
        }
        if (ppFHSocket)
            *ppFHSocket = pFH;
        LIBCLOG_RETURN_MSG(0, "ret 0 *pfh=%d *ppFHSocket=%p\n", pfh ? *pfh : -2, pFH);
    }
    LIBCLOG_ERROR_RETURN_P(rc);
}



/**
 * Calculate the size required for the converted set structure.
 * @returns number of bytes.
 * @param   c       Number of file descriptors specified in the call.
 * @param   pFrom   The select input fd_set structure.
 * @parm    pcFDs   Store the new number of file descriptors (socket handles) to examin.
 */
static inline int calcsize(int c, const struct fd_set *pFrom, int *pcFDs)
{
    int cbRet;
    int i;
#ifdef TCPV40HDRS
    /* here we need to figure out the max real socket handle */
    int iMax;
    for (iMax = *pcFDs - 1, i = 0; i < c; i++)
        if (FD_ISSET(i, pFrom))
        {
            PLIBCSOCKETFH   pFHSocket = __libc_TcpipFH(i);
            if (pFHSocket && iMax < pFHSocket->iSocket)
                iMax = pFHSocket->iSocket;
        }
    cbRet = (iMax + 8) / 8;             /* iMax is index not count, thus +8 not +7. */
    *pcFDs = iMax + 1;
#else
    for (i = cbRet = 0; i < c; i++)
        if (FD_ISSET(i, pFrom))
            cbRet++;
    if (*pcFDs < cbRet + 1)
        *pcFDs = cbRet + 1;
    cbRet *= sizeof(int);
    cbRet += offsetof(my_fd_set, fd_array);
#endif
    if (cbRet < sizeof(struct my_fd_set))
        return sizeof(struct my_fd_set);
    return cbRet;
}

/** Converts the LIBC fd_set strcture pointed to by pFrom with it's LIBC socket handles,
 * to the fd_set strcuture used by the OS/2 tcpip and the OS/2 socket handles.
 * @returns 0 on success.
 * @returns -1 on failure, both errnos set.
 * @param   c       Number of file descriptors specified in the call.
 * @param   cb      Size of pTo. (used for zeroing it)
 * @param   pFrom   The select input fd_set structure.
 * @param   pTo     The structure we present to OS/2 TCPIP.
 *                  This will be initialized.
 * @param   pszType Typestring to use in the log.
 */
static inline int convert(int c, int cb, const struct fd_set *pFrom, struct my_fd_set *pTo, const char *pszType)
{
    int i;
    MY_FD_ZERO(pTo, cb)
    for (i = 0; i < c; i++)
    {
        if (FD_ISSET(i, pFrom))
        {
            PLIBCSOCKETFH   pFHSocket = __libc_TcpipFH(i);
            if (!pFHSocket)
            {
                LIBCLOG_MSG2("Invalid handle %d specified (%s).\n", i, pszType);
                return -1;
            }
            MY_FD_SET(pFHSocket->iSocket, pTo);
            LIBCLOG_MSG2("%s: %02d -> %03d\n", pszType, i, pFHSocket->iSocket);
        }
    }
    pszType = pszType;
    return 0;
}

/**
 * Updates the pTo structure with the fds marked ready in pFrom.
 *
 * @param   c       Number of file descriptors specified in the call.
 * @param   pFrom   The structure returned from OS/2 TCPIP select.
 * @param   pTo     The structure passed in to select which have to
 *                  be updated for the return.
 * @param   pszType Typestring to use in the log.
 */
static inline void update(int c, const struct my_fd_set *pFrom, struct fd_set *pTo, const char *pszType)
{
    int i;
    for (i = 0; i < c; i++)
    {
        if (FD_ISSET(i, pTo))
        {
            PLIBCSOCKETFH   pFHSocket = __libc_TcpipFH(i);
            if (pFHSocket)
            {
                if (!MY_FD_ISSET(pFHSocket->iSocket, pFrom))
                {
                    FD_CLR(i, pTo);
                    LIBCLOG_MSG2("%s: %02d -> %03d set\n", pszType, i, pFHSocket->iSocket);
                }
                else
                    LIBCLOG_MSG2("%s: %02d -> %03d clear\n", pszType, i, pFHSocket->iSocket);
            }
        }
    }
    pszType = pszType;
}



int TCPNAMEG(bsdselect)(int nfds, struct fd_set *readfds, struct fd_set *writefds, struct fd_set *exceptfds, struct timeval *tv)
{
    LIBCLOG_ENTER("nfds=%d readfds=%p writefds=%p exceptfds=%p tv=%p={tv_sec=%d, tv_usec=%ld}\n",
                  nfds, (void *)readfds, (void *)writefds, (void *)exceptfds, (void *)tv, tv ? tv->tv_sec : -1, tv ? tv->tv_usec : -1);
#ifdef TCPV40HDRS
    struct fd_set      *pRead;
    struct fd_set      *pWrite;
    struct fd_set      *pExcept;
#else
    struct v5_fd_set   *pRead;
    struct v5_fd_set   *pWrite;
    struct v5_fd_set   *pExcept;
#endif
    int                 rc;
    int                 cb = 0;
    int                 cFDs = 0;

    /*
     * Calculate bitmapsize.
     */
    if (readfds)
        cb = calcsize(nfds, readfds, &cFDs);
    if (writefds)
    {
        int cbT = calcsize(nfds, writefds, &cFDs);
        if (cbT > cb)
            cb = cbT;
    }
    if (exceptfds)
    {
        int cbT = calcsize(nfds, exceptfds, &cFDs);
        if (cbT > cb)
            cb = cbT;
    }

    /*
     * Allocate new bitmaps.
     */
    pRead = NULL;
    if (readfds)
    {
        pRead = alloca(cb);
        if (!pRead)
        {
            __libc_TcpipSetErrno(ENOMEM);
            LIBCLOG_ERROR_RETURN_INT(-1);
        }
    }

    pWrite = NULL;
    if (writefds)
    {
        pWrite = alloca(cb);
        if (!pWrite)
        {
            __libc_TcpipSetErrno(ENOMEM);
            LIBCLOG_ERROR_RETURN_INT(-1);
        }
    }

    pExcept = NULL;
    if (exceptfds)
    {
        pExcept = alloca(cb);
        if (!pExcept)
        {
            __libc_TcpipSetErrno(ENOMEM);
            LIBCLOG_ERROR_RETURN_INT(-1);
        }
    }

    /*
     * Convert the bitmaps.
     */
    if (readfds)
    {
        if (convert(nfds, cb, readfds, pRead, "read "))
            LIBCLOG_ERROR_RETURN_INT(-1);
    }

    if (writefds)
    {
        if (convert(nfds, cb, writefds, pWrite, "write"))
            LIBCLOG_ERROR_RETURN_INT(-1);
    }

    if (exceptfds)
    {
        if (convert(nfds, cb, exceptfds, pExcept, "excpt"))
            LIBCLOG_ERROR_RETURN_INT(-1);
    }

    /*
     * Do the call.
     */
    LIBCLOG_MSG("calling native: cFDs=%d pRead=%p pWrite=%p, pExcept=%p tv=%p\n",
                cFDs, (void *)pRead, (void *)pWrite, (void *)pExcept, (void *)tv);
    __LIBC_PTHREAD pThrd = __libc_threadCurrent();
    pThrd->ulSigLastTS = 0;
    rc = TCPNAME(imp_bsdselect)(cFDs, pRead, pWrite, pExcept, tv);
    if (rc < 0)
    {
        __libc_TcpipUpdateErrno();
        LIBCLOG_ERROR_RETURN_INT(rc);
    }
    else if (!rc && pThrd->ulSigLastTS)      /* detect interruption */
    {
        __libc_TcpipSetErrno(EINTR);
        LIBCLOG_ERROR_RETURN_INT(-1);
    }

    /*
     * Timeout?
     *  Just clear the bitmaps and return.
     */
    if (rc == 0)
    {
        cb = (nfds + 7) / 8;
        if (readfds)
            bzero(readfds, cb);
        if (writefds)
            bzero(writefds, cb);
        if (exceptfds)
            bzero(exceptfds, cb);
        LIBCLOG_ERROR_RETURN_INT(0);
    }

    /*
     * Convert the bitmaps.
     */
    if (readfds)
        update(nfds, pRead, readfds, "read ");
    if (writefds)
        update(nfds, pWrite, writefds, "write");
    if (exceptfds)
        update(nfds, pExcept, exceptfds, "excpt");

    if (rc >= 0)
        LIBCLOG_RETURN_INT(rc);
    LIBCLOG_ERROR_RETURN_INT(rc);
}



//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//
//
//          DYNAMIC IMPORTS
//
//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//
#ifdef TCPV40HDRS
# define ORD_SOCK_ERRNO                 20
# define ORD_SET_ERRNO                  35
# define ORD_SOCLOSE                    17
# define ORD_SHUTDOWN                   25
# define ORD_IOCTL                      8
# define ORD_OS2_IOCTL                  8
# define ORD_RECV                       10
# define ORD_SEND                       13
# define ORD_ADDSOCKETTOLIST            27
# define ORD_REMOVESOCKETFROMLIST       28
# define ORD_BSDSELECT                  32
#else
# define ORD_SOCK_ERRNO                 20
# define ORD_SET_ERRNO                  35
# define ORD_SOCLOSE                    17
# define ORD_SHUTDOWN                   25
# define ORD_IOCTL                      8
# define ORD_OS2_IOCTL                  200
# define ORD_RECV                       10
# define ORD_SEND                       13
# define ORD_ADDSOCKETTOLIST            27
# define ORD_REMOVESOCKETFROMLIST       28
# define ORD_BSDSELECT                  32
#endif

/** Handle of the socket dll. */
static HMODULE      ghmod;
/** The name of the DLL. */
#ifdef TCPV40HDRS
static const char   gszDllName[] = "SO32DLL";
#else
static const char   gszDllName[] = "TCPIP32";
#endif


/**
 * Resolves an external symbol in a tcpip dll.
 */
static int  TCPNAME(get_imp)(unsigned iOrdinal, void **ppfn)
{
    LIBCLOG_ENTER("iOrdinal=%d ppfn=%p\n", iOrdinal, (void *)ppfn);
    int     rc;
    PFN     pfn;
    if (!ghmod)
    {

        HMODULE hmod;
        char    szErr[16];
        rc = DosLoadModuleEx((PSZ)szErr, sizeof(szErr), (PCSZ)gszDllName, &hmod);
        if (rc)
        {
            errno = ENOSYS;
            LIBCLOG_ERROR_RETURN(-1, "ret -1 - DosLoadModule(,,%s,) failed. rc=%d szErr=%.16s\n", gszDllName, rc, szErr);
        }
        __atomic_xchg((unsigned *)(void *)&ghmod, (unsigned)hmod);
    }

    /*
     * Resolve the symbol.
     */
    rc = DosQueryProcAddr(ghmod, iOrdinal, NULL, &pfn);
    if (rc)
    {
        errno = ENOSYS;
        LIBCLOG_ERROR_RETURN(-1, "ret -1 - DosQueryProcAddr(%#lx,%d,,) failed. rc=%d\n", ghmod, iOrdinal, rc);
    }

    __atomic_xchg((unsigned *)ppfn, (unsigned)*pfn);
    LIBCLOG_RETURN_MSG(0, "ret 0 - *ppfn=%p\n", (void *)pfn);
}

int     TCPNAME(imp_sock_errno)(void)
{
    static int (*TCPCALL pfn)(void);
    if (!pfn && TCPNAME(get_imp)(ORD_SOCK_ERRNO, (void **)(void *)&pfn))
        return errno;
    return pfn();
}

void    TCPNAME(imp_set_errno)(int iErr)
{
    static int (*TCPCALL pfn)(int iErr);
    if (!pfn && TCPNAME(get_imp)(ORD_SET_ERRNO, (void **)(void *)&pfn))
        return;
    pfn(iErr);
}

static int     TCPNAME(imp_soclose)(int s)
{
    LIBCLOG_ENTER("iSocket=%d\n", s);
    static int (*TCPCALL pfn)(int s);
    if (!pfn && TCPNAME(get_imp)(ORD_SOCLOSE, (void **)(void *)&pfn))
        LIBCLOG_ERROR_RETURN_INT(-1);
    int rc = pfn(s);
    LIBCLOG_MIX_RETURN_INT(rc);
}

int     TCPNAME(imp_shutdown)(int s, int howto)
{
    LIBCLOG_ENTER("iSocket=%d\n", s);
    static int (*TCPCALL pfn)(int s, int howto);
    if (!pfn && TCPNAME(get_imp)(ORD_SHUTDOWN, (void **)(void *)&pfn))
        LIBCLOG_ERROR_RETURN_INT(-1);
    int rc = pfn(s, howto);
    LIBCLOG_MIX_RETURN_INT(rc);
}

#ifndef TCPV40HDRS
static int     TCPNAME(imp_ioctl)(int s, int req, char *arg)
{
    static int (*TCPCALL pfn)(int s, int req, char *arg);
    if (!pfn && TCPNAME(get_imp)(ORD_IOCTL, (void **)(void *)&pfn))
        return -1;
    return pfn(s, req, arg);
}
#endif

static int     TCPNAME(imp_os2_ioctl)(int s, unsigned long req, char *arg, int len)
{
    static int (*TCPCALL pfn)(int s, unsigned long req, char *arg, int len);
    if (!pfn && TCPNAME(get_imp)(ORD_OS2_IOCTL, (void **)(void *)&pfn))
        return -1;
    return pfn(s,req, arg, len);
}

static int     TCPNAME(imp_recv)(int s, void *buf, int len, int flags)
{
    static int (*TCPCALL pfn)(int s, void *buf, int len, int flags);
    if (!pfn && TCPNAME(get_imp)(ORD_RECV, (void *)(void **)(void *)&pfn))
        return -1;
    return pfn(s, buf, len, flags);
}

static int     TCPNAME(imp_send)(int s, const void *buf, int len, int flags)
{
    static int (*TCPCALL pfn)(int s, const void *buf, int len, int flags);
    if (!pfn && TCPNAME(get_imp)(ORD_SEND, (void **)(void *)&pfn))
        return -1;
    return pfn(s, buf, len, flags);
}

static void     TCPNAME(imp_addsockettolist)(int s)
{
    LIBCLOG_ENTER("iSocket=%d\n", s);
    static void (*TCPCALL pfn)(int s);
    if (!pfn && TCPNAME(get_imp)(ORD_ADDSOCKETTOLIST, (void **)(void *)&pfn))
        LIBCLOG_ERROR_RETURN_VOID();
    pfn(s);
    LIBCLOG_RETURN_VOID();
}

static _Bool    TCPNAME(imp_removesocketfromlist)(int s)
{
    LIBCLOG_ENTER("iSocket=%d\n", s);
    static int (*TCPCALL pfn)(int s);
    if (!pfn && TCPNAME(get_imp)(ORD_REMOVESOCKETFROMLIST, (void **)(void *)&pfn))
        LIBCLOG_ERROR_RETURN_INT(0);
    int rc = pfn(s);
    LIBCLOG_RETURN_INT(!!rc); /* paranoia!!!! */
}

#ifdef TCPV40HDRS
static int     TCPNAME(imp_bsdselect)(int nfds, fd_set *r, fd_set *w, fd_set *x, struct timeval *tv)
{
    static int (*TCPCALL pfn)(int nfds, fd_set *r, fd_set *w, fd_set *x, struct timeval *tv);
#else
static int     TCPNAME(imp_bsdselect)(int nfds, v5_fd_set *r, v5_fd_set *w, v5_fd_set *x, struct timeval *tv)
{
    static int (*TCPCALL pfn)(int nfds, v5_fd_set *r, v5_fd_set *w, v5_fd_set *x, struct timeval *tv);
#endif
    if (!pfn && TCPNAME(get_imp)(ORD_BSDSELECT, (void **)(void *)&pfn))
        return -1;
    return pfn(nfds, r, w, x, tv);
}



#undef  __LIBC_LOG_GROUP
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_INITTERM
_CRT_INIT1(tcpipInit)

/**
 * Init function which registers the tcpip cleanup handler with the exit list.
 */
CRT_DATA_USED
static void tcpipInit(void)
{
    LIBCLOG_ENTER("\n");
    static void *pfn = (void*)tcpipInit; /* reference hack */
    pfn = pfn;
    __libc_spmRegTerm(TCPNAME(Cleanup));
    LIBCLOG_RETURN_VOID();
}


/**
 * This function will close all open sockets updating
 * the global reference counters.
 *
 * The SPM exit list handler calls it.
 */
static void TCPNAME(Cleanup)(void)
{
    LIBCLOG_ENTER("\n");
    PLIBCSOCKETFH   pFH;

    /*
     * Walk list of socket handles and dereference them.
     */
    pFH = gpSocketsHead;
    while (pFH)
    {
        int rc = __libc_spmSocketDeref(pFH->iSocket);
        LIBC_ASSERTM(rc >= 0, "__libc_spmSocketDeref(%d) -> rc=%d\n", pFH->iSocket, rc);
        if (rc < 0)
            rc = 0; /* we'd rather have failures and fix the problem than stale sockets and bugs. */
        if (!rc)
        {
            /*
             * Close the socket, noone needs it.
             */
            rc = TCPNAME(imp_soclose)(pFH->iSocket);
            LIBC_ASSERTM(!rc, "soclose(%d) -> rc=%d sock_errno()->%d\n", pFH->iSocket, rc, TCPNAME(imp_sock_errno)());
        }
        else if (rc >> 16) /* (high word is process reference count) */
            /* The socket is duplicated within the current process, it will be close later when the other handle(s) is(/are) closed. */
            rc = 0;
        else
        {
            /*
             * If in use by other process, we'll remove it from the list of sockets
             * owned by this process to prevent it from begin closed.
             *
             * Please note that removesocketfromlist is returning a boolean success indicator and probably no errno.
             */
            TCPNAME(imp_set_errno)(0);
            rc = TCPNAME(imp_removesocketfromlist)(pFH->iSocket);
            /*
             * @todo removesocketfromlist fails in some cases (e.g. when the child inheritance chain
             * is too long and complex). This might be a race in TCPIP32.DLL. Do not assert for now.
             */
#if 0
            LIBC_ASSERTM(rc == 1, "removesocketfromlist(%d) -> rc=%d sock_errno()->%d\n", pFH->iSocket, rc, TCPNAME(imp_sock_errno)());
#else
            if (!rc)
                LIBCLOG_MSG("removesocketfromlist(%d) -> false! sock_errno()->%d\n", pFH->iSocket, TCPNAME(imp_sock_errno)());
#endif
        }

        /* next */
        pFH = pFH->pNext;
    }
    LIBCLOG_RETURN_VOID();
}

