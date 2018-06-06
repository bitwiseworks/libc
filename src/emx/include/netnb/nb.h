/* Copyright (C)1996 by Holger Veit
 * This file may be freely used within the EMX development system
 */

/* NETBIOS declarations */

#ifndef _NETNB_NB_H_
#define _NETNB_NB_H_

#include <sys/socket.h>
#include <netinet/in.h>

#define	NBPROTO_NB	1
#define	NB_FAMILY	18 /* temporary hack? */
#define	NB_ADDRSIZE	sizeof(struct sockaddr_nb)
#define	NB_UNIQUE	0
#define	NB_GROUP	1
#define	NB_BROAD	2
#define	NB_NAMELEN	16
#define	NB_NETIDLEN	8
#define	NB_HOSTLEN	12
#define	NB_PORTLEN	(NB_NAMELEN - NB_HOSTLEN)
/*                       1234567890123456 */
#define	NB_BCAST_NAME	"*               "
#define	NB_BLANK_NAME	"                "
#define NB_NULL_NAME	"\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"

/* must match IOPORT_* constants in netinet/in.h! */
#ifdef TCPV40HDRS
#define	NBPORT_RESERVED	        "1024"
#define	NBPORT_USERRESERVED     "5000"
#else
#define NBPORT_RESERVED         "49152"
#define NBPORT_USERRESERVED     "65535"
#endif

/* address format for netbios */
#pragma pack(1)
struct sockaddr_nb {
#ifdef TCPV40HDRS
	short	snb_family;		/* protocol family */
#else
        u_char  snb_len;
        u_char  snb_family;             /* netbios protocol family */
#endif
	short	snb_type;		/* unique/multicast */
	char	snb_netid[NB_NETIDLEN];	/* netid */
	char	snb_name[NB_NAMELEN];	/* name */
};
#pragma pack()

#endif /* !_NETNB_NB_H_ */

