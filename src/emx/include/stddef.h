/*-
 * Copyright (c) 1990, 1993
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
 *	@(#)stddef.h	8.1 (Berkeley) 6/2/93
 *
 * $FreeBSD: src/include/stddef.h,v 1.9 2002/09/01 17:57:20 mike Exp $
 */

/** @file
 * FreeBSD 5.1
 * @changed bird: EMXifications.
 */

#ifndef _STDDEF_H_
#define _STDDEF_H_

#include <sys/cdefs.h>
#include <sys/_types.h>

typedef	__ptrdiff_t	ptrdiff_t;

#if __BSD_VISIBLE
#ifndef _RUNE_T_DECLARED
typedef	__rune_t	rune_t;
#define	_RUNE_T_DECLARED
#endif
#endif

#if !defined(_SIZE_T_DECLARED) && !defined(_SIZE_T) /* bird: emx */
typedef	__size_t	size_t;
#define	_SIZE_T_DECLARED
#define	_SIZE_T                         /* bird: emx */
#endif

#ifndef	__cplusplus
#if !defined(_WCHAR_T_DECLARED) && !defined(_WCHAR_T)
typedef	__wchar_t	wchar_t;
#define	_WCHAR_T_DECLARED
#define	_WCHAR_T                        /* bird: emx */
#endif
#endif

#ifndef	NULL
#define	NULL	0
#endif

#define	offsetof(type, member)	__offsetof(type, member)


/* bird: EMX - start */
#define _PTRDIFF_T
#if (!defined (__STRICT_ANSI__) && !defined (_POSIX_SOURCE)) || defined (_WITH_UNDERSCORE) || defined(__USE_EMX)
unsigned *__threadid (void);
#define _threadid (__threadid ())

#endif
/* bird: EMX - end */

#endif /* _STDDEF_H_ */
