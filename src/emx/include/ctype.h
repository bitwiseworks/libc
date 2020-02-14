/* ctype.h,v 1.14 2004/09/04 06:22:16 bird Exp */
/** @file
 *
 * InnoTek LIBC - Character type querying.
 *
 * Copyright (ch) 2004 knut st. osmundsen <bird@innotek.de>
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with This program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef _CTYPE_H_
#define	_CTYPE_H_

#include <sys/cdefs.h>
#include <_ctype.h>


__BEGIN_DECLS
int     isalnum(int);
int     isalpha(int);
int     iscntrl(int);
int     isdigit(int);
int     isgraph(int);
int     islower(int);
int     isprint(int);
int     ispunct(int);
int     isspace(int);
int     isupper(int);
int     isxdigit(int);
int     toupper(int);
int     tolower(int);

#if __XSI_VISIBLE
int     _toupper(int);
int     _tolower(int);
int     isascii(int);
int     toascii(int);
#endif

#if __ISO_C_VISIBLE >= 1999
int	isblank(int);
#endif

#if __BSD_VISIBLE
int	digittoint(int);
int	ishexnumber(int);
int	isideogram(int);
int	isnumber(int);
/* @todo int	isphonogram(int); */
int	isrune(int);
/* @todo int	isspecial(int); */
#endif
__END_DECLS

#ifndef __cplusplus
#define isalnum(ch)     __istype((ch), (__CT_ALPHA)|(__CT_DIGIT))
#define isalpha(ch)     __istype((ch), (__CT_ALPHA))
#define iscntrl(ch)     __istype((ch), (__CT_CNTRL))
#define isgraph(ch)     __istype((ch), (__CT_GRAPH))
#define islower(ch)     __istype((ch), (__CT_LOWER))
#define isprint(ch)     __istype((ch), (__CT_PRINT))
#define ispunct(ch)     __istype((ch), (__CT_PUNCT))
#define isspace(ch)     __istype((ch), (__CT_SPACE))
#define isupper(ch)     __istype((ch), (__CT_UPPER))
#ifdef __UNIX_CHAR_CLASS__ /* BSD and some other UNIXes have non-standard definitions of these two at least. */
#define isdigit(ch)     __isctype((ch),(__CT_DIGIT))
#define isxdigit(ch)    __isctype((ch),(__CT_XDIGIT))
#else
#define isdigit(ch)     __istype((ch), (__CT_DIGIT))
#define isxdigit(ch)    __istype((ch), (__CT_XDIGIT))
#endif
#define tolower(ch)     __tolower(ch)
#define toupper(ch)     __toupper(ch)
#endif /* !__cplusplus */

#if __XSI_VISIBLE
#define _toupper(ch)    __toupper(ch)
#define _tolower(ch)    __tolower(ch)
#define	isascii(ch)     (((ch) & ~0x7F) == 0)
#define	toascii(ch)     ((ch) & 0x7F)
#endif

#if __ISO_C_VISIBLE >= 1999 && !defined(__cplusplus)
#define isblank(ch)     __istype((ch), (__CT_BLANK))
#endif

#if __BSD_VISIBLE
#define ishexnumber(ch) __istype((ch), (__CT_XDIGIT))
#define isideogram(ch)  __istype((ch), (__CT_IDEOGRAM))
#define isnumber(ch)    __istype((ch), (__CT_DIGIT))
#define	isrune(ch)      __istype((ch), ~(__CT_NUM_MASK))
#define	digittoint(ch)  __ctype((ch), (__CT_NUM_MASK))
#endif

#endif
