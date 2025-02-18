/*
 * Copyright (c) 1987, 1991 Regents of the University of California.
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
 *	@(#)endian.h	7.8 (Berkeley) 4/3/91
 * $FreeBSD: src/sys/i386/include/endian.h,v 1.35 2002/09/23 04:32:16 mike Exp $
 */

/** @file
 * FreeBSD 5.1
 * @changed bird: Mixed in older stuff for toolkit compatibility. Not sure how successfull that is.
 */

#ifndef _MACHINE_ENDIAN_H_
#define _MACHINE_ENDIAN_H_

#include <sys/cdefs.h>
#include <sys/_types.h>

/*
 * Define the order of 32-bit words in 64-bit words.
 */
#define _QUAD_HIGHWORD 1
#define _QUAD_LOWWORD 0

/*
 * Definitions for byte order, according to byte significance from low
 * address to high.
 */
#define	_LITTLE_ENDIAN	1234	/* LSB first: i386, vax */
#define	_BIG_ENDIAN	4321	/* MSB first: 68000, ibm, net */
#define	_PDP_ENDIAN	3412	/* LSB first in word, MSW first in long */

#define	_BYTE_ORDER	_LITTLE_ENDIAN

/*
 * Deprecated variants that don't have enough underscores to be useful in more
 * strict namespaces.
 */
#if __BSD_VISIBLE
#define	LITTLE_ENDIAN	_LITTLE_ENDIAN
#define	BIG_ENDIAN	_BIG_ENDIAN
#define	PDP_ENDIAN	_PDP_ENDIAN
#define	BYTE_ORDER	_BYTE_ORDER
#endif

#ifdef __GNUC__

#define __word_swap_int(x) \
__extension__ ({ register __uint32_t __X = (x); \
   __asm ("rorl $16, %0" : "+r" (__X)); \
   __X; })

#if defined(_KERNEL) && (defined(I486_CPU) || defined(I586_CPU) || defined(I686_CPU)) && !defined(I386_CPU)

#define __byte_swap_int(x) \
__extension__ ({ register __uint32_t __X = (x); \
   __asm ("bswap %0" : "+r" (__X)); \
   __X; })
#else

#define __byte_swap_int(x) \
__extension__ ({ register __uint32_t __X = (x); \
   __asm ("xchgb %h0, %b0\n\trorl $16, %0\n\txchgb %h0, %b0" \
       : "+q" (__X)); \
   __X; })
#endif

#define __byte_swap_word(x) \
__extension__ ({ register __uint16_t __X = (x); \
   __asm ("xchgb %h0, %b0" : "+q" (__X)); \
   __X; })

static __inline __uint64_t
__bswap64(__uint64_t _x)
{
	return ((_x >> 56) | ((_x >> 40) & 0xff00) | ((_x >> 24) & 0xff0000) |
	    ((_x >> 8) & 0xff000000) | ((_x << 8) & ((__uint64_t)0xff << 32)) |
	    ((_x << 24) & ((__uint64_t)0xff << 40)) |
	    ((_x << 40) & ((__uint64_t)0xff << 48)) | ((_x << 56)));
}

static __inline __uint32_t
__bswap32(__uint32_t _x)
{

	return (__byte_swap_int(_x));
}

static __inline __uint16_t
__bswap16(__uint16_t _x)
{

	return (__byte_swap_word(_x));
}

#define	__htonl(x)	__bswap32(x)
#define	__htons(x)	__bswap16(x)
#define	__ntohl(x)	__bswap32(x)
#define	__ntohs(x)	__bswap16(x)

#else /* !__GNUC__ */

/*
 * No optimizations are available for this compiler.  Fall back to
 * non-optimized functions by defining the constant usually used to prevent
 * redefinition.
 */
#define	_BYTEORDER_FUNC_DEFINED

#endif /* __GNUC__ */


/* bird:
 * For toolkit compatibility we must do this stuff here.
 * Not sure how correct this is now. And mixing 4 versions
 * of (Free)BSD doesn't help....
 */
#if !defined(_POSIX_SOURCE) || defined(__USE_EMX)

#ifndef _KERNEL
#include <sys/cdefs.h>
#endif

__BEGIN_DECLS
unsigned long  _System htonl __P((unsigned long));
unsigned short _System htons __P((unsigned short));
unsigned long  _System ntohl __P((unsigned long));
unsigned short _System ntohs __P((unsigned short));
__END_DECLS

#ifndef _BYTEORDER_FUNC_DEFINED
#define _BYTEORDER_FUNC_DEFINED
#define htonl(x)   __htonl(x)
#define htons(x)   __htons(x)
#define ntohl(x)   __ntohl(x)
#define ntohs(x)   __ntohs(x)
#endif

/*
 * Macros for network/external number representation conversion.
 */
#define NTOHL(x)   ((x) = ntohl((u_long)(x)))
#define NTOHS(x)   ((x) = ntohs((u_short)(x)))
#define HTONL(x)   ((x) = htonl((u_long)(x)))
#define HTONS(x)   ((x) = htons((u_short)(x)))

#endif /* !_POSIX_SOURCE || __USE_EMX */


#endif /* _MACHINE_ENDIAN_H_ */
