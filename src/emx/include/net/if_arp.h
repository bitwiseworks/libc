/* Modified for emx by hv 1994,1996
 * Modified for gcc/os2 by bird 2003
 *
 * Copyright (c) 1986 Regents of the University of California.
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
 *	from: @(#)if_arp.h	7.4 (Berkeley) 6/28/90
 *	$Id: if_arp.h,v 1.5 1993/09/05 00:46:54 cassidy Exp $
 */

#ifndef _NET_IF_ARP_H_
#define _NET_IF_ARP_H_

/*
 * Address Resolution Protocol.
 *
 * See RFC 826 for protocol description.  ARP packets are variable
 * in size; the arphdr structure defines the fixed-length portion.
 * Protocol type values are the same as those for 10 Mb/s Ethernet.
 * It is followed by the variable-sized fields ar_sha, arp_spa,
 * arp_tha and arp_tpa in that order, according to the lengths
 * specified.  Field names used correspond to RFC 826.
 */
#ifndef TCPV40HDR
#pragma pack(1)
#endif
struct	arphdr {
	u_short	ar_hrd;		/* format of hardware address */
#define ARPHRD_ETHER 	1	/* ethernet hardware address */
#define ARPHRD_802	6	/* 802 net hardware address */
#ifndef TCPV40HDRS
#define ARPHRD_FRELAY   15      /* frame relay hardware format */
#endif
	u_short	ar_pro;		/* format of protocol address */
	u_char	ar_hln;		/* length of hardware address */
	u_char	ar_pln;		/* length of protocol address */
	u_short	ar_op;		/* one of: */
#define	ARPOP_REQUEST	1	/* request to resolve address */
#define	ARPOP_REPLY	2	/* response to previous request */
#ifndef TCPV40HDRS
#define ARPOP_REVREQUEST 3      /* request protocol address given hardware */
#define ARPOP_REVREPLY   4      /* response giving protocol address */
#define ARPOP_INVREQUEST 8      /* request to identify peer */
#define ARPOP_INVREPLY   9      /* response identifying peer */
#endif
#if 0 /* not OS/2 */
#define REVARP_REQUEST	3	/* reverse ARP request (not IBM) */
#define REVARP_REPLY	4	/* reverse ARP reply (not IBM) */
#endif
/*
 * The remaining fields are variable in size,
 * according to the sizes above.
 */
/*	u_char	ar_sha[];	 * sender hardware address */
/*	u_char	ar_spa[];	 * sender protocol address */
/*	u_char	ar_tha[];	 * target hardware address */
/*	u_char	ar_tpa[];	 * target protocol address */
};
#ifndef TCPV40HDRS
#pragma pack()
#endif

/*
 * ARP ioctl request
 */
#ifndef TCPV40HDRS
#pragma pack(4)
#endif
struct arpreq {
	struct	sockaddr arp_pa;		/* protocol address */
	struct	sockaddr arp_ha;		/* hardware address */
	int	arp_flags;			/* flags */
};
#ifndef TCPV40HDRS
#pragma pack()
#endif

#ifndef TCPV40HDRS
/*
 * Token ring ARP ioctl request
 */
#pragma pack(4)
struct arpreq_tr {
        struct  sockaddr arp_pa;                /* protocol address */
        struct  sockaddr arp_ha;                /* hardware address */
        long    arp_flags;                     /* flags */
        u_short arp_rcf;                        /* token ring routing control field */
        u_short arp_rseg[8];                    /* token ring routing segments */
};
#endif /*!TCPV40HDRS*/


/*  arp_flags and at_flags field values */
#define	ATF_INUSE	0x01	/* entry in use */
#define ATF_COM		0x02	/* completed entry (enaddr valid) */
#define	ATF_PERM	0x04	/* permanent entry */
#define	ATF_PUBL	0x08	/* publish entry (respond for other host) */
#define	ATF_USETRAILERS	0x10	/* has requested trailers */
#ifndef TCPV40HDRS
#define ATF_802_3       0x20    /* sender's ether if set to 802.3 */
#endif

#endif /* !_NET_IF_ARP_H_ */
