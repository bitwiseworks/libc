/* $Id: isxxx.c 1705 2004-12-06 02:19:54Z bird $ */
/** @file
 *
 * ctype and wctype simple query functions.
 *
 * Copyright (c) 2004 knut st. osmundsen <bird-srcspam@anduin.net>
 *
 *
 * This file is part of InnoTek LIBC.
 *
 * InnoTek LIBC is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * InnoTek LIBC is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with InnoTek LIBC; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include "libc-alias.h"
#include <sys/cdefs.h>
#define _DONT_USE_CTYPE_INLINE_
#include <ctype.h>
#include <wctype.h>

int     (isalnum)(int ch)       { return isalnum(ch); }
int     (isalpha)(int ch)       { return isalpha(ch); }
int     (iscntrl)(int ch)       { return iscntrl(ch); }
int     (isdigit)(int ch)       { return isdigit(ch); }
int     (isgraph)(int ch)       { return isgraph(ch); }
int     (islower)(int ch)       { return islower(ch); }
int     (isprint)(int ch)       { return isprint(ch); }
int     (ispunct)(int ch)       { return ispunct(ch); }
int     (isspace)(int ch)       { return isspace(ch); }
int     (isupper)(int ch)       { return isupper(ch); }
int     (isxdigit)(int ch)      { return isxdigit(ch); }
int     (toupper)(int ch)       { return toupper(ch); }
int     (tolower)(int ch)       { return tolower(ch); }
int     (_toupper)(int ch)      { return _toupper(ch); }
int     (_tolower)(int ch)      { return _toupper(ch); }
int     (isascii)(int ch)       { return isascii(ch); }
int     (toascii)(int ch)       { return isascii(ch); }
int	(digittoint)(int ch)    { return digittoint(ch); }
int     (isblank)(int ch)       { return isblank(ch); }
int	(ishexnumber)(int ch)   { return ishexnumber(ch); }
int	(isideogram)(int ch)    { return isideogram(ch); }
int	(isnumber)(int ch)      { return isnumber(ch); }
int	(isrune)(int ch)        { return isrune(ch); }

#if !defined(isalnum) \
 || !defined(isalpha) \
 || !defined(iscntrl) \
 || !defined(isdigit) \
 || !defined(isgraph) \
 || !defined(islower) \
 || !defined(isprint) \
 || !defined(ispunct) \
 || !defined(isspace) \
 || !defined(isupper) \
 || !defined(isxdigit) \
 || !defined(toupper) \
 || !defined(tolower) \
 || !defined(_toupper) \
 || !defined(_tolower) \
 || !defined(isascii) \
 || !defined(toascii) \
 || !defined(digittoint) \
 || !defined(isblank) \
 || !defined(ishexnumber) \
 || !defined(isideogram) \
 || !defined(isnumber) \
 || !defined(isrune)
#error "One or more of the ctype.h macros are undefined!"
#endif


int	(iswalnum)(int wc)       { return iswalnum(wc); }
int	(iswalpha)(int wc)       { return iswalpha(wc); }
int	(iswblank)(int wc)       { return iswblank(wc); }
int	(iswcntrl)(int wc)       { return iswcntrl(wc); }
int	(iswctype)(wint_t wc, wctype_t wctype)    { return iswctype(wc, wctype); }
int	(iswdigit)(int wc)       { return iswdigit(wc); }
int	(iswgraph)(int wc)       { return iswgraph(wc); }
int	(iswlower)(int wc)       { return iswlower(wc); }
int	(iswprint)(int wc)       { return iswprint(wc); }
int	(iswpunct)(int wc)       { return iswpunct(wc); }
int	(iswspace)(int wc)       { return iswspace(wc); }
int	(iswupper)(int wc)       { return iswupper(wc); }
int	(iswxdigit)(int wc)      { return iswxdigit(wc); }
wint_t	(towlower)(int wc)       { return towlower(wc); }
wint_t	(towupper)(int wc)       { return towupper(wc); }
wint_t	(iswascii)(int wc)       { return iswascii(wc); }
wint_t	(iswhexnumber)(int wc)   { return iswhexnumber(wc); }
wint_t	(iswideogram)(int wc)    { return iswideogram(wc); }
wint_t	(iswnumber)(int wc)      { return iswnumber(wc); }
wint_t	(iswrune)(int wc)        { return iswrune(wc); }

#if !defined(iswalnum) \
 || !defined(iswalpha) \
 || !defined(iswblank) \
 || !defined(iswcntrl) \
 || !defined(iswctype) \
 || !defined(iswdigit) \
 || !defined(iswgraph) \
 || !defined(iswlower) \
 || !defined(iswprint) \
 || !defined(iswpunct) \
 || !defined(iswspace) \
 || !defined(iswupper) \
 || !defined(iswxdigit) \
 || !defined(towlower) \
 || !defined(towupper) \
 || !defined(iswascii) \
 || !defined(iswhexnumber) \
 || !defined(iswideogram) \
 || !defined(iswnumber) \
 || !defined(iswrune)
#error "One or more of the wctype.h macros are undefined!"
#endif

