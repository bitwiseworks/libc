/*- Modified for gcc/os2 by bird 2003
 *
 * Copyright (c) 1982, 1986, 1990, 1993, 1994
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
 *	@(#)sockio.h	8.1 (Berkeley) 3/28/94
 * $FreeBSD: src/sys/sys/sockio.h,v 1.14.2.6 2002/02/07 15:12:37 ambrisko Exp $
 */

#ifndef	_SYS_SOCKIO_H_
#define	_SYS_SOCKIO_H_

#include <sys/ioccom.h>
#include "ioccom.h"

/* Socket ioctl's. */
#ifdef TCPV40HDRS
#define	SIOCSHIWAT	 _IOW('s',  0, int)		/* set high watermark */
#define	SIOCGHIWAT	 _IOR('s',  1, int)		/* get high watermark */
#define	SIOCSLOWAT	 _IOW('s',  2, int)		/* set low watermark */
#define	SIOCGLOWAT	 _IOR('s',  3, int)		/* get low watermark */
#endif /* TCPV40HDRS */
#define	SIOCATMARK	 _IOR('s',  7, int)		/* at oob mark? */
#define	SIOCSPGRP	 _IOW('s',  8, int)		/* set process group */
#define	SIOCGPGRP	 _IOR('s',  9, int)		/* get process group */

#define	SIOCADDRT	 _IOW('r', 10, struct ortentry)	/* add route */
#define	SIOCDELRT	 _IOW('r', 11, struct ortentry)	/* delete route */
#if 0 /* not on OS/2 */
#define	SIOCGETVIFCNT	_IOWR('r', 15, struct sioc_vif_req)/* get vif pkt cnt */
#define	SIOCGETSGCNT	_IOWR('r', 16, struct sioc_sg_req) /* get s,g pkt cnt */
#endif

#define	SIOCSIFADDR	 _IOW('i', 12, struct ifreq)	/* set ifnet address */
#define	OSIOCGIFADDR	_IOWR('i', 13, struct ifreq)	/* get ifnet address */
#ifdef TCPV40HDRS
#define	SIOCGIFADDR	_IOWR('i', 13, struct ifreq)	/* get ifnet address */
#else
#define	SIOCGIFADDR	_IOWR('i', 33, struct ifreq)	/* get ifnet address */
#endif /* TCPV40HDRS */
#define	SIOCSIFDSTADDR	 _IOW('i', 14, struct ifreq)	/* set p-p address */
#define	OSIOCGIFDSTADDR	_IOWR('i', 15, struct ifreq)	/* get p-p address */
#ifdef TCPV40HDRS
#define	SIOCGIFDSTADDR	_IOWR('i', 15, struct ifreq)	/* get p-p address */
#else
#define	SIOCGIFDSTADDR	_IOWR('i', 34, struct ifreq)	/* get p-p address */
#endif /* TCPV40HDRS */
#define	SIOCSIFFLAGS	 _IOW('i', 16, struct ifreq)	/* set ifnet flags */
#define	SIOCGIFFLAGS	_IOWR('i', 17, struct ifreq)	/* get ifnet flags */
#define	OSIOCGIFBRDADDR	_IOWR('i', 18, struct ifreq)	/* get broadcast addr */
#ifdef TCPV40HDRS
#define	SIOCGIFBRDADDR	_IOWR('i', 18, struct ifreq)	/* get broadcast addr */
#else
#define	SIOCGIFBRDADDR	_IOWR('i', 35, struct ifreq)	/* get broadcast addr */
#endif /* TCPV40HDRS */
#define	SIOCSIFBRDADDR	 _IOW('i', 19, struct ifreq)	/* set broadcast addr */
#define	OSIOCGIFCONF	_IOWR('i', 20, struct ifconf)	/* get ifnet list */
#ifdef TCPV40HDRS
#define	SIOCGIFCONF	_IOWR('i', 20, struct ifconf)	/* get ifnet list */
#else
#define	SIOCGIFCONF	_IOWR('i', 36, struct ifconf)	/* get ifnet list */
#endif /* TCPV40HDRS */
#define	OSIOCGIFNETMASK	_IOWR('i', 21, struct ifreq)	/* get net addr mask */
#ifdef TCPV40HDRS
#define	SIOCGIFNETMASK	_IOWR('i', 21, struct ifreq)	/* get net addr mask */
#else
#define	SIOCGIFNETMASK	_IOWR('i', 37, struct ifreq)	/* get net addr mask */
#endif /* TCPV40HDRS */
#define	SIOCSIFNETMASK	 _IOW('i', 22, struct ifreq)	/* set net addr mask */
#define	SIOCGIFMETRIC	_IOWR('i', 23, struct ifreq)	/* get IF metric */
#define	SIOCSIFMETRIC	 _IOW('i', 24, struct ifreq)	/* set IF metric */

#if 1 /* different on OS/2 */
#define SIOCDIFADDR      _IOW('i', 64, struct ifreq)    /* delete IF addr */
#define SIOCAIFADDR      _IOW('i', 63, struct ifaliasreq)/* add/chg IF alias */
#else /*!OS2:*/
#define	SIOCDIFADDR	 _IOW('i', 25, struct ifreq)	/* delete IF addr */
#define	SIOCAIFADDR	 _IOW('i', 26, struct ifaliasreq)/* add/chg IF alias */
#endif

#if 0 /* not on OS/2 */
#define	SIOCALIFADDR	 _IOW('i', 27, struct if_laddrreq) /* add IF addr */
#define	SIOCGLIFADDR	_IOWR('i', 28, struct if_laddrreq) /* get IF addr */
#define	SIOCDLIFADDR	 _IOW('i', 29, struct if_laddrreq) /* delete IF addr */
#define	SIOCSIFCAP	 _IOW('i', 30, struct ifreq)	/* set IF features */
#define	SIOCGIFCAP	_IOWR('i', 31, struct ifreq)	/* get IF features */
#endif /*!OS2*/

#if 1 /* different on OS/2 */
#define SIOCADDMULTI     _IOW('i', 51, struct ifreq)    /* add m'cast addr */
#define SIOCDELMULTI     _IOW('i', 52, struct ifreq)    /* del m'cast addr */
#else /* !OS2: */
#define	SIOCADDMULTI	 _IOW('i', 49, struct ifreq)	/* add m'cast addr */
#define	SIOCDELMULTI	 _IOW('i', 50, struct ifreq)	/* del m'cast addr */
#endif

#if 1 /* different on OS/2 */
#define SIOCGIFMTU        _IOR('i', 57, struct ifreq)   /* get IF mtu */
#define SIOCSIFMTU        _IOW('i', 45, struct ifreq)   /* set IF mtu */
#else
#define	SIOCGIFMTU	_IOWR('i', 51, struct ifreq)	/* get IF mtu */
#define	SIOCSIFMTU	 _IOW('i', 52, struct ifreq)	/* set IF mtu */
#endif
#if 0 /* not on OS/2 */
#define	SIOCGIFPHYS	_IOWR('i', 53, struct ifreq)	/* get IF wire */
#define	SIOCSIFPHYS	 _IOW('i', 54, struct ifreq)	/* set IF wire */
#define	SIOCSIFMEDIA	_IOWR('i', 55, struct ifreq)	/* set net media */
#define	SIOCGIFMEDIA	_IOWR('i', 56, struct ifmediareq) /* get net media */

#define	SIOCSIFPHYADDR   _IOW('i', 70, struct ifaliasreq) /* set gif addres */
#define	SIOCGIFPSRCADDR	_IOWR('i', 71, struct ifreq)	/* get gif psrc addr */
#define	SIOCGIFPDSTADDR	_IOWR('i', 72, struct ifreq)	/* get gif pdst addr */
#define	SIOCDIFPHYADDR	 _IOW('i', 73, struct ifreq)	/* delete gif addrs */
#define	SIOCSLIFPHYADDR	 _IOW('i', 74, struct if_laddrreq) /* set gif addrs */
#define	SIOCGLIFPHYADDR	_IOWR('i', 75, struct if_laddrreq) /* get gif addrs */

#define	SIOCSIFGENERIC	 _IOW('i', 57, struct ifreq)	/* generic IF set op */
#define	SIOCGIFGENERIC	_IOWR('i', 58, struct ifreq)	/* generic IF get op */

#define	SIOCGIFSTATUS	_IOWR('i', 59, struct ifstat)	/* get IF status */
#define	SIOCSIFLLADDR	 _IOW('i', 60, struct ifreq)	/* set linklevel addr */

#define	SIOCGPRIVATE_0	_IOWR('i', 80, struct ifreq)	/* Linux Private + 0 */
#define	SIOCGPRIVATE_1	_IOWR('i', 81, struct ifreq)	/* Linux Private + 1 */

#define SIOCIFCREATE	_IOWR('i', 122, struct ifreq)	/* create clone if */
#define SIOCIFDESTROY	 _IOW('i', 121, struct ifreq)	/* destroy clone if */
#define SIOCIFGCLONERS	_IOWR('i', 120, struct if_clonereq) /* get cloners */
#endif /*!OS2*/

/* OS2 specific ioctls */
#define SIOCSHOSTID       _IOW('s', 10, long)
#define SIOCGNBNAME       _IOR('s', 11, long)   /* AFNB code. Not clear now */
#define SIOCSNBNAME       _IOW('s', 12, long)   /* AFNB                     */
#define SIOCGNCBFN        _IOR('s', 13, long)   /* AFNB                     */
#ifndef TCPV40HDRS
#define SIOCSSYN          _IOW('s', 14, long)   /* SYN Attack */
#endif /* !TCPV40HDRS */
#define SIOCSIFBRD        _IOW('i', 27, int)    /* SINGLE-rt bcst. using old # for bkw cmpt */
#ifdef TCPV40HDRS
#define SIOCSIFALLRTB      ioc('i', 63) /* added to configure all-route broadcst */
#else
#define SIOCSIFALLRTB     _IOW('i', 65, struct ifreq) /* added to configure all-route broadcst */
#endif /* TCPV40HDRS */

#define SIOCSARP          _IOW('i', 30, struct arpreq)/* set ARP entry */
#define SIOCGARP          _IOR('i', 31, struct arpreq)
#define SIOCDARP          _IOW('i', 32, struct arpreq)

#define SIOCSIF802_3      _IOW('i', 40, struct ifreq)
#define SIOCSIFNO802_3    _IOW('i', 41, struct ifreq)
#define SIOCSIFNOREDIR    _IOW('i', 42, struct ifreq)
#define SIOCSIFYESREDIR   _IOW('i', 43, struct ifreq)

#define SIOCSIFFDDI       _IOW('i', 46, struct ifreq)
#define SIOCSIFNOFDDI     _IOW('i', 47, struct ifreq)
#define SIOCSRDBRD        _IOW('i', 48, struct ifreq)

#define SIOCGARP_TR       _IOR('i', 50, struct arpreq_tr)
#define SIOCSARP_TR       _IOW('i', 49, struct arpreq_tr)

#if defined(SLBOOTP) || defined(INCL_TCPIP_ALLIOCTLS)
/** Used to retreive unit number on serial interface. */
#define SIOCGUNIT         _IOR('i', 70, struct ifreq)
#endif

/** To check if the interface is Valid or not */
#define SIOCGIFVALID      _IOR('i', 75, struct ifreq)
/** Ioctl to return bound/shld bind ifs */
struct bndreq { short bindings; short bound; };
#define SIOCGIFBOUND      _IOR('i', 76, struct bndreq)

/** Get multicast gp. info for an interface ret list of m-cast addrs for an if */
#define SIOCGMCAST        _IOR('i', 81, struct addrreq)
#define SIOCMULTISBC      _IOW('i', 61, struct ifreq)   /* use broadcast to send IP multicast*/
#define SIOCMULTISFA      _IOW('i', 62, struct ifreq)   /* use functional addr to send IP multicast*/

#ifndef TCPV40HDRS
/** block until intf change to running state */
#define SIOCSIFRUNNINGBLK _IOW('i', 77, struct ifreq)
#endif

/* Interface Tracing Support */
#define SIOCGIFEFLAGS     _IOR('i', 150, struct ifreq)
#define SIOCSIFEFLAGS     _IOW('i', 151, struct ifreq)
#define SIOCGIFTRACE      _IOR('i', 152, struct ifreq)
#define SIOCSIFTRACE      _IOW('i', 153, struct ifreq)

/* SLIP STATS */
#define SIOCSSTAT         _IOW('i', 154, struct ifreq)
#define SIOCGSTAT         _IOR('i', 155, struct ifreq)

#ifndef TCPV40HDRS
#define SIOCSMSL          _IOW('t', 1, long)          /* set the msl in seconds */
#define SIOCGMSL          _IOR('t', 2, long)          /* get the msl in seconds */
#endif

/* NETSTAT stuff */
#define SIOSTATMBUF       _IOR('n', 40, struct mbstat)
#define SIOSTATTCP        _IOR('n', 41, struct tcpstat)
#define SIOSTATUDP        _IOR('n', 42, struct udpstat)
#define SIOSTATIP         _IOR('n', 43, struct ipstat)
#define SIOSTATSO         _IOR('n', 44, char /*struct sockaddr*/)
#ifndef TCPV40HDRS
#define SIOSTATTCPZ       _IOR('n', 241, struct tcpstat)
#define SIOSTATUDPZ       _IOR('n', 242, struct udpstat)
#define SIOSTATIPZ        _IOR('n', 243, struct ipstat)
#endif

#define SIOSTATRT         _IOR('n', 45, char /*struct rtentries*/)
#define SIOFLUSHRT        _IOW('n', 46, long)                     /* Backward compatibility */
#define SIOSTATICMP       _IOR('n', 47, struct icmpstat)
#ifndef TCPV40HDRS
#define SIOSTATICMPZ      _IOR('n', 247, struct icmpstat)
#endif
#define SIOSTATIF         _IOR('n', 48, char /*struct ifmib*/)
#define SIOSTATAT         _IOR('n', 49, char /*struct statatreq*/)
#define SIOSTATARP        _IOR('n', 50, char /*struct oarptab*/)
#define SIOSTATIF42       _IOR('n', 51, char /*struct ifmib*/)
#define SIOSTATCNTRT      _IOR('n', 52, int)
#define SIOSTATCNTAT      _IOR('n', 53, int)

#ifndef TCPV40HDRS
#define SIOSTATIGMP       _IOR('n', 54, struct igmpstat)  /* SNMP stuff     */
#define SIOFLUSHRTIFP     _IOW('n', 55, long) /* delete routes on an interface */
#define SIOSTATIGMPZ      _IOR('n', 254, struct igmpstat) /* SNMP stuff     */

/** adding this ioctl() to be able to send arp request using an ioctl() */
#define SIOCARP           _IOR('i', 156, int)
#endif /* !TCPV40HDRS */

#if defined(TCPV40HDRS) || defined(INCL_TCPIP_ALLIOCTLS)
#define SIOMETRIC1RT    ioc('r', 12)
#define SIOMETRIC2RT    ioc('r', 13)
#define SIOMETRIC3RT    ioc('r', 14)
#define SIOMETRIC4RT    ioc('r', 15)
#define SIOCREGADDNET   ioc('r', 12)
#define SIOCREGDELNET   ioc('r', 13)
#define SIOCREGROUTES   ioc('r', 14)
#define SIOCFLUSHROUTES ioc('r', 15)
#define SIOCSIFSETSIG   ioc('i', 25)
#define SIOCSIFCLRSIG   ioc('i', 26)
#define SIOCGIFLOAD     ioc('i', 27)
#define SIOCSIFFILTERSRC ioc('i', 28)
#define SIOCGIFFILTERSRC ioc('i',29)
#define SIOCSIFSNMPSIG  ioc('i', 33)
#define SIOCSIFSNMPCLR  ioc('i', 34)
#define SIOCSIFSNMPCRC  ioc('i', 35)
#define SIOCSIFPRIORITY ioc('i', 36)
#define SIOCGIFPRIORITY ioc('i', 37)
#define SIOCSIFFILTERDST ioc('i', 38)
#define SIOCGIFFILTERDST ioc('i',39)
#define SIOCSIFSPIPE    ioc('i',71)   /* used to set pipe size on interface */
                                      /* this is used as tcp send buffer size */
#define SIOCSIFRPIPE    ioc('i',72)   /* used to set pipe size on interface */
                                      /* this is used as tcp recv buffer size */
#define SIOCSIFTCPSEG   ioc('i',73)   /* set the TCP segment size on interface*/
#define SIOCSIFUSE576   ioc('i',74)   /* enable/disable the automatic change of mss to 576 */
                                      /* if going through a router */
#endif  /* TCPV40HDRS || ALL */

#endif /* !_SYS_SOCKIO_H_ */
