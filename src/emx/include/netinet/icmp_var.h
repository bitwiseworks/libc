/* Modified for gcc/os2 by bird 2003
 *
 * Copyright (c) 1982, 1986, 1993
 *	The Regents of the University of California.  All rights reserved.
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
 *	@(#)icmp_var.h	8.1 (Berkeley) 6/10/93
 * $FreeBSD: src/sys/netinet/icmp_var.h,v 1.15.2.2 2001/12/07 09:23:11 ru Exp $
 */

#ifndef _NETINET_ICMP_VAR_H_
#define _NETINET_ICMP_VAR_H_

#ifndef TCPV40HDRS

#ifdef _KERNEL
#include "opt_icmp_bandlim.h"		/* for ICMP_BANDLIM     */
#endif

/*
 * Variables related to this implementation
 * of the internet control message protocol.
 */
struct	icmpstat {
#if 0 /* OS2 is slightly different */
/* statistics related to icmp packets generated */
	u_long	icps_error;		/* # of calls to icmp_error */
	u_long	icps_oldshort;		/* no error 'cuz old ip too short */
	u_long	icps_oldicmp;		/* no error 'cuz old was icmp */
	u_long	icps_outhist[ICMP_MAXTYPE + 1];
/* statistics related to input messages processed */
 	u_long	icps_badcode;		/* icmp_code out of range */
	u_long	icps_tooshort;		/* packet < ICMP_MINLEN */
	u_long	icps_checksum;		/* bad checksum */
	u_long	icps_badlen;		/* calculated bound mismatch */
	u_long	icps_reflect;		/* number of responses */
	u_long	icps_inhist[ICMP_MAXTYPE + 1];
#else  /* OS2: short not longs */
	u_short	icps_error;		/* # of calls to icmp_error */
	u_short	icps_oldshort;		/* no error 'cuz old ip too short */
	u_short	icps_oldicmp;		/* no error 'cuz old was icmp */
	u_short	icps_outhist[ICMP_MAXTYPE + 1];
/* statistics related to input messages processed */
 	u_short	icps_badcode;		/* icmp_code out of range */
	u_short	icps_tooshort;		/* packet < ICMP_MINLEN */
	u_short	icps_checksum;		/* bad checksum */
	u_short	icps_badlen;		/* calculated bound mismatch */
	u_short	icps_reflect;		/* number of responses */
	u_short	icps_inhist[ICMP_MAXTYPE + 1];
#endif
#if 0 /* not on OS/2 */
	u_long	icps_bmcastecho; 	/* b/mcast echo requests dropped */
	u_long	icps_bmcasttstamp; 	/* b/mcast tstamp requests dropped */
	u_long	icps_badaddr;		/* bad return address */
	u_long	icps_noroute; 		/* no route back */
#endif
#ifdef MIB
        u_long  icps_OutMsgs;
        u_long  icps_OutErrors;
        u_long  icps_InMsgs;
        u_long  icps_InDestUnreachs;
        u_long  icps_InTimeExcds;
        u_long  icps_InParmProbs;
        u_long  icps_InSrcQuenchs;
        u_long  icps_InRedirects;
        u_long  icps_InEchos;
        u_long  icps_InEchoReps;
        u_long  icps_InTimestamps;
        u_long  icps_InTimestampReps;
        u_long  icps_InAddrMasks;
        u_long  icps_InAddrMaskReps;
        u_long  icps_OutDestUnreachs;
        u_long  icps_OutTimeExcds;
        u_long  icps_OutParmProbs;
        u_long  icps_OutSrcQuenchs;
        u_long  icps_OutRedirects;
        u_long  icps_OutEchos;
        u_long  icps_OutEchoReps;
        u_long  icps_OutTimestamps;
        u_long  icps_OutTimestampReps;
        u_long  icps_OutAddrMasks;
        u_long  icps_OutAddrMaskReps;
#endif
/* OS/2 stuff */
};

/*
 * Names for ICMP sysctl objects
 */
#define	ICMPCTL_MASKREPL	1	/* allow replies to netmask requests */
#define	ICMPCTL_STATS		2	/* statistics (read-only) */
#if 0 /* Diffent on OS2 */
#define ICMPCTL_ICMPLIM		3
#define ICMPCTL_MAXID		4

#define ICMPCTL_NAMES { \
	{ 0, 0 }, \
	{ "maskrepl", CTLTYPE_INT }, \
	{ "stats", CTLTYPE_STRUCT }, \
	{ "icmplim", CTLTYPE_INT }, \
}
#else /* OS2: */
#define ICMPCTL_ECHOREPL        3  /* allow replies to ping requests */
#define ICMPCTL_TTL             50 /* sysctl code - TTL for ICMP packets */
#define ICMPCTL_NAMES { \
        { 0, 0 }, \
        { "maskrepl", CTLTYPE_INT }, \
        { "stats", CTLTYPE_STRUCT }, \
        { "echorepl", CTLTYPE_STRUCT }, \
        { "icmpttl",CTLYPE_INT }, \
}
#endif

#if 0 /* not on OS2 */
#ifdef _KERNEL
SYSCTL_DECL(_net_inet_icmp);
#ifdef ICMP_BANDLIM
extern int badport_bandlim __P((int));
#endif
#define BANDLIM_UNLIMITED -1
#define BANDLIM_ICMP_UNREACH 0
#define BANDLIM_ICMP_ECHO 1
#define BANDLIM_ICMP_TSTAMP 2
#define BANDLIM_RST_CLOSEDPORT 3 /* No connection, and no listeners */
#define BANDLIM_RST_OPENPORT 4   /* No connection, listener */
#define BANDLIM_MAX 4
#endif
#endif  /* 0 */

#endif /* !TCPV40HDRS */
#endif
