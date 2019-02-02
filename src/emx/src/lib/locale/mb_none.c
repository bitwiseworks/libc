/*-
 * Copyright (c) 2002-2004 Tim J. Robbins. All rights reserved.
 * Copyright (c) 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Paul Borman at Krystal Technologies.
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
 */

#include "libc-alias.h"
#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)none.c	8.1 (Berkeley) 6/4/93";
#endif /* LIBC_SCCS and not lint */
#include <sys/cdefs.h>
__FBSDID("$FreeBSD: src/lib/libc/locale/none.c,v 1.12 2004/07/21 10:54:57 tjr Exp $");

#include <errno.h>
#include <limits.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <InnoTekLIBC/locale.h>
#include <wchar.h>

int
__libc_locale_none_mbsinit(const mbstate_t *ps __unused)
{

    /*
     * Encoding is not state dependent - we are always in the
     * initial state.
     */
    return(1);
}

size_t
__libc_locale_none_mbrtowc(wchar_t * __restrict pwc, const char * __restrict s, size_t n,
              mbstate_t * __restrict ps __unused)
{

    if (s == NULL)
        /* Reset to initial shift state (no-op) */
        return(0);
    if (n == 0)
        /* Incomplete multibyte sequence */
        return((size_t)-2);
    if (pwc != NULL)
        *pwc = (unsigned char)*s;
    return(*s == '\0' ? 0 : 1);
}

size_t
__libc_locale_none_wcrtomb(char * __restrict s, wchar_t wc,
              mbstate_t * __restrict ps __unused)
{

    if (s == NULL)
        /* Reset to initial shift state (no-op) */
        return(1);
    if (/*wc < 0 ||*/ (unsigned)wc > UCHAR_MAX)
    {
        errno = EILSEQ;
        return((size_t)-1);
    }
    *s = (unsigned char)wc;
    return(1);
}

size_t
__libc_locale_none_mbsnrtowcs(wchar_t * __restrict dst, const char * __restrict * __restrict src,
                 size_t nms, size_t len, mbstate_t * __restrict ps __unused)
{
    const char *s;
    size_t nchr;

    if (dst == NULL)
    {
        s = memchr(*src, '\0', nms);
        return(s != NULL ? s - *src : nms);
    }

    s = *src;
    nchr = 0;
    while (len-- > 0 && nms-- > 0)
    {
        if ((*dst++ = (unsigned char)*s++) == L'\0')
        {
            *src = NULL;
            return(nchr);
        }
        nchr++;
    }
    *src = s;
    return(nchr);
}

size_t
__libc_locale_none_wcsnrtombs(char * __restrict dst, const wchar_t * __restrict * __restrict src,
                 size_t nwc, size_t len, mbstate_t * __restrict ps __unused)
{
    const wchar_t *s;
    size_t nchr;

    if (dst == NULL)
    {
        for (s = *src; nwc > 0 && *s != L'\0'; s++, nwc--)
        {
            if (/**s < 0 ||*/ (unsigned)*s > UCHAR_MAX)
            {
                errno = EILSEQ;
                return((size_t)-1);
            }
        }
        return(s - *src);
    }

    s = *src;
    nchr = 0;
    while (len-- > 0 && nwc-- > 0)
    {
        if (/**s < 0 ||*/ (unsigned)*s > UCHAR_MAX)
        {
            errno = EILSEQ;
            return((size_t)-1);
        }
        if ((*dst++ = *s++) == '\0')
        {
            *src = NULL;
            return(nchr);
        }
        nchr++;
    }
    *src = s;
    return(nchr);
}

