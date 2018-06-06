/* $Id: tcpip.h 3809 2014-02-16 20:20:59Z bird $ */
/** @file
 *
 * Semi Private TCP/IP header.
 *
 * Copyright (c) 2003-2004 knut st. osmundsen <bird-srcspam@anduin.net>
 *
 *
 * This file is part of Innotek LIBC.
 *
 * Innotek LIBC is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Innotek LIBC is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Innotek LIBC; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef __InnoTekLIBC_tcpip_h__
#define __InnoTekLIBC_tcpip_h__

#include <errno.h>
#include <sys/socket.h>
#include <emx/io.h>


/** @group grp_libc_tcpip       LIBC TCP/IP Backend
 * @{
 */

/*******************************************************************************
*   Defined Constants And Macros                                               *
*******************************************************************************/
/** @def TCPNAME44
 * Name construction macro for BSD 4.4 specific function.
 */
#define TCPNAME44(a)    __libc_tcpip##a##44
/** @def TCPNAME43
 * Name construction macro for BSD 4.3 specific function.
 */
#define TCPNAME43(a)    __libc_tcpip##a##43
/** @def TCPNAME
 * Name constructure macro for the current BSD mode.
 * TCPV40HDRS is used to figure out which mode we're in.
 */
#ifdef TCPV40HDRS
# define TCPNAME(a)     TCPNAME43(a)
#else
# define TCPNAME(a)     TCPNAME44(a)
#endif

/** @def TCPNAMEG44
 * Name construction macro for BSD 4.4 specific function.
 */
#define TCPNAMEG44(a)   __libc_Tcpip##a##44
/** @def TCPNAMEG43
 * Name construction macro for BSD 4.3 specific function.
 */
#define TCPNAMEG43(a)   __libc_Tcpip##a##43
/** @def TCPNAMEG
 * Name constructure macro for the current BSD mode.
 * TCPV40HDRS is used to figure out which mode we're in.
 */
#ifdef TCPV40HDRS
# define TCPNAMEG(a)    TCPNAMEG43(a)
#else
# define TCPNAMEG(a)    TCPNAMEG44(a)
#endif


/** The offset the OS/2 TCP/IP errno values are skewed compared to the
 * LIBC errno values. */
#define EOS2_TCPIP_OFFSET   10000

/** Get the low word of the ioctl request number.
 * Used to support ioctl request numbers from old and new _IOC macros.
 */
#define __IOCLW(a) ((unsigned short)(a))


#ifndef TCPV40HDRS
#define V5_FD_SETSIZE      64
#define V5_FD_SET(fd, set) do { \
   /* if (((v5_fd_set *)(set))->fd_count < V5_FD_SETSIZE) - calcsize fixes this */ \
        ((v5_fd_set *)(set))->fd_array[((v5_fd_set *)(set))->fd_count++]=fd;\
} while(0)
#define V5_FD_ISSET(fd, set)    v5_isset(fd, set)
#endif



/*******************************************************************************
*   Structures and Typedefs                                                    *
*******************************************************************************/
/** Socket handle.*/
typedef struct __libc_SocketHandle
{
    /** the common fh core. */
    LIBCFH      core;

    /** OS/2 socket number. */
    int         iSocket;
    /** Pointer to next socket handle in the chain. */
    struct __libc_SocketHandle * volatile pNext;
    /** Pointer to previous socket handle in the chain. */
    struct __libc_SocketHandle * volatile pPrev;
} LIBCSOCKETFH, *PLIBCSOCKETFH;


#ifndef TCPV40HDRS

#pragma pack(4)
/** OS/2 oddities from the BSD 4.4 stack. */
typedef struct v5_fd_set {
        u_short fd_count;               /* how many are SET? */
        int     fd_array[V5_FD_SETSIZE];/* an array of SOCKETs */
} v5_fd_set;
#pragma pack()

/** internal helper */
static inline int v5_isset(int fd, const struct v5_fd_set *set)
{
    const int  *pfd = &set->fd_array[0];
    int         c   = set->fd_count;
    while (c > 0)
    {
        if (*pfd == fd)
            return 1;
        pfd++;
        c--;
    }
    return 0;
}

#endif


/*******************************************************************************
*   Internal Functions                                                         *
*******************************************************************************/
/** @defgroup libsocket_renamed     Renamed Imports.
 * @{ */
int     _System __libsocket_accept(int, struct sockaddr *, int *);
int     _System __libsocket_bind(int, const struct sockaddr *, int);
int     _System __libsocket_connect(int, const struct sockaddr *, int);
int     _System __libsocket_getpeername(int, struct sockaddr *, int *);
int     _System __libsocket_getsockname(int, struct sockaddr *, int *);
int     _System __libsocket_getsockopt(int, int, int, void *, int *);
int     _System __libsocket_ioctl(int, int, char *);
int     _System __libsocket_listen(int, int);
int     _System __libsocket_os2_ioctl(int, unsigned long, char *, int);
int     _System __libsocket_os2_select(int *, int, int, int, long);
int     _System __libsocket_recv(int, void *, int, int);
int     _System __libsocket_recvfrom(int, void *, int, int, struct sockaddr *, int *);
int     _System __libsocket_recvmsg(int, struct msghdr *, int);
int     _System __libsocket_send(int, const void *, int, int);
int     _System __libsocket_sendmsg(int, const struct msghdr *, int);
int     _System __libsocket_sendto(int, const void *, int, int, const struct sockaddr *, int);
int     _System __libsocket_setsockopt(int, int, int, const void *, int);
int     _System __libsocket_shutdown(int, int);
int     _System __libsocket_sock_errno(void);
int     _System __libsocket_socket(int, int, int);
int     _System __libsocket_socketpair(int, int, int, int *);
int     _System __libsocket_soclose(int);
int     _System __libsocket_accept_and_recv(long, long*, struct sockaddr *, long*, struct sockaddr*, long*, caddr_t, size_t);
void    _System __libsocket_addsockettolist(int);
int     _System __libsocket_removesocketfromlist(int);
ssize_t _System __libsocket_so_readv(int, struct iovec *, int);
ssize_t _System __libsocket_so_writev(int, struct iovec *, int);
int     _System __libsocket_so_cancel(int);
int     _System __libsocket_soabort(int);
int     _System __libsocket_Raccept(int, struct sockaddr *, int *);
struct sockaddr_in;
int     _System __libsocket_tcpip4_Rbind(int, struct sockaddr_in *, int, struct sockaddr_in *);
int     _System __libsocket_Rbind(int, struct sockaddr *, int, struct sockaddr *);
int     _System __libsocket_Rconnect(int, const struct sockaddr *, int);
int     _System __libsocket_Rgetsockname(int, struct sockaddr *, int *);
int     _System __libsocket_Rlisten(int, int);
#ifndef TCPV40HDRS
ssize_t _System __libsocket_send_file(int *, struct sf_parms *, int );
int     _System __libsocket_socketpair(int af, int type, int flags, int *osfd);
#endif
char *  _System __libsocket_sock_strerror(int);
#ifdef TCPV40HDRS
int     _System __libsocket_bsdselect(int, fd_set *, fd_set *, fd_set *, struct timeval *);
#else
int     _System __libsocket_bsdselect(int, v5_fd_set *, v5_fd_set *, v5_fd_set *, struct timeval *);
#endif
void    _System __libsocket_set_errno(int);
/** @} */


/** @defgroup grp_libc_tcpip_internal    Internal helpers.
 * @{ */

/**
 * BSD Select worker.
 */
int TCPNAMEG(bsdselect)(int nfds, struct fd_set *readfds, struct fd_set *writefds, struct fd_set *exceptfds, struct timeval *tv);


/**
 * Allocates file handle for a BSD 4.4 socket.
 *
 * @returns Pointer to socket handle on success.
 * @returns NULL and errno on failure.
 * @param   iSocket     The socket to allocate handle for.
 * @param   pfh         Where to store the filename.
 */
PLIBCSOCKETFH TCPNAMEG44(AllocFH)(int iSocket, int *pfh);
/**
 * Allocates file handle for a BSD 4.3 socket.
 *
 * @returns Pointer to socket handle on success.
 * @returns NULL and errno on failure.
 * @param   iSocket     The socket to allocate handle for.
 * @param   pfh         Where to store the filename.
 */
PLIBCSOCKETFH TCPNAMEG43(AllocFH)(int iSocket, int *pfh);

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
 * @param   pfh         Where to store the filename.
 * @param   ppFHSocket  Where to store the allocated socket handle. Optional.
 */
int TCPNAMEG44(AllocFHEx)(int fh, int iSocket, unsigned fFlags, int fNew, int *pfh, PLIBCSOCKETFH *ppFHSocket);
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
 * @param   pfh         Where to store the filename.
 * @param   ppFHSocket  Where to store the allocated socket handle. Optional.
 */
int TCPNAMEG43(AllocFHEx)(int fh, int iSocket, unsigned fFlags, int fNew, int *pfh, PLIBCSOCKETFH *ppFHSocket);


#if defined(OS2_INCLUDED) || defined(_OS2EMX_H)
/**
 * Loads a given TCPIP dll during fork.
 * @returns 0 on success.
 * @returns negative errno on failure.
 * @param   hmodLoad    The handle of the module we're loading.
 * @param   pszDll      The DLL name.
 */
int __libc_tcpipForkLoadModule(HMODULE hmodLoad, const char *pszDll);
#endif

#ifndef NO_TCPIP_INLINE

#ifdef IN_INNOTEK_LIBC
int TCPNAME(imp_sock_errno)(void);
void TCPNAME(imp_set_errno)(int);
#endif

/**
 * Sets the LIBC and socket errno variables to a given error number.
 */
static inline void  __libc_TcpipSetErrno(int err)
{
    errno = err;
#ifdef IN_INNOTEK_LIBC
    TCPNAME(imp_set_errno)(err + EOS2_TCPIP_OFFSET);
#else
    __libsocket_set_errno(err + EOS2_TCPIP_OFFSET);
#endif
}

/**
 * Updates the LIBC errno with the latest socket error.
 */
static inline void  __libc_TcpipUpdateErrno(void)
{
#ifdef IN_INNOTEK_LIBC
    int err = TCPNAME(imp_sock_errno)();
#else
    int err = __libsocket_sock_errno();
#endif
    if (err >= EOS2_TCPIP_OFFSET && err < EOS2_TCPIP_OFFSET + 1000)
        errno = err - EOS2_TCPIP_OFFSET;
}

/**
 * Updates the socket errno with the latest LIBC error.
 */
static inline void  __libc_TcpipSetSocketErrno(void)
{
#ifdef IN_INNOTEK_LIBC
    TCPNAME(imp_set_errno)(errno + EOS2_TCPIP_OFFSET);
#else
    __libsocket_set_errno(errno + EOS2_TCPIP_OFFSET);
#endif
}

/**
 * Gets the socket errno translating it to a LIBC errno.
 */
static inline int   __libc_TcpipGetSocketErrno(void)
{
#ifdef IN_INNOTEK_LIBC
    int err = TCPNAME(imp_sock_errno)();
#else
    int err = __libsocket_sock_errno();
#endif
    if (err >= EOS2_TCPIP_OFFSET && err < EOS2_TCPIP_OFFSET + 1000)
        return err - EOS2_TCPIP_OFFSET;
    return EDOOFUS;
}


/**
 * Retrieve the socket handle structure for a given handle.
 *
 * @returns Pointer to socket handle structure on success.
 * @returns NULL on failure with errno set to the appropriate value.
 * @param   iSocket  Socket handle number.
 */
static inline PLIBCSOCKETFH   __libc_TcpipFH(int iSocket)
{
    PLIBCSOCKETFH   pFHSocket = (PLIBCSOCKETFH)__libc_FH(iSocket);
    if (pFHSocket)
    {
        if ((pFHSocket->core.fFlags & __LIBC_FH_TYPEMASK) == F_SOCKET)
            return pFHSocket;
        __libc_TcpipSetErrno(ENOTSOCK);
    }
    else
        __libc_TcpipSetErrno(EBADF);
    return NULL;
}

#endif /* !NO_TCPIP_INLINE */

/** @} */


/** @} */

#endif
