/* Modified for emx by hv 1994,1996
 * Modified for gcc/os2 by bird 2003
 *
 * Copyright (c) 1983, 1989 Regents of the University of California.
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
 *	from: @(#)nameser.h	5.25 (Berkeley) 4/3/91
 *	$Id: nameser.h,v 1.4 1993/10/27 00:10:40 mycroft Exp $
 */

#ifndef _NAMESER_H_
#define	_NAMESER_H_

#if defined (__cplusplus)
extern "C" {
#endif

#include <sys/param.h>          /* htons() etc. */
#include <machine/endian.h>	/* BYTE ORDER etc. */
#ifndef TCPV40HDRS
#include "../types.h" /* #include <types.h> frequently includes the wrong header. */
#include <sys/cdefs.h>
#endif

/*
 * Define constants based on rfc883
 */
#define PACKETSZ	512		/* maximum packet size */
#ifdef TCPV40HDRS
#define MAXDNAME	256		/* maximum domain name */
#else
#define MAXDNAME        1025            /* maximum presentation domain name */
#endif
#define MAXCDNAME	255		/* maximum compressed domain name */
#define MAXLABEL	63		/* maximum length of domain label */
	/* Number of bytes of fixed size data in query structure */
#define QFIXEDSZ	4
	/* number of bytes of fixed size data in resource record */
#define RRFIXEDSZ	10
#ifndef TCPV40HDRS
#define HFIXEDSZ        12              /* #/bytes of fixed data in header */
#define RRFIXEDSZ       10              /* #/bytes of fixed data in r record */
#define INT32SZ         4               /* for systems without 32-bit ints */
#define INT16SZ         2               /* for systems without 16-bit ints */
#define INADDRSZ        4               /* IPv4 T_A */
#define IN6ADDRSZ       16              /* IPv6 T_AAAA */
#endif

/*
 * Internet nameserver port number
 */
#define NAMESERVER_PORT	53

/*
 * Currently defined opcodes
 */
#define QUERY		0x0		/* standard query */
#define IQUERY		0x1		/* inverse query */
#ifdef TCPV40HDRS
#define CQUERYM         2               /* completion query (multiple) */
#define CQUERYU         3               /* completion query (unique) */
#endif
#define STATUS		0x2		/* nameserver status query */
/*#define xxx		0x3		   0x3 reserved */
	/* non standard */
#ifdef TCPV40HDRS
/* IBM has changed these codes in TCP/IP for OS/2 */
#define UPDATEA		100		/* add resource record */
#define UPDATED		101		/* delete a specific resource record */
#define UPDATEM		102		/* modify a specific resource record */
#define ZONEINIT	103		/* initial zone transfer */
#define ZONEREF		104		/* incremental zone referesh */
#else
#define NS_NOTIFY_OP    0x4             /* notify secondary of SOA change */
#endif

/*
 * Currently defined response codes
 */
#define NOERROR		0		/* no error */
#define FORMERR		1		/* format error */
#define SERVFAIL	2		/* server failure */
#define NXDOMAIN	3		/* non existent domain */
#define NOTIMP		4		/* not implemented */
#define REFUSED		5		/* query refused */
	/* non standard */
#ifdef TCPV40HDRS
#define NOCHANGE	100		/* update failed to change db */
#endif

/*
 * Type values for resources and queries
 */
#define T_A		1		/* host address */
#define T_NS		2		/* authoritative server */
#define T_MD		3		/* mail destination */
#define T_MF		4		/* mail forwarder */
#define T_CNAME		5		/* connonical name */
#define T_SOA		6		/* start of authority zone */
#define T_MB		7		/* mailbox domain name */
#define T_MG		8		/* mail group member */
#define T_MR		9		/* mail rename name */
#define T_NULL		10		/* null resource record */
#define T_WKS		11		/* well known service */
#define T_PTR		12		/* domain name pointer */
#define T_HINFO		13		/* host information */
#define T_MINFO		14		/* mailbox information */
#define T_MX		15		/* mail routing information */
#ifndef TCPV40HDRS
#define T_TXT		16		/* text strings */
#define T_RP            17              /* responsible person */
#define T_AFSDB         18              /* AFS cell database */
#define T_X25           19              /* X_25 calling address */
#define T_ISDN          20              /* ISDN calling address */
#define T_RT            21              /* router */
#define T_NSAP          22              /* NSAP address */
#define T_NSAP_PTR      23              /* reverse NSAP lookup (deprecated) */
#define T_SIG           24              /* security signature */
#define T_KEY           25              /* security key */
#define T_PX            26              /* X.400 mail mapping */
#define T_GPOS          27              /* geographical position (withdrawn) */
#define T_AAAA          28              /* IP6 Address */
#define T_LOC           29              /* Location Information */
#define T_NXT           30              /* Next Valid Name in Zone */
#define T_EID           31              /* Endpoint identifier */
#define T_NIMLOC        32              /* Nimrod locator */
#define T_SRV           33              /* Server selection */
#define T_ATMA          34              /* ATM Address */
#define T_NAPTR         35              /* Naming Authority PoinTeR */
#endif
	/* non standard */
#define T_UINFO		100		/* user (finger) information */
#define T_UID		101		/* user ID */
#define T_GID		102		/* group ID */
#ifndef TCPV40HDRS
#define T_UNSPEC	103		/* Unspecified format (binary data) */
#endif
	/* Query type values which do not appear in resource records */
#ifndef TCPV40HDRS
#define T_IXFR          251             /* incremental zone transfer */
#endif
#define T_AXFR		252		/* transfer zone of authority */
#define T_MAILB		253		/* transfer mailbox records */
#define T_MAILA		254		/* transfer mail agent records */
#define T_ANY		255		/* wildcard match */

/*
 * Values for class field
 */
#define C_IN		1		/* the arpa internet */
#define C_CHAOS		3		/* for chaos net at MIT */
#ifndef TCPV40HDRS
#define C_HS		4		/* for Hesiod name server at MIT */
#endif
	/* Query class values which do not appear in resource records */
#define C_ANY		255		/* wildcard match */

/*
 * Flags field of the KEY RR rdata
 */
#define KEYFLAG_TYPEMASK        0xC000  /* Mask for "type" bits */
#define KEYFLAG_TYPE_AUTH_CONF  0x0000  /* Key usable for both */
#define KEYFLAG_TYPE_CONF_ONLY  0x8000  /* Key usable for confidentiality */
#define KEYFLAG_TYPE_AUTH_ONLY  0x4000  /* Key usable for authentication */
#define KEYFLAG_TYPE_NO_KEY     0xC000  /* No key usable for either; no key */
/* The type bits can also be interpreted independently, as single bits: */
#define KEYFLAG_NO_AUTH         0x8000  /* Key not usable for authentication */
#define KEYFLAG_NO_CONF         0x4000  /* Key not usable for confidentiality */

#define KEYFLAG_EXPERIMENTAL    0x2000  /* Security is *mandatory* if bit=0 */
#define KEYFLAG_RESERVED3       0x1000  /* reserved - must be zero */
#define KEYFLAG_RESERVED4       0x0800  /* reserved - must be zero */
#define KEYFLAG_USERACCOUNT     0x0400  /* key is assoc. with a user acct */
#define KEYFLAG_ENTITY          0x0200  /* key is assoc. with entity eg host */
#define KEYFLAG_ZONEKEY         0x0100  /* key is zone key for the zone named */
#define KEYFLAG_IPSEC           0x0080  /* key is for IPSEC use (host or user)*/
#define KEYFLAG_EMAIL           0x0040  /* key is for email (MIME security) */
#define KEYFLAG_RESERVED10      0x0020  /* reserved - must be zero */
#define KEYFLAG_RESERVED11      0x0010  /* reserved - must be zero */
#define KEYFLAG_SIGNATORYMASK   0x000F  /* key can sign DNS RR's of same name */

#define  KEYFLAG_RESERVED_BITMASK ( KEYFLAG_RESERVED3 | \
                                    KEYFLAG_RESERVED4 | \
                                    KEYFLAG_RESERVED10| KEYFLAG_RESERVED11)

/* The Algorithm field of the KEY and SIG RR's is an integer, {1..254} */
#define ALGORITHM_MD5RSA        1       /* MD5 with RSA */
#define ALGORITHM_EXPIRE_ONLY   253     /* No alg, no security */
#define ALGORITHM_PRIVATE_OID   254     /* Key begins with OID indicating alg */

/* Signatures */
                                        /* Size of a mod or exp in bits */
#define MIN_MD5RSA_KEY_PART_BITS         512
#define MAX_MD5RSA_KEY_PART_BITS        2552
                                        /* Total of binary mod and exp, bytes */
#define MAX_MD5RSA_KEY_BYTES            ((MAX_MD5RSA_KEY_PART_BITS+7/8)*2+3)
                                        /* Max length of text sig block */
#define MAX_KEY_BASE64                  (((MAX_MD5RSA_KEY_BYTES+2)/3)*4)

#ifndef TCPV40HDRS
/*
 * Status return codes for T_UNSPEC conversion routines
 */
#define CONV_SUCCESS 0
#define CONV_OVERFLOW -1
#define CONV_BADFMT -2
#define CONV_BADCKSUM -3
#define CONV_BADBUFLEN -4
#endif

/*
 * Structure for query header, the order of the fields is machine and
 * compiler dependent, in our case, the bits within a byte are assignd
 * least significant first, while the order of transmition is most
 * significant first.  This requires a somewhat confusing rearrangement.
 */
#ifndef TCPV40HDRS
#pragma pack(1)
#endif
typedef struct {
	u_short	id;		/* query identification number */
#if BYTE_ORDER == BIG_ENDIAN
			/* fields in third byte */
	u_char	qr:1;		/* response flag */
	u_char	opcode:4;	/* purpose of message */
	u_char	aa:1;		/* authoritive answer */
	u_char	tc:1;		/* truncated message */
	u_char	rd:1;		/* recursion desired */
			/* fields in fourth byte */
	u_char	ra:1;		/* recursion available */
#ifdef TCPV40HDRS
	u_char	pr:1;		/* primary server required (non standard) */
	u_char	unused:2;	/* unused bits */
#else
        u_char  unused :1;      /* unused bits (MBZ as of 4.9.3a3) */
        u_char  ad: 1;          /* authentic data from named */
        u_char  cd: 1;          /* checking disabled by resolver */
#endif
	u_char	rcode:4;	/* response code */
#endif
#if BYTE_ORDER == LITTLE_ENDIAN || BYTE_ORDER == PDP_ENDIAN
			/* fields in third byte */
	u_char	rd:1;		/* recursion desired */
	u_char	tc:1;		/* truncated message */
	u_char	aa:1;		/* authoritive answer */
	u_char	opcode:4;	/* purpose of message */
	u_char	qr:1;		/* response flag */
			/* fields in fourth byte */
	u_char	rcode:4;	/* response code */
#ifdef TCPV40HDRS
	u_char	unused:2;	/* unused bits */
	u_char	pr:1;		/* primary server required (non standard) */
#else
        u_char  cd: 1;          /* checking disabled by resolver */
        u_char  ad: 1;          /* authentic data from named */
        u_char  unused :1;      /* unused bits (MBZ as of 4.9.3a3) */
#endif
	u_char	ra:1;		/* recursion available */
#endif
			/* remaining bytes */
	u_short	qdcount;	/* number of question entries */
	u_short	ancount;	/* number of answer entries */
	u_short	nscount;	/* number of authority entries */
	u_short	arcount;	/* number of resource entries */
} HEADER;
#ifndef TCPV40HDRS
#pragma pack()
#endif

/*
 * Defines for handling compressed domain names
 */
#define INDIR_MASK	0xc0

#ifdef TCPV40HDRS
/*
 * Structure for passing resource records around.
 */
#pragma pack(4)
struct rrec {
	short	r_zone;			/* zone number */
	short	r_class;		/* class number */
	short	r_type;			/* type number */
	u_long	r_ttl;			/* time to live */
	int	r_size;			/* size of data area */
	char	*r_data;		/* pointer to data */
};
#pragma pack()
#endif

#ifndef TCPV40HDRS
/*
 * Inline versions of get/put short/long.  Pointer is advanced.
 *
 * These macros demonstrate the property of C whereby it can be
 * portable or it can be elegant but rarely both.
 */
#define GETSHORT(s, cp) { \
        register u_char *t_cp = (u_char *)(cp); \
        (s) = ((u_int16_t)t_cp[0] << 8) \
            | ((u_int16_t)t_cp[1]) \
            ; \
        (cp) += INT16SZ; \
}

#define GETLONG(l, cp) { \
        register u_char *t_cp = (u_char *)(cp); \
        (l) = ((u_int32_t)t_cp[0] << 24) \
            | ((u_int32_t)t_cp[1] << 16) \
            | ((u_int32_t)t_cp[2] << 8) \
            | ((u_int32_t)t_cp[3]) \
            ; \
        (cp) += INT32SZ; \
}

#define PUTSHORT(s, cp) { \
        register u_int16_t t_s = (u_int16_t)(s); \
        register u_char *t_cp = (u_char *)(cp); \
        *t_cp++ = t_s >> 8; \
        *t_cp   = t_s; \
        (cp) += INT16SZ; \
}

#define PUTLONG(l, cp) { \
        register u_int32_t t_l = (u_int32_t)(l); \
        register u_char *t_cp = (u_char *)(cp); \
        *t_cp++ = t_l >> 24; \
        *t_cp++ = t_l >> 16; \
        *t_cp++ = t_l >> 8; \
        *t_cp   = t_l; \
        (cp) += INT32SZ; \
}
#endif /*!TCPV40HDRS*/


/* function prototypes */
u_short TCPCALL _getshort(u_char*);
u_long	TCPCALL _getlong(u_char*);
#ifdef TCPV40HDRS /* newer have this in resolver */
void	TCPCALL putshort(u_short, u_char *);
void	TCPCALL putlong(u_long, u_char *);
int	TCPCALL dn_expand(const u_char*, const u_char*, const u_char*, u_char*, int);
int	TCPCALL dn_comp(const u_char*, u_char*, int, u_char**, u_char**);
int	TCPCALL dn_find(u_char*, u_char*, u_char**, u_char**);
int	TCPCALL dn_skipname(const u_char*, const u_char*);
int	TCPCALL res_init(void);
int	TCPCALL res_mkquery(int, const char*, int, int, const char*, int, const struct rrec*, char*, int);
int	TCPCALL res_send(const char*, int, char*, int);
int	TCPCALL res_query(char*, int, int, u_char*, int);
int	TCPCALL res_search(char*, int, int, u_char*, int);
int	TCPCALL res_querydomain(char*, char*, int, int, u_char*, int);
#endif

#if defined (__cplusplus)
}
#endif

#endif /* !_NAMESER_H_ */
