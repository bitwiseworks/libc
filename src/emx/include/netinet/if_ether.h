/* Modified for emx by hv 1994,1996
 * Modified for gcc by bird 2003
 *
 * Copyright (c) 1982, 1986 Regents of the University of California.
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
 *	from: @(#)if_ether.h	7.5 (Berkeley) 6/28/90
 *	$Id: if_ether.h,v 1.8 1994/02/02 05:58:54 hpeyerl Exp $
 */

#ifndef _NETINET_IF_ETHER_H_
#define _NETINET_IF_ETHER_H_

#ifdef TCPV40HDRS

/*
 * Structure of a 10Mb/s Ethernet header.
 */
struct	ether_header {
	u_char	ether_dhost[6];
	u_char	ether_shost[6];
	u_short	ether_type;
};

#define	ETHERTYPE_PUP		0x0200	/* PUP protocol */
#define	ETHERTYPE_IP		0x0800	/* IP protocol */
#ifdef OS2 /* hv thinks this is just a dirty hack, but we must include it. */
#define	ETHERTYPE_ARP		0x0608	/* address resolution protocol */
#else
#define	ETHERTYPE_ARP		0x0806	/* address resolution protocol */
#endif

/*
 * The ETHERTYPE_NTRAILER packet types starting at ETHERTYPE_TRAIL have
 * (type-ETHERTYPE_TRAIL)*512 bytes of data followed
 * by an ETHER type (as given above) and then the (variable-length) header.
 */
#define	ETHERTYPE_TRAIL		0x1000		/* Trailer packet */
#define	ETHERTYPE_NTRAILER	16

#define	ETHERMTU	1500
#define	ETHERMIN	(60-14)

/*
 * Ethernet Address Resolution Protocol.
 *
 * See RFC 826 for protocol description.  Structure below is adapted
 * to resolving internet addresses.  Field names used correspond to
 * RFC 826.
 */
struct	ether_arp {
	struct	arphdr ea_hdr;	/* fixed-size header */
	u_char	arp_sha[6];	/* sender hardware address */
	u_char	arp_spa[4];	/* sender protocol address */
	u_char	arp_tha[6];	/* target hardware address */
	u_char	arp_tpa[4];	/* target protocol address */
};
#define	arp_hrd	ea_hdr.ar_hrd
#define	arp_pro	ea_hdr.ar_pro
#define	arp_hln	ea_hdr.ar_hln
#define	arp_pln	ea_hdr.ar_pln
#define	arp_op	ea_hdr.ar_op


/*
 * Structure shared between the ethernet driver modules and
 * the address resolution code.  For example, each ec_softc or il_softc
 * begins with this structure.
 */
struct	arpcom {
	struct 	ifnet ac_if;		/* network-visible interface */
	u_char	ac_enaddr[6];		/* ethernet hardware address */
	struct in_addr ac_ipaddr;	/* copy of ip address- XXX */
/* not os2 */
/*	struct ether_multi *ac_multiaddrs;*/ /* list of ether multicast addrs */
/*	int ac_multicnt;*/		/* length of ac_multiaddrs list */
};

/*
 * Internet to ethernet address resolution table.
 */
#pragma pack(1)
struct	arptab {
	struct	in_addr at_iaddr;	/* internet address */
	u_char	at_enaddr[6];		/* ethernet address */
	u_char	at_timer;		/* minutes since last reference */
	u_char	at_flags;		/* flags */
	struct	mbuf *at_hold;		/* last packet until resolved/timeout */
/* only os2 */
	u_short	at_rcf;			/* token ring routing control field */
	u_short at_rseg[8];		/* token ring routing segments */
#ifdef OS2
	u_long at_millisec;		/* TOD millsecons of last update */
	short at_interface;		/* interface index */
#endif
};
#pragma pack()


#else /*TCPV40HDRS*/


#pragma pack(1)
struct sockaddr_inarp {
        u_char  sin_len;
        u_char  sin_family;
        u_short sin_port;
        struct  in_addr sin_addr;
        struct  in_addr sin_srcaddr;
        u_short sin_tos;
        u_short sin_other;
#define SIN_PROXY 1
};
struct oarptab {
        struct  in_addr at_iaddr;       /* internet address */
        u_char  at_enaddr[6];           /* ethernet address */
        u_char  at_timer;               /* minutes since last reference */
        u_char  at_flags;               /* flags */
        void * at_hold;
        u_short at_rcf;                 /* token ring routing control field */
        u_short at_rseg[8];             /* token ring routing segments */
        u_long  at_millisec;            /* TOD milliseconds of last update */
        u_short at_interface;           /* interface index */
};
#pragma pack()

/*
 * IP and ethernet specific routing flags
 */
#define RTF_USETRAILERS RTF_PROTO1      /* use trailers */
#define RTF_ANNOUNCE    RTF_PROTO2      /* announce new arp entry */


#endif /*TCPV40HDRS (else) */

#endif /* !_NETINET_IF_ETHER_H_ */
