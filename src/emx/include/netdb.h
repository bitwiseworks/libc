/* netdb.h,v 1.8 2004/09/14 22:27:34 bird Exp */
/** @file
 * BSD
 *
 * @changed Modified for emx by hv and em 1994-1997
 * @changed Modified for gcc by bird 2003
 */

/* netdb.h,v 1.8 2004/09/14 22:27:34 bird Exp */
/*-
 *
 * Copyright (c) 1980, 1983, 1988 Regents of the University of California.
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
 *	from: @(#)netdb.h	5.15 (Berkeley) 4/3/91
 *	netdb.h,v 1.8 2004/09/14 22:27:34 bird Exp
 */

#ifndef _NETDB_H_
#define _NETDB_H_
#ifdef TCPV40HDRS
#define __NETDB_32H
#endif

#if defined (__cplusplus)
extern "C" {
#endif

/* toolkit compatibility - start */
#include <sys/param.h>
#include <sys/cdefs.h>
/* toolkit compatibility - end */

#ifndef _EMX_TCPIP
#define _EMX_TCPIP
#endif

#ifdef __RSXNT__    /* convert some int values to short */
#define __INT_T short
#pragma pack(1)
#else /* emx */
#define __INT_T int
#endif


#define	_PATH_HEQUIV	"/@unixroot/etc/hosts.equiv"
#define	_PATH_HOSTS	"/@unixroot/etc/hosts"
#define	_PATH_NETWORKS	"/@unixroot/etc/networks"
#define	_PATH_PROTOCOLS	"/@unixroot/etc/protocols"
#define	_PATH_SERVICES	"/@unixroot/etc/services"

#ifdef TCPV40HDRS
#define h_errno         (tcp_h_errno())   /* __MT__ safe */
#else
#define h_errno         (*tcp_h_errno1()) /* __MT__ safe */
#endif

/*
 * Structures returned by network data base library.  All addresses are
 * supplied in host order, and returned in network order (suitable for
 * use in system calls).
 */
struct	hostent {
	char	*h_name;	/* official name of host */
	char	**h_aliases;	/* alias list */
	__INT_T	h_addrtype;	/* host address type */
	__INT_T	h_length;	/* length of address */
	char	**h_addr_list;	/* list of addresses from name server */
#define	h_addr	h_addr_list[0]	/* address, for backward compatiblity */
};

/*
 * Assumption here is that a network number
 * fits in 32 bits -- probably a poor one.
 */
struct	netent {
	char		*n_name;	/* official name of net */
	char		**n_aliases;	/* alias list */
	__INT_T		n_addrtype;	/* net address type */
	unsigned long	n_net;		/* network # */
};

struct	servent {
	char	*s_name;	/* official service name */
	char	**s_aliases;	/* alias list */
	__INT_T	s_port;		/* port # */
	char	*s_proto;	/* protocol to use */
};

struct	protoent {
	char	*p_name;	/* official protocol name */
	char	**p_aliases;	/* alias list */
	__INT_T	p_proto;	/* protocol # */
};

/*
 * Error return codes from gethostbyname() and gethostbyaddr()
 * (left in extern int h_errno).
 */
#ifndef TCPV40HDRS
#define NETDB_INTERNAL  -1      /* see errno */
#define NETDB_SUCCESS   0       /* no problem */
#endif
#define	HOST_NOT_FOUND	1 /* Authoritative Answer Host not found */
#define	TRY_AGAIN	2 /* Non-Authoritive Host not found, or SERVERFAIL */
#define	NO_RECOVERY	3 /* Non recoverable errors, FORMERR, REFUSED, NOTIMP */
#define	NO_DATA		4 /* Valid name, no data record of requested type */
#define	NO_ADDRESS	NO_DATA		/* no address, look for MX record */


#ifdef TCPV40HDRS
#include <stdio.h>
#include <string.h>
#include <netinet\in.h>

/* IBM addendum structures and functions */
#define	_HOSTBUFSIZE	(4096+1)
#define	_MAXADDRS	35
#define	_MAXALIASES	35
#define	_MAXLINELEN	1024

/* this is the internally used data structure to which pointers of
 * struct hostent ponit to, after gethostbyname_r/gethostbyaddr_r were
 * called. ATTENTION EMX: The component _opaque_ is a pointer to a data
 * structure which is not accessible by EMX
 */
struct hostent_data {
	struct in_addr	host_addr;		/* host address pointer */
	char	*h_addr_ptrs[_MAXADDRS+1];	/* host address */
	char	hostaddr[_MAXADDRS];
	char	hostbuf[_HOSTBUFSIZE+1];	/* host data */
	char	*host_aliases[_MAXALIASES];
	char	*host_addrs[2];
	void	*_opaque_;			/* EMX: dead component! */
	int	stayopen;			/* AIX addon */
	u_long	host_addresses[_MAXADDRS];	/* Actual Addresses. */
};

struct servent_data {
	void	*_opaque_;
	char	line[_MAXLINELEN];
	char	*serv_aliases[_MAXALIASES];
	int	_serv_stayopen;
};
#endif


/* BSD */
int                 TCPCALL gethostname( char *, int );
struct hostent *    TCPCALL gethostbyname( const char * );
struct hostent *    TCPCALL gethostbyaddr( const char *, int, int );
struct netent *     TCPCALL getnetbyname( const char * );
struct netent *     TCPCALL getnetbyaddr( unsigned long, int );
struct servent *    TCPCALL getservbyname( const char *, const char * );
struct servent *    TCPCALL getservbyport( int, const char * );
struct servent *    TCPCALL getservent( void );
struct protoent *   TCPCALL getprotobyname( const char * );
struct protoent *   TCPCALL getprotobynumber( int );
void                TCPCALL sethostent( int );
struct hostent *    TCPCALL gethostent( void );
void                TCPCALL endhostent(void);
void                TCPCALL setnetent( int );
struct netent *     TCPCALL getnetent( void );
void                TCPCALL endnetent(void);
void                TCPCALL setprotoent( int );
struct protoent *   TCPCALL getprotoent( void );
void                TCPCALL endprotoent(void);
void                TCPCALL setservent( int );
struct servent *    TCPCALL getservent( void );
void                TCPCALL endservent(void);
#ifndef TCPV40HDRS
struct hostent *    TCPCALL gethostbyname2( const char *, int );
#endif

/* New BSD Additions. */
int		innetgr(const char *, const char *, const char *, const char *);
void		setnetgrent(const char *);
int		getnetgrent(char **, char **, char **);
void		endnetgrent(void);



/* OS2 Additions */
#ifdef TCPV40HDRS
int                 TCPCALL tcp_h_errno(void);
struct hostent *    TCPCALL Rgethostbyname(char *); /* Socks additions */
#else
const char *        TCPCALL hstrerror(int);
/* void               TCPCALL sethostfile(const char *); */
int *               TCPCALL tcp_h_errno1(void);
#endif

/* EMX/BSD additions. */
void		    TCPCALL herror(const char *);

/* EMX/BSD stuff which isn't implemeneted  */
#ifndef TCPV40HDRS
struct hostent_data;
struct servent_data;
#endif
int                 gethostbyname_r(char*, struct hostent*, struct hostent_data*);
int                 gethostbyaddr_r(char*, int, int, struct hostent*, struct hostent_data*);
int                 getservbyname_r(char*, char*, struct servent*, struct servent_data*);
struct hostent *    _gethtbyname(char*);
struct hostent *    _gethtbyaddr(char*, int, int);


#ifdef __RSXNT__
#pragma pack()
#endif
#undef __INT_T

#if defined (__cplusplus)
}
#endif

#endif /* !_NETDB_H_ */

