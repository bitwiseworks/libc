/* Modified for emx by hv and em 1994
 * Modified for gcc/os2 by bird 2003
 *
 * Copyright (c) 1983 Regents of the University of California.
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
 *	from: @(#)inet.h	5.7 (Berkeley) 4/3/91
 *	$Id: inet.h,v 1.2 1993/08/01 18:46:22 mycroft Exp $
 */

#ifndef _INET_H_
#define	_INET_H_

#if defined (__cplusplus)
extern "C" {
#endif


/* External definitions for functions in inet(3) */

#include <sys/param.h>
#include <sys/types.h>
#include <sys/cdefs.h>

/*
 * Internet address (a structure for historical reasons)
 */
#ifndef _STRUCT_IN_ADDR_DECLARED
struct in_addr {
	u_long s_addr;
};
#define _STRUCT_IN_ADDR_DECLARED
#endif

#ifndef TCPV40HDRS
int             TCPCALL inet_aton (const char *, struct in_addr *);
char *          TCPCALL inet_neta (u_long, char *, size_t);
char *          TCPCALL inet_net_ntop (int, const void *, int, char *, size_t);
int             TCPCALL inet_net_pton (int, const char *, void *, size_t);
int             TCPCALL inet_pton (int af, const char *src, void *dst);
const char *    TCPCALL inet_ntop (int af, const void *src, char *dst, size_t s);
u_int           TCPCALL inet_nsap_addr (const char *, u_char *, int maxlen);
char *          TCPCALL inet_nsap_ntoa (int, const u_char *, char *ascii);
#endif

extern u_long   TCPCALL inet_addr(const char*);
extern u_long	TCPCALL inet_lnaof(struct in_addr);
extern struct in_addr TCPCALL inet_makeaddr(u_long, u_long);
extern u_long   TCPCALL inet_netof(struct in_addr);
extern u_long   TCPCALL inet_network(const char*);
extern char*    TCPCALL inet_ntoa(struct in_addr);

#if defined (__cplusplus)
}
#endif

#endif /* !_INET_H_ */
