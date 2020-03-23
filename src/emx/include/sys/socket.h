/* Modified for emx by hv and em 1994-1997
 * Modified for gcc/os2 by bird 2003
 *
 * Copyright (c) 1982,1985,1986,1988 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	from: @(#)socket.h	7.13 (Berkeley) 4/20/91
 *	$Id: socket.h,v 1.5 1993/06/27 05:59:06 andrew Exp $
 */

#ifndef _SYS_SOCKET_H_
#define _SYS_SOCKET_H_

#include <machine/ansi.h>
#define _NO_NAMESPACE_POLLUTION
#include <machine/param.h>
#undef _NO_NAMESPACE_POLLUTION


/* toolkit stuff */
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/cdefs.h>

#ifndef _SA_FAMILY_T_DECLARED
typedef	__sa_family_t	sa_family_t;
#define	_SA_FAMILY_T_DECLARED
#endif

#ifndef _SOCKLEN_T_DECLARED
typedef	__socklen_t	socklen_t;
#define	_SOCKLEN_T_DECLARED
#endif

#if defined (__cplusplus)
extern "C" {
#endif

#ifndef _EMX_TCPIP
#define _EMX_TCPIP
#endif

/*
 * Definitions related to sockets: types, address families, options.
 */

/*
 * Types
 */
#define	SOCK_STREAM	1		/* stream socket */
#define	SOCK_DGRAM	2		/* datagram socket */
#define	SOCK_RAW	3		/* raw-protocol interface */
#define	SOCK_RDM	4		/* reliably-delivered message */
#define	SOCK_SEQPACKET	5		/* sequenced packet stream */

/*
 * Option flags per-socket.
 */
#define	SO_DEBUG	0x0001		/* turn on debugging info recording */
#define	SO_ACCEPTCONN	0x0002		/* socket has had listen() */
#define	SO_REUSEADDR	0x0004		/* allow local address reuse */
#define	SO_KEEPALIVE	0x0008		/* keep connections alive */
#define	SO_DONTROUTE	0x0010		/* just use interface addresses */
#define	SO_BROADCAST	0x0020		/* permit sending of broadcast msgs */
#define	SO_USELOOPBACK	0x0040		/* bypass hardware when possible */
#define	SO_LINGER	0x0080		/* linger on close if data present */
#define	SO_OOBINLINE	0x0100		/* leave received OOB data in line */
#define	SO_L_BROADCAST	0x0200		/* limited broadcast sent on all IFs*/
#define	SO_RCV_SHUTDOWN	0x0400		/* set if shut down called for rcv */
#define	SO_SND_SHUTDOWN	0x0800		/* set if shutdown called for send */
#ifndef TCPV40HDRS
#define SO_REUSEPORT    0x1000          /* allow local address & port reuse */
#define SO_TTCP         0x2000          /* allow t/tcp on socket */
#endif

/*
 * Additional options, not kept in so_options.
 */
#define SO_SNDBUF	0x1001		/* send buffer size */
#define SO_RCVBUF	0x1002		/* receive buffer size */
#define SO_SNDLOWAT	0x1003		/* send low-water mark */
#define SO_RCVLOWAT	0x1004		/* receive low-water mark */
#ifndef TCPV40HDRS
#define SO_SNDTIMEO	0x1005		/* send timeout */
#define SO_RCVTIMEO	0x1006		/* receive timeout */
#endif
#define	SO_ERROR	0x1007		/* get error status and clear */
#define	SO_TYPE		0x1008		/* get socket type */
#define	SO_OPTIONS	0x1010		/* get socket options */

/*
 * Structure used for manipulating linger option.
 */
struct	linger {
#ifdef __RSXNT__
	short	l_onoff;		/* option on/off */
	short	l_linger;		/* linger time */
#else
	int	l_onoff;		/* option on/off */
	int	l_linger;		/* linger time */
#endif
};

/*
 * Level number for (get/set)sockopt() to apply to socket itself.
 */
#define	SOL_SOCKET	0xffff		/* options for socket level */

/*
 * Address families.
 */
#define	AF_UNSPEC	0		/* unspecified */
#define	AF_UNIX		1		/* local to host (pipes, portals) */
#ifndef TCPV40HDRS
#define AF_LOCAL        AF_UNIX
#endif
#define AF_OS2          AF_UNIX
#define	AF_INET		2		/* internetwork: UDP, TCP, etc. */
#define	AF_IMPLINK	3		/* arpanet imp addresses */
#define	AF_PUP		4		/* pup protocols: e.g. BSP */
#define	AF_CHAOS	5		/* mit CHAOS protocols */
#define	AF_NS		6		/* XEROX NS protocols */
#ifdef TCPV40HDRS
#define	AF_NBS		7		/* IBM: nbs protocols */
					/* (hv: I think IBM is outdated here */
#endif
#define	AF_ISO		7		/* ISO protocols */
#define	AF_OSI		AF_ISO
#define	AF_ECMA		8		/* european computer manufacturers */
#define	AF_DATAKIT	9		/* datakit protocols */
#define	AF_CCITT	10		/* CCITT protocols, X.25 etc */
#define	AF_SNA		11		/* IBM SNA */
#define AF_DECnet	12		/* DECnet */
#define AF_DLI		13		/* DEC Direct data link interface */
#define AF_LAT		14		/* LAT */
#define	AF_HYLINK	15		/* NSC Hyperchannel */
#define	AF_APPLETALK	16		/* Apple Talk */
#define	AF_NB		17		/* Netbios */
#define	AF_NETBIOS	AF_NB

#ifdef TCPV40HDRS
#define	AF_MAX		18
#else
#define AF_LINK         18              /* Link layer interface */
#define pseudo_AF_XTP   19              /* eXpress Transfer Protocol (no AF) */
#define AF_COIP         20              /* connection-oriented IP, aka ST II */
#define AF_CNT          21              /* Computer Network Technology */
#define pseudo_AF_RTIP  22              /* Help Identify RTIP packets */
#define AF_IPX          23              /* Novell Internet Protocol */
#define AF_SIP          24              /* Simple Internet Protocol */
#define AF_INET6        24
#define pseudo_AF_PIP   25              /* Help Identify PIP packets */
#define AF_ROUTE        39              /* Internal Routing Protocol */
#define AF_FWIP         40              /* firewall support */
#define AF_IPSEC        41              /* IPSEC and encryption techniques */
#define AF_DES          42              /* DES */
#define AF_MD5          43
#define AF_CDMF         44

#define AF_MAX          45
#endif


/*
 * Structure used by kernel to store most
 * addresses.
 * is called struct osockaddr in 4.4BSD
 */
struct sockaddr {
#ifdef TCPV40HDRS
	u_short	sa_family;		/* address family */
	char	sa_data[14];		/* up to 14 bytes of direct address */
#else
	u_char		sa_len;		/* total length */
	sa_family_t	sa_family;	/* address family */
	char		sa_data[14];	/* actually longer; address value */
#endif
};

/*
 * Structure used by kernel to pass protocol
 * information in raw sockets.
 */
struct sockproto {
	u_short	sp_family;		/* address family */
	u_short	sp_protocol;		/* protocol */
};

#include <sys/_sockaddr_storage.h>

/*
 * Protocol families, same as address families for now.
 */
#define	PF_UNSPEC	AF_UNSPEC
#define	PF_UNIX		AF_UNIX
#ifndef TCPV40HDRS
#define PF_LOCAL        AF_LOCAL
#endif
#define	PF_OS2		AF_UNIX
#define	PF_INET		AF_INET
#define	PF_IMPLINK	AF_IMPLINK
#define	PF_PUP		AF_PUP
#define	PF_CHAOS	AF_CHAOS
#define	PF_NS		AF_NS
#ifdef TCPV40HDRS
#define	PF_NBS		AF_NBS
#endif
#define	PF_ISO		AF_ISO
#define	PF_OSI		AF_OSI
#define	PF_ECMA		AF_ECMA
#define	PF_DATAKIT	AF_DATAKIT
#define	PF_CCITT	AF_CCITT
#define	PF_SNA		AF_SNA
#define PF_DECnet	AF_DECnet
#define PF_DLI		AF_DLI
#define PF_LAT		AF_LAT
#define	PF_HYLINK	AF_HYLINK
#define	PF_APPLETALK	AF_APPLETALK
#define	PF_NETBIOS	AF_NB
#define	PF_NB		AF_NB
#ifndef TCPV40HDRS
#define PF_ROUTE        AF_ROUTE
#define PF_LINK         AF_LINK
#define PF_XTP          pseudo_AF_XTP   /* really just proto family, no AF */
#define PF_COIP         AF_COIP
#define PF_CNT          AF_CNT
#define PF_SIP          AF_SIP
#define PF_INET6        AF_INET6
#define PF_IPX          AF_IPX          /* same format as AF_NS */
#define PF_RTIP         pseudo_AF_FTIP  /* same format as AF_INET */
#define PF_PIP          pseudo_AF_PIP
#endif /* !TCPV40HDRS */

#define	PF_MAX		AF_MAX


#ifndef TCPV40HDRS
/*
 * Definitions for network related sysctl, CTL_NET.
 *
 * Second level is protocol family.
 * Third level is protocol number.
 *
 * Further levels are defined by the individual families below.
 */
#define NET_MAXID       AF_MAX

#define CTL_NET_NAMES { \
        { 0, 0 }, \
        { "local", CTLTYPE_NODE }, \
        { "inet", CTLTYPE_NODE }, \
        { "implink", CTLTYPE_NODE }, \
        { "pup", CTLTYPE_NODE }, \
        { "chaos", CTLTYPE_NODE }, \
        { "xerox_ns", CTLTYPE_NODE }, \
        { "iso", CTLTYPE_NODE }, \
        { "emca", CTLTYPE_NODE }, \
        { "datakit", CTLTYPE_NODE }, \
        { "ccitt", CTLTYPE_NODE }, \
        { "ibm_sna", CTLTYPE_NODE }, \
        { "decnet", CTLTYPE_NODE }, \
        { "dec_dli", CTLTYPE_NODE }, \
        { "lat", CTLTYPE_NODE }, \
        { "hylink", CTLTYPE_NODE }, \
        { "appletalk", CTLTYPE_NODE }, \
        { "netbios", CTLTYPE_NODE }, \
        { "route", CTLTYPE_NODE }, \
        { "link_layer", CTLTYPE_NODE }, \
        { "xtp", CTLTYPE_NODE }, \
        { "coip", CTLTYPE_NODE }, \
        { "cnt", CTLTYPE_NODE }, \
        { "rtip", CTLTYPE_NODE }, \
        { "ipx", CTLTYPE_NODE }, \
        { "sip", CTLTYPE_NODE }, \
        { "pip", CTLTYPE_NODE }, \
}

/*
 * PF_ROUTE - Routing table
 *
 * Three additional levels are defined:
 *      Fourth: address family, 0 is wildcard
 *      Fifth: type of info, defined below
 *      Sixth: flag(s) to mask with for NET_RT_FLAGS
 */
#define NET_RT_DUMP     1               /* dump; may limit to a.f. */
#define NET_RT_FLAGS    2               /* by flags, e.g. RESOLVING */
#define NET_RT_IFLIST   3               /* survey interface list */
#define NET_RT_MAXID    4

#define CTL_NET_RT_NAMES { \
        { 0, 0 }, \
        { "dump", CTLTYPE_STRUCT }, \
        { "flags", CTLTYPE_STRUCT }, \
        { "iflist", CTLTYPE_STRUCT }, \
}

#endif /* !TCPV40HDRS */


/*
 * Maximum queue length specifiable by listen.
 */
#ifdef TCPV40HDRS
#define	SOMAXCONN	5
#else
#define SOMAXCONN       1024
#endif

/*
 * 4.3-compat message header (move to compat file later).
 * is called omsghdr in 4.4BSD
 */
struct msghdr {
	caddr_t	msg_name;		/* optional address */
#ifdef TCPV40HDRS
	int	msg_namelen;		/* size of address */
	struct	iovec *msg_iov;		/* scatter/gather array */
	int	msg_iovlen;		/* # elements in msg_iov */
	caddr_t	msg_accrights;		/* access rights sent/received */
	int	msg_accrightslen;
#else
        socklen_t   msg_namelen;            /* size of address */
        struct iovec *msg_iov;              /* scatter/gather array */
        u_int       msg_iovlen;             /* # elements in msg_iov */
        caddr_t     msg_control;            /* ancillary data, see below */
        socklen_t   msg_controllen;         /* ancillary data buffer len */
        long        msg_flags;              /* flags on received message */
#endif
};

#define	MSG_OOB		0x1		/* process out-of-band data */
#define	MSG_PEEK	0x2		/* peek at incoming message */
#define	MSG_DONTROUTE	0x4		/* send without using routing tables */
#define MSG_FULLREAD    0x8             /* send without using routing tables */
#ifndef TCPV40HDRS
#define MSG_EOR         0x10            /* data completes record */
#define MSG_TRUNC       0x20            /* data discarded before delivery */
#define MSG_CTRUNC      0x40            /* control data lost before delivery */
#define MSG_WAITALL     0x80            /* wait for full request or error */
#define MSG_DONTWAIT    0x100           /* this message should be nonblocking */
#ifdef TTCP
#define MSG_EOF         0x200
#endif
#define MSG_MAPIO       0x400           /* mem mapped io */
#define MSG_CLOSE       0x800           /* close connection after succesful send_file */
#endif

#ifdef TCPV40HDRS
#define	MSG_MAXIOVLEN	16
#endif

/*
 * Header for ancillary data objects in msg_control buffer.
 * Used for additional information with/about a datagram
 * not expressible by flags.  The format is a sequence
 * of message elements headed by cmsghdr structures.
 */
struct cmsghdr {
	socklen_t	cmsg_len;	/* data byte count, including hdr */
	int		cmsg_level;	/* originating protocol */
	int		cmsg_type;	/* protocol-specific type */
/* followed by	u_char  cmsg_data[]; */
};

/* given pointer to struct adatahdr, return pointer to data */
#define	CMSG_DATA(cmsg)		((u_char *)((cmsg) + 1))

/* given pointer to struct adatahdr, return pointer to next adatahdr */
#define	CMSG_NXTHDR(mhdr, cmsg)	\
	(((caddr_t)(cmsg) + (cmsg)->cmsg_len + sizeof(struct cmsghdr) > \
	    (mhdr)->msg_control + (mhdr)->msg_controllen) ? \
	    (struct cmsghdr *)NULL : \
	    (struct cmsghdr *)((caddr_t)(cmsg) + _ALIGN((cmsg)->cmsg_len)))

#define	CMSG_FIRSTHDR(mhdr)	((struct cmsghdr *)(mhdr)->msg_control)

/* size of control message for contents of length len inlcuding its header */
#define CMSG_LEN(len) (_ALIGN(sizeof(struct cmsghdr)) + (len))

/* size needed to hold control message and its contents of length len */
#define CMSG_SPACE(len) (_ALIGN(sizeof(struct cmsghdr)) + _ALIGN(len))

/* "Socket"-level control message types: */
#define	SCM_RIGHTS	0x01		/* access rights (array of int) */

#ifndef TCPV40HDRS
/*
 * 4.3 compat sockaddr, move to compat file later
 */
struct osockaddr {
        u_short sa_family;              /* address family */
        char    sa_data[14];            /* up to 14 bytes of direct address */
};

/*
 * 4.3-compat message header (move to compat file later).
 */
struct omsghdr {
        caddr_t msg_name;               /* optional address */
        int     msg_namelen;            /* size of address */
        struct  iovec *msg_iov;         /* scatter/gather array */
        int     msg_iovlen;             /* # elements in msg_iov */
        caddr_t msg_accrights;          /* access rights sent/received */
        int     msg_accrightslen;
};

/*
 * send_file parameter structure
 */
struct sf_parms {
        void   *header_data;      /* ptr to header data */
        size_t header_length;     /* size of header data */
        int    file_handle;       /* file handle to send from */
        size_t file_size;         /* size of file */
        int    file_offset;       /* byte offset in file to send from */
        size_t file_bytes;        /* bytes of file to be sent */
        void   *trailer_data;     /* ptr to trailer data */
        size_t trailer_length;    /* size of trailer data */
        size_t bytes_sent;        /* bytes sent in this send_file call */
};
#endif

/*
 * howto arguments for shutdown(2), specified by Posix.1g.
 */
#define	SHUT_RD		0		/* shut down the reading side */
#define	SHUT_WR		1		/* shut down the writing side */
#define	SHUT_RDWR	2		/* shut down both sides */

int     TCPCALL accept (int, struct sockaddr *, int *);
int     TCPCALL bind (int, const struct sockaddr *, int);
int     TCPCALL connect (int, const struct sockaddr *, int);
int     TCPCALL gethostid (void);
int     TCPCALL getpeername (int, struct sockaddr *, int *);
int     TCPCALL getsockname (int, struct sockaddr *, int *);
int     TCPCALL getsockopt (int, int, int, void *, int *);
int     TCPCALL listen (int, int);
int     TCPCALL recv (int, void *, int, int);
int     TCPCALL recvfrom (int, void *, int, int, struct sockaddr *, int *);
int     TCPCALL recvmsg (int, struct msghdr *, int);
int     TCPCALL send (int, const void *, int, int);
int     TCPCALL sendto (int, const void *, int, int, const struct sockaddr *, int);
int     TCPCALL sendmsg (int, const struct msghdr *, int);
int     TCPCALL setsockopt (int, int, int, const void *, int);
int     TCPCALL shutdown (int, int);
int     TCPCALL socket (int, int, int);
int     TCPCALL socketpair (int, int, int, int *);

/* EMX addition */
int    _impsockhandle (int, int);

#ifndef TCPV40HDRS
int     TCPCALL accept_and_recv (long, long*, struct sockaddr *, long*, struct sockaddr*, long*, caddr_t, size_t);
#endif

/* OS/2 additions */
void    TCPCALL addsockettolist(int);
int     TCPCALL removesocketfromlist(int);
#include <sys/ioccom.h>
#ifdef TCPV40HDRS
#include <sys/select.h>
#endif
int     TCPCALL sock_init( void );
int     TCPCALL sock_errno( void );
int     TCPCALL os2_sock_errno( void );
void    TCPCALL psock_errno( char * );
int     TCPCALL soclose( int );
int     TCPCALL so_cancel(int);
int     TCPCALL Raccept(int, struct sockaddr *, int *);
#ifdef TCPV40HDRS
struct sockaddr_in;
int     TCPCALL Rbind(int, struct sockaddr_in *, int, struct sockaddr_in *);
#else
int     TCPCALL Rbind(int, struct sockaddr *, int, struct sockaddr *);
#endif
int     TCPCALL Rconnect(int, const struct sockaddr *, int);
int     TCPCALL Rgetsockname(int, struct sockaddr *, int *);
int     TCPCALL Rlisten(int, int);
#ifndef TCPV40HDRS
ssize_t TCPCALL send_file(int *, struct sf_parms *, int );
char *  TCPCALL sock_strerror(int);
int     TCPCALL getinetversion(char *);
#endif


/* more OS/2 stuff. */
#ifndef MAXSOCKETS
#ifdef TCPV40HDRS
#define MAXSOCKETS  2048
#else
#define MAXSOCKETS  32768
#endif
#endif

#define	MT_FREE		0
#define	MT_DATA		1
#define	MT_HEADER	2
#define	MT_SOCKET	3
#define	MT_PCB		4
#define	MT_RTABLE	5
#define	MT_HTABLE	6
#define	MT_ATABLE	7
#define	MT_SONAME	8
#define	MT_ZOMBIE	9
#define	MT_SOOPTS	10
#define	MT_FTABLE	11
#define	MT_RIGHTS	12
#define	MT_IFADDR	13

struct mbstat {
	u_short		m_mbufs;
	u_short		m_clusters;
	u_short		m_clfree;
	u_short		m_drops;
	u_long		m_wait;
	u_short		m_mtypes[256];
};


struct sostats {
	short count;
#ifdef TCPV40HDRS
        short socketdata[9*MAXSOCKETS];
#else
        short socketdata[13*MAXSOCKETS];
#endif
};

#if defined (__cplusplus)
}
#endif

#endif /* !_SYS_SOCKET_H_ */
