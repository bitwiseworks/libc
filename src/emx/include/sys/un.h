/*
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
 *	@(#)un.h	8.3 (Berkeley) 2/19/95
 * $FreeBSD: src/sys/sys/un.h,v 1.25 2002/08/21 16:20:01 mike Exp $
 */

/** @file
 * FreeBSD 5.1
 *
 * @changed bird: TCPV40HDRS and hid LOCAL_PEERCRED.
 * @todo    Investigate the LOCAL_PEERCRED #define.
 */

#ifndef _SYS_UN_H_
#define _SYS_UN_H_

#include <sys/cdefs.h>
#include <sys/_types.h>

#ifndef _SA_FAMILY_T_DECLARED
typedef	__sa_family_t	sa_family_t;
#define	_SA_FAMILY_T_DECLARED
#endif

/*
 * Definitions for UNIX IPC domain.
 */
struct sockaddr_un {
#ifdef TCPV40HDRS                       /* bird: old tcpip header mode */
	u_short	sun_family;	        /* socket family: AF_UNIX */
#else
	unsigned char	sun_len;	/* sockaddr len including null */
	sa_family_t	sun_family;	/* AF_UNIX */
#endif
	char	sun_path[108];		/* path name (gag) */
};

#if __BSD_VISIBLE

#if 0                                   /* bird: don't expose unsupported stuff. */
/* Socket options. */
#define LOCAL_PEERCRED		0x001		/* retrieve peer credentails */
#endif

#ifdef _KERNEL
struct mbuf;
struct socket;
struct sockopt;

int	uipc_ctloutput(struct socket *so, struct sockopt *sopt);
int	uipc_usrreq(struct socket *so, int req, struct mbuf *m,
		struct mbuf *nam, struct mbuf *control);
int	unp_connect2(struct socket *so, struct socket *so2);
void	unp_dispose(struct mbuf *m);
int	unp_externalize(struct mbuf *mbuf, struct mbuf **controlp);
void	unp_init(void);
extern	struct pr_usrreqs uipc_usrreqs;

#else /* !_KERNEL */

/* actual length of an initialized sockaddr_un */
#define SUN_LEN(su) \
	(sizeof(*(su)) - sizeof((su)->sun_path) + strlen((su)->sun_path))

#endif /* _KERNEL */

#endif /* __BSD_VISIBLE */

#endif /* !_SYS_UN_H_ */
