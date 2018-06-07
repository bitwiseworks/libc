/* $Id; $ */
/** @file
 * FreeBSD 5.1
 * @changed bird: removed unsupported values and added comments to lconv.
 * @changed bird: Changed the LC_* values to match those unidef.h sets.
 */

/*
 * Copyright (c) 1991, 1993
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
 *	@(#)locale.h	8.1 (Berkeley) 6/2/93
 * $FreeBSD: src/include/locale.h,v 1.7 2002/10/09 09:19:27 tjr Exp $
 */

#ifndef _LOCALE_H_
#define _LOCALE_H_

struct lconv {
	char	*decimal_point;         /** non-monetary decimal point */
	char	*thousands_sep;         /** non-monetary thousands separator */
	char	*grouping;              /** non-monetary size of grouping */
	char	*int_curr_symbol;       /** international currency symbol and separator */
	char	*currency_symbol;       /** local currency symbol */
	char	*mon_decimal_point;     /** monetary decimal point */
	char	*mon_thousands_sep;     /** monetary thousands separator */
	char	*mon_grouping;          /** monetary size of grouping */
	char	*positive_sign;         /** non-negative values sign */
	char	*negative_sign;         /** negative values sign */
	char	int_frac_digits;        /** number of fractional digits - int currency */
	char	frac_digits;            /** number of fractional digits - local currency */
	char	p_cs_precedes;          /** (non-neg curr sym) 1-precedes, 0-succeeds */
	char	p_sep_by_space;         /** (non-neg curr sym) 1-space, 0-no space */
	char	n_cs_precedes;          /** (neg curr sym) 1-precedes, 0-succeeds */
	char	n_sep_by_space;         /** (neg curr sym) 1-space, 0-no space */
	char	p_sign_posn;            /** positioning of non-negative monetary sign */
	char	n_sign_posn;            /** positioning of negative monetary sign */
	char	int_p_cs_precedes;
	char	int_n_cs_precedes;
	char	int_p_sep_by_space;
	char	int_n_sep_by_space;
	char	int_p_sign_posn;
	char	int_n_sign_posn;
};

#ifndef NULL
#define	NULL	0
#endif

#define	LC_ALL		(-1) /* bird: was 0 */
#define	LC_COLLATE	0    /* bird: was 1 */
#define	LC_CTYPE	1    /* bird: was 2 */
#define	LC_NUMERIC	2    /* bird: was 4 */
#define	LC_MONETARY	3    /* bird: was 3 */
#define	LC_TIME		4    /* bird: was 5 */
#define	LC_MESSAGES	5    /* bird: was 6 */

#define	_LC_LAST	6    /* bird: was 7 */		/* marks end */

#include <sys/cdefs.h>

__BEGIN_DECLS
struct lconv	*localeconv(void);
char		*setlocale(int, const char *);
__END_DECLS

#endif /* !_LOCALE_H_ */
