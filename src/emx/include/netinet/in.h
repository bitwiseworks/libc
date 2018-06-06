/* Modified for emx by hv and em 1994,1996
 * Modified for gcc/os2 by bird 2003
 * Modified for kLIBC by bird 2007, added INET_ADDRSTRLEN.
 *
 * Copyright (c) 1982, 1986, 1990 Regents of the University of California.
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
 *	from: @(#)in.h	7.11 (Berkeley) 4/20/91
 *	$Id: in.h,v 1.8 1994/01/28 13:46:02 deraadt Exp $
 */

#ifndef _NETINET_IN_H_
#define _NETINET_IN_H_

#ifndef _EMX_TCPIP
#define _EMX_TCPIP
#endif

#include <sys/param.h>          /* htons() etc. */
/* added 2007: */
#include <sys/cdefs.h>
#include <sys/_types.h>
#include <machine/endian.h>

__BEGIN_DECLS

/* added 2007: */
#if __POSIX_VISIBLE >= 200112
#define	INET_ADDRSTRLEN		16
#endif

/*
 * Constants and structures defined by the internet system,
 * Per RFC 790, September 1981.
 */

/*
 * Protocols
 */
#define	IPPROTO_IP		0		/* dummy for IP */
#define	IPPROTO_ICMP		1		/* control message protocol */
#define	IPPROTO_IGMP		2		/* group mgmt protocol */
#define	IPPROTO_GGP		3		/* gateway^2 (deprecated) */
#define	IPPROTO_TCP		6		/* tcp */
#define	IPPROTO_EGP		8		/* exterior gateway protocol */
#define	IPPROTO_PUP		12		/* pup */
#define	IPPROTO_UDP		17		/* user datagram protocol */
#define	IPPROTO_IDP		22		/* xns idp */
#ifndef TCPV40HDRS
#define	IPPROTO_TP		29 		/* tp-4 w/ class negotiation */
#define	IPPROTO_EON		80		/* ISO cnlp */
#define IPPROTO_ENCAP           98              /* encapsulation header */
#endif

#define	IPPROTO_RAW		255		/* raw IP packet */
#define	IPPROTO_MAX		256


/*
 * Local port number conventions:
 * Ports < IPPORT_RESERVED are reserved for
 * privileged processes (e.g. root).
 * Ports > IPPORT_USERRESERVED are reserved
 * for servers, not necessarily privileged.
 */
#ifdef TCPV40HDRS
#define	IPPORT_RESERVED		1024
#define	IPPORT_USERRESERVED	5000
#else
 /* Changing the ephemeral port #s as per Internet Assigned Numbers Authority's
  * update as found in JUL '97. [Ref: http://www.isi.edu/div7/iana/descript/html] */
#define IPPORT_RESERVED        49152  /* Old 1024. Changed as per IANA draft */
#define IPPORT_USERRESERVED    65535  /* Old 5000. Changed as per IANA draft */
#endif

#ifdef TCPV40HDRS
/*
 * Link numbers
 */
#define IMPLINK_IP              155
#define IMPLINK_LOWEXPER        156
#define IMPLINK_HIGHEXPER       158
#endif

/*
 * Internet address (a structure for historical reasons)
 */
#ifndef _STRUCT_IN_ADDR_DECLARED
struct in_addr {
	u_long s_addr;
};
#define _STRUCT_IN_ADDR_DECLARED
#endif


/*
 * Definitions of bits in internet address integers.
 * On subnets, the decomposition of addresses to host and net parts
 * is done according to subnet mask, not the masks here.
 */
#define	IN_CLASSA(i)		(((long)(i) & 0x80000000) == 0)
#define	IN_CLASSA_NET		0xff000000
#define	IN_CLASSA_NSHIFT	24
#define	IN_CLASSA_HOST		0x00ffffff
#define	IN_CLASSA_MAX		128

#define	IN_CLASSB(i)		(((long)(i) & 0xc0000000) == 0x80000000)
#define	IN_CLASSB_NET		0xffff0000
#define	IN_CLASSB_NSHIFT	16
#define	IN_CLASSB_HOST		0x0000ffff
#define	IN_CLASSB_MAX		65536

#define	IN_CLASSC(i)		(((long)(i) & 0xe0000000) == 0xc0000000)
#define	IN_CLASSC_NET		0xffffff00
#define	IN_CLASSC_NSHIFT	8
#define	IN_CLASSC_HOST		0x000000ff

#define	IN_CLASSD(i)		(((long)(i) & 0xf0000000) == 0xe0000000)
#ifdef TCPV40HDRS
#define IN_CLASSD_NET           0xffffffff
#define	IN_CLASSD_HOST		0
#else
#define	IN_CLASSD_NET		0xf0000000	/* These ones aren't really */
#define	IN_CLASSD_HOST		0x0fffffff	/* routing needn't know. */
#endif
#define	IN_CLASSD_NSHIFT	28		/* net and host fields, but */
#define	IN_MULTICAST(i)		IN_CLASSD(i)

#ifdef TCPV40HDRS
#define	IN_EXPERIMENTAL(i)	(((long)(i) & 0xe0000000) == 0xe0000000)
#else
#define	IN_EXPERIMENTAL(i)	(((long)(i) & 0xf0000000) == 0xe0000000)
#endif
#define	IN_BADCLASS(i)		(((long)(i) & 0xf0000000) == 0xf0000000)

#define	INADDR_ANY		(u_long)0x00000000
#ifndef TCPV40HDRS
#define	INADDR_LOOPBACK		(u_long)0x7f000001
#endif
#define	INADDR_BROADCAST	(u_long)0xffffffff	/* must be masked */

#define	INADDR_UNSPEC_GROUP	(u_long)0xe0000000	/* 224.0.0.0 */
#define	INADDR_ALLHOSTS_GROUP	(u_long)0xe0000001	/* 224.0.0.1 */
#define	INADDR_MAX_LOCAL_GROUP	(u_long)0xe00000ff	/* 224.0.0.255 */

#ifndef KERNEL
#define	INADDR_NONE		0xffffffff		/* -1 return */
#endif

#define	IN_LOOPBACKNET		127			/* official! */


/*
 * Socket address, internet style. 4.3BSD
 */
#ifdef TCPV40HDRS
struct sockaddr_in {
	short	sin_family;
	u_short	sin_port;
	struct	in_addr sin_addr;
	char	sin_zero[8];
};
#else
#pragma pack(1)
struct sockaddr_in {
        u_char  sin_len;
        u_char  sin_family;
        u_short sin_port;
        struct  in_addr sin_addr;
        char    sin_zero[8];
};
#pragma pack()
#endif

/*
 * Structure used to describe IP options.
 * Used to store options internally, to pass them to a process,
 * or to restore options retrieved earlier.
 * The ip_dst is used for the first-hop gateway when using a source route
 * (this gets put into the header proper).
 */
#ifndef TCPV40HDRS
#pragma pack(1)
#endif
struct ip_opts {
	struct	in_addr ip_dst;		/* first hop, 0 w/o src rt */
	char	ip_opts[40];		/* actually variable in size */
};
#ifndef TCPV40HDRS
#pragma pack()
#endif

/*
 * Options for use with [gs]etsockopt at the IP level.
 * First word of comment is data type; bool is stored in int.
 */
#define	IP_OPTIONS		1	/* buf/ip_opts; set/get IP options */
#define	IP_MULTICAST_IF		2	/* u_char; set/get IP mcast i/f */
#define	IP_MULTICAST_TTL	3	/* u_char; set/get IP mcast ttl */
#define	IP_MULTICAST_LOOP	4	/* u_char; set/get IP mcast loopback */
#define	IP_ADD_MEMBERSHIP	5	/* ip_mreq; add IP group membership */
#define	IP_DROP_MEMBERSHIP	6	/* ip_mreq; drop IP group membership */
#ifndef TCPV40HDRS
#define IP_HDRINCL              7    /* int; header is included with data */
#define IP_TOS                  8    /* int; IP type of service and preced. */
#define IP_TTL                  9    /* int; IP time to live */
#define IP_RECVOPTS             10   /* bool; receive all IP opts w/dgram */
#define IP_RECVRETOPTS          11   /* bool; receive IP opts for response */
#define IP_RECVDSTADDR          12   /* bool; receive IP dst addr w/dgram */
#define IP_RETOPTS              13   /* ip_opts; set/get IP options */
#define IP_RECVTRRI             14   /* bool; receive token ring routing inf */
#endif

/*
 * Defaults and limits for options
 */
#define	IP_DEFAULT_MULTICAST_TTL   1    /* normally limit m'casts to 1 hop  */
#define	IP_DEFAULT_MULTICAST_LOOP  1    /* normally hear sends if a member  */
#define	IP_MAX_MEMBERSHIPS         20   /* per socket; must fit in one mbuf */
#ifndef TCPV40HDRS
#define MAX_IN_MULTI      16*IP_MAX_MEMBERSHIPS      /* 320 max per os2 */
extern u_short CntInMulti;
#endif

/*
 * Argument structure for IP_ADD_MEMBERSHIP and IP_DROP_MEMBERSHIP.
 */
#ifndef TCPV40HDRS
#pragma pack(1)
#endif
struct ip_mreq {
	struct	in_addr imr_multiaddr;	/* IP multicast address of group */
	struct	in_addr imr_interface;	/* local IP address of interface */
};
#ifndef TCPV40HDRS
#pragma pack()
#endif

#ifdef TCPV40HDRS
/* for functions */
#include <arpa/inet.h>

#else

/*
 * Definitions for inet sysctl operations.
 *
 * Third level is protocol number.
 * Fourth level is desired variable within that protocol.
 */
#define IPPROTO_MAXID   (IPPROTO_IDP + 1)       /* don't list to IPPROTO_MAX */

#define CTL_IPPROTO_NAMES { \
        { "ip", CTLTYPE_NODE }, \
        { "icmp", CTLTYPE_NODE }, \
        { "igmp", CTLTYPE_NODE }, \
        { "ggp", CTLTYPE_NODE }, \
        { 0, 0 }, \
        { 0, 0 }, \
        { "tcp", CTLTYPE_NODE }, \
        { 0, 0 }, \
        { "egp", CTLTYPE_NODE }, \
        { 0, 0 }, \
        { 0, 0 }, \
        { 0, 0 }, \
        { "pup", CTLTYPE_NODE }, \
        { 0, 0 }, \
        { 0, 0 }, \
        { 0, 0 }, \
        { 0, 0 }, \
        { "udp", CTLTYPE_NODE }, \
        { 0, 0 }, \
        { 0, 0 }, \
        { 0, 0 }, \
        { 0, 0 }, \
        { "idp", CTLTYPE_NODE }, \
}
#endif /*TCPV40HDRS (else) */

__END_DECLS

#endif /* !_NETINET_IN_H_ */
