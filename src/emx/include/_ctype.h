/* $Id: _ctype.h 3789 2012-03-22 20:53:05Z bird $ */
/** @file
 *
 * _ctype.h - common header for ctype.h and wctype.h.
 *
 * Copyright (c) 2004 knut st. osmundsen <bird@anduin.net>
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

#ifndef __CTYPE_H_
#define	__CTYPE_H_

#include <sys/cdefs.h>
#include <sys/_types.h>

#if !defined(__InnoTekLIBC_locale_h__)
__BEGIN_DECLS
/**
 * Simplified Ctype structures for inline functions.
 */
extern struct
{
    /** All uppercased characters. */
    unsigned char   auchUpper[256];
    /** All lowercased characters. */
    unsigned char   auchLower[256];
    /** Bit flags for every character (for isXXX() function series). */
    unsigned        aufType[256];
} __libc_GLocaleCtype;

/**
 * Simplified Default Ctype structures for inline functions.
 */
extern const struct
{
    /** All uppercased characters. */
    unsigned char   auchUpper[256];
    /** All lowercased characters. */
    unsigned char   auchLower[256];
    /** Bit flags for every character (for isXXX() function series). */
    unsigned        aufType[256];
} __libc_GLocaleCtypeDefault;

/**
 * Unicode CType data.
 * The structure contains information for the first 256 unicode chars.
 */
extern const struct __libc_localeWCType
{
    /** All uppercased characters. */
    __wchar_t       awcUpper[256];
    /** All lowercased characters. */
    __wchar_t       awcLower[256];
    /** Bit flags for every character (for iswXXX() function series). */
    unsigned        aufType[256];
    /** Mask used to check if an index is within the above arrays.
     * This is required because 'C' doesn't do more than 0-127. So,
     * the mask is either ~0xff or ~0x7f. */
    unsigned        uMask;
} __libc_GLocaleWCtype;
__END_DECLS
#endif /* !__InnoTekLIBC_locale_h__ */


/** Bit masks for the aufType member of __libc_GLocaleCtype
 *  and __libc_GLocaleCtypeDefault
 *
 * @remark These have values identical values to the CT_* << 8 to speed up
 *         setlocale() and to make us compatible with the BSD scheme. In addition
 *         values C3_IDEOGRAPH and the C2_* << 24 are converted into the value. The
 *         screen width stuff is attempted derived from other info provided by the
 *         unicode libraries.
 * @{
 */
#define __CT_NUM_MASK   0x000000ffU     /** Numberic value for digit to int conversion. */

#define __CT_UPPER      0x00000100U     /** Upper case alphabetic character. */
#define __CT_LOWER      0x00000200U     /** Lower case alphabetic character. */
#define __CT_DIGIT      0x00000400U     /** Digits 0-9. */
#define __CT_SPACE      0x00000800U     /** White space and line ends. */
#define __CT_PUNCT      0x00001000U     /** Punctuation marks. */
#define __CT_CNTRL      0x00002000U     /** Control and format characters. */
#define __CT_BLANK      0x00004000U     /** Space and tab. */
#define __CT_XDIGIT     0x00008000U     /** Hex digits. */
#define __CT_ALPHA      0x00010000U     /** Letters and linguistic marks. */
#define __CT_ALNUM      0x00020000U     /** Alphanumeric - obsolete (__CT_ALPHA | __CT_DIGIT) is the same. */
#define __CT_GRAPH      0x00040000U     /** All except controls and space. */
#define __CT_PRINT      0x00080000U     /** Everything except controls. */
#define __CT_NUMBER     0x00100000U     /** Integral number. */
#define __CT_SYMBOL     0x00200000U     /** Symbol. */
#define __CT_ASCII      0x00800000U     /** In standard ASCII set. */

#define __CT_IDEOGRAM   0x00400000U     /** Ideogram. (C3_IDEOGRAPH?) (Unused/known CT_ bit.) */

/* #define __CT_BIDI_MASK  0x0f000000U - not implemented */     /** Bidi mask C2_*. */

#define __CT_SCRW0      0x20000000U     /** 0 width character. (Just indicator for SW data when width is 0.) */
#define __CT_SCRW1      0x40000000U     /** 1 width character. */
#define __CT_SCRW2      0x80000000U     /** 2 width character. */
#define __CT_SCRW3      0xc0000000U     /** 3 width character. */
#define __CT_SCRW_MASK  0xe0000000U     /** Mask for screen width data. */
#define __CT_SCRW_SHIFT 30              /** Bits to shift to get width. */

/** BSD compatability.
 * @{ */
#define _CTYPE_A        __CT_ALPHA
#define _CTYPE_C        __CT_CNTRL
#define _CTYPE_D        __CT_DIGIT
#define _CTYPE_G        __CT_GRAPH
#define _CTYPE_L        __CT_LOWER
#define _CTYPE_P        __CT_PUNCT
#define _CTYPE_S        __CT_SPACE
#define _CTYPE_U        __CT_UPPER
#define _CTYPE_X        __CT_XDIGIT
#define _CTYPE_B        __CT_BLANK
#define _CTYPE_R        __CT_PRINT
#define _CTYPE_I        __CT_IDEOGRAM
#if 0 /* We don't have this info :/ */
#define _CTYPE_T    	0x00100000L		/* Special */
#define _CTYPE_Q	0x00200000L		/* Phonogram */
#endif
#define _CTYPE_SW0      __CT_SCRW0
#define _CTYPE_SW1      __CT_SCRW1
#define _CTYPE_SW2      __CT_SCRW2
#define _CTYPE_SW3      __CT_SCRW3
#define _CTYPE_SWM      __CT_SCRW_MASK
#define _CTYPE_SWS      __CT_SCRW_SHIFT
/** @} */
/** @} */


/** Functions for handling runes outside the 0-255 range.
 * @{ */
__BEGIN_DECLS
unsigned ___wctype(__wchar_t);
__wchar_t ___towlower(__wchar_t);
__wchar_t ___towupper(__wchar_t);
__END_DECLS
/** @} */


#if !defined(_DONT_USE_CTYPE_INLINE_) && \
    (defined(_USE_CTYPE_INLINE_) || defined(__GNUC__) || defined(__cplusplus))

/** Inlined function versions.
 * @{
 */
__BEGIN_DECLS
static __inline__ unsigned __ctype(__ct_rune_t __ch, unsigned __f)
{
    return __ch < 256 && __ch >= -128 && __ch != -1 /*EOF*/
        ? __libc_GLocaleCtype.aufType[__ch & 0xff] & __f
        : 0;
}

static __inline__ int __istype(__ct_rune_t __ch, unsigned __f)
{
    return !!__ctype(__ch, __f);
}

static __inline__ int __isctype(__ct_rune_t __ch, unsigned __f)
{
    return __ch <= 255 && __ch >= -128 && __ch != -1 /*EOF*/
        ? !!(__libc_GLocaleCtypeDefault.aufType[__ch & 0xff] & __f)
        : 0;
}

static __inline__ __ct_rune_t __toupper(__ct_rune_t __ch)
{
    return __ch <= 255 && __ch >= -128 && __ch != -1 /*EOF*/
        ? __libc_GLocaleCtype.auchUpper[__ch & 0xff]
        : (__ch);
}

static __inline__ __ct_rune_t __tolower(__ct_rune_t __ch)
{
    return __ch <= 255 && __ch >= -128 && __ch != -1 /*EOF*/
        ? __libc_GLocaleCtype.auchLower[__ch & 0xff]
        : __ch;
}


static __inline__ unsigned __wctype(__wint_t __wc, unsigned __f)
{
    return !(__wc & __libc_GLocaleWCtype.uMask)
        ? __libc_GLocaleWCtype.aufType[__wc] & __f
        : ___wctype(__wc) & __f;
}

static __inline__ int __iswtype(__wint_t __wc, unsigned __f)
{
    return !!__wctype(__wc, __f);
}

static __inline__ __wint_t __towupper(__wint_t __wc)
{
    return !(__wc & __libc_GLocaleWCtype.uMask)
        ? __libc_GLocaleWCtype.awcUpper[__wc]
        : ___towupper(__wc);
}

static __inline__ __wint_t __towlower(__wint_t __wc)
{
    return !(__wc & __libc_GLocaleWCtype.uMask)
        ? __libc_GLocaleWCtype.awcLower[__wc]
        : ___towlower(__wc);
}

static __inline__ int __wcwidth(__wint_t __wc)
{
    if (__wc != 0)
    {
        unsigned __f = __wctype(__wc, __CT_SCRW_MASK | __CT_PRINT);
        return (__f & __CT_SCRW_MASK)
            ? (__f & __CT_SCRW_MASK) >> __CT_SCRW_SHIFT
            : (__f & __CT_PRINT) ? 1 : -1;
    }
    else
        return 0;
}
__END_DECLS
/** @} */

#else

/** Non-inlined function versions.
 * @{
 */
__BEGIN_DECLS
unsigned    __ctype(__ct_rune_t, unsigned);
int         __istype(__ct_rune_t, unsigned);
__ct_rune_t __isctype(__ct_rune_t, unsigned);
__ct_rune_t __toupper(__ct_rune_t);
int         __tolower(__ct_rune_t);

unsigned    __wctype(__wint_t, unsigned);
int         __iswtype(__wint_t, unsigned);
__wint_t    __towupper(__wint_t);
__wint_t    __towlower(__wint_t);
int         __wcwidth(__wint_t);
__END_DECLS
/** @} */

#endif

#endif /* !__CTYPE_H_ */
