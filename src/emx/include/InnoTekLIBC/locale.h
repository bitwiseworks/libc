/* $Id: locale.h 3055 2007-04-08 17:11:36Z bird $ */
/** @file
 *
 * Internal InnoTek LIBC header.
 * Locale support implementation through OS/2 Unicode API.
 *
 * Copyright (c) 2003 InnoTek Systemberatung GmbH
 * Copyright (c) 2003-2004 knut st. osmundsen <bird-srcspam@anduin.net>
 *
 *
 * This file is part of InnoTek LIBC.
 *
 * InnoTek LIBC is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * InnoTek LIBC is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with InnoTek LIBC; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef __InnoTekLIBC_locale_h__
#define __InnoTekLIBC_locale_h__

#include <sys/cdefs.h>
#include <sys/_types.h>
#include <locale.h>
#include <uconv.h>

__BEGIN_DECLS

/**
 * LC_COLLATE information.
 */
typedef struct __libc_LocaleCollate
{
    /** Character weight for SBCS codepages. */
    unsigned char   auchWeight[256];
    /** MBCS prefixes. Two bits per character. */
    unsigned char   au2MBCSPrefixs[256/4];
#ifdef __OS2__
    /** The converter object to convert to and from selected codepage
      (used with MBCS codepages only). */
    UconvObject     uobj;
    /** The locale object. */
    LocaleObject    lobj;
#endif
    /** Non-zero if there are any MBCS prefix characters in codepage. */
    char            mbcs;
} __LIBC_LOCALECOLLATE;
/** Pointer to locale collate structure. */
typedef __LIBC_LOCALECOLLATE *__LIBC_PLOCALECOLLATE;

/**
 * Multibyte to/from wide character conversion functions.
 */
typedef struct __libc_localeCTypeFuncs
{
    int     (*pfnmbsinit)(const __mbstate_t *);
    size_t  (*pfnmbrtowc)(__wchar_t * __restrict, const char * __restrict, size_t, __mbstate_t * __restrict);
    size_t  (*pfnmbsnrtowcs)(__wchar_t * __restrict, const char ** __restrict, size_t, size_t, __mbstate_t * __restrict);
    size_t  (*pfnwcrtomb)(char * __restrict, __wchar_t, __mbstate_t * __restrict);
    size_t  (*pfnwcsnrtombs)(char * __restrict, const __wchar_t ** __restrict, size_t, size_t, __mbstate_t * __restrict);
} __LIBC_LOCALECTYPEFUNCS;
/** Pointer to multibyte/wide character conversion functions. */
typedef __LIBC_LOCALECTYPEFUNCS *__LIBC_PLOCALECTYPEFUNCS;
/** Pointer to const multibyte/wide character conversion functions. */
typedef const __LIBC_LOCALECTYPEFUNCS *__LIBC_PCLOCALECTYPEFUNCS;

/**
 * This structure contains the flags and uppercase/lowercase tables.
 */
typedef struct __libc_LocaleCtype
{
    /** All uppercased characters. */
    unsigned char           auchUpper[256];
    /** All lowercased characters. */
    unsigned char           auchLower[256];
    /** Bit flags for every character (for isXXX() function series). */
    unsigned                aufType[256];

/* part which we don't 'expose': */
    /** MBCS prefixes. Two bits per character. */
    unsigned char           au2MBCSPrefixs[256/4];
    /** Unicode translation. (0xffff means no translation.) */
    unsigned short          aucUnicode[256];
    /** Unicode -> SBCS conversion: 0..128. */
    unsigned char           auchToSBCS0To128[128];
    /** Unicode -> SBCS conversion: Custom regions. */
    struct
    {
        /** First unicode code point. */
        unsigned short      usStart;
        /** Number of entries used. */
        unsigned short      cChars;
        /** Array SBCS chars corresponding to (wc - usStart). 0 means no conversion. */
        unsigned char       auch[28];
    }                       aSBCSs[8];
    /** Number of aSBCS regions in use. */
    unsigned                cSBCSs;
    /** Conversion functions. */
    __LIBC_LOCALECTYPEFUNCS CtypeFuncs;
#ifdef __OS2__
    /** The converter object to convert to and from selected codepage
      (used with MBCS codepages only). */
    UconvObject             uobj;
    /** The locale object. */
    LocaleObject            lobj;
#endif
    /** Non-zero if there are any MBCS prefix characters in codepage. */
    char                    mbcs;
    /** Codeset name. */
    char                    szCodeSet[32];
} __LIBC_LOCALECTYPE;
/** Pointer to the Ctype locale struct. */
typedef __LIBC_LOCALECTYPE *__LIBC_PLOCALECTYPE;


/**
 * Unicode CType data.
 * The structure contains information for the first 256 unicode chars.
 */
typedef struct __libc_localeWCType
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
} __LIBC_LOCALEWCTYPE;
/** Pointer to the Ctype unicode struct. */
typedef __LIBC_LOCALEWCTYPE *__LIBC_PLOCALEWCTYPE;

/**
 * This structure keeps the time formatting rules.
 * The fConsts flag indicates what kind of memory is backing the strings.
 */
typedef struct __libc_LocaleTime
{
    /** Short month names. */
    char   *smonths[12];
    /** Long month names. */
    char   *lmonths[12];
    /** Short weekday names. */
    char   *swdays[7];
    /** Long weekday names. */
    char   *lwdays[7];
    /** Date and time format. */
    char   *date_time_fmt;
    /** Date format. */
    char   *date_fmt;
    /** Time format. */
    char   *time_fmt;
    /** AM strings. */
    char   *am;
    /** PM strings. */
    char   *pm;
    /** AM/PM format. (T_FMT_AMPM) */
    char   *ampm_fmt;
    /** ERA */
    char   *era;
    /** ERA_D_FMT. */
    char   *era_date_fmt;
    /** ERA_D_T_FMT. */
    char   *era_date_time_fmt;
    /** ERA_T_FMT. */
    char   *era_time_fmt;
    /** ALT_DIGITS. */
    char   *alt_digits;
    /** DATESEP. */
    char   *datesep;
    /** TIMESEP. */
    char   *timesep;
    /** LISTSEP. */
    char   *listsep;
    /** If set all the strings are consts and shall not be free()ed. */
    int     fConsts;
} __LIBC_LOCALETIME;
/** Pointer to time locale data. */
typedef __LIBC_LOCALETIME *__LIBC_PLOCALETIME;


/**
 * Locale information structure.
 *
 * This is the lconv struct with a couple of private field indicating
 * which parts of it we have updated and assigned heap strings.
 */
typedef struct __libc_localeLconv
{
    /** The lconv structure. */
    struct lconv    s;
    /** CRNCYSTR. */
    char           *pszCrncyStr;
    /** Indicates that all the numeric members are readonly const strings. */
    int             fNumericConsts;
    /** Indicates that all the monetary members are readonly const strings. */
    int             fMonetaryConsts;
} __LIBC_LOCALELCONV;
/** Pointer to extended locale information structure. */
typedef __LIBC_LOCALELCONV *__LIBC_PLOCALELCONV;


/**
 * Message locale information.
 * The content is available thru the nl_langinfo() interface only.
 */
typedef struct __libc_localeMsg
{
    /** YESEXPR */
    char           *pszYesExpr;
    /** NOEXPR */
    char           *pszNoExpr;
    /** YESSTR */
    char           *pszYesStr;
    /** NOSTR */
    char           *pszNoStr;
    /** Indicates that all members are readonly const strings. */
    int             fConsts;
} __LIBC_LOCALEMSG;
/** Pointer to the message locale information. */
typedef __LIBC_LOCALEMSG *__LIBC_PLOCALEMSG;


/** String collation information. */
extern __LIBC_LOCALECOLLATE         __libc_gLocaleCollate;
/** String collation information for the default 'C'/'POSIX' locale. */
extern const __LIBC_LOCALECOLLATE   __libc_gLocaleCollateDefault;
/** Character case conversion tables. */
extern __LIBC_LOCALECTYPE           __libc_GLocaleCtype;
/** Character case conversion tables for the default 'C'/'POSIX' locale. */
extern const __LIBC_LOCALECTYPE     __libc_GLocaleCtypeDefault;
/** Cached Unicode (__wchar_t) case conversion tables and flags. */
extern __LIBC_LOCALEWCTYPE          __libc_GLocaleWCtype;
/** Locale information structure. */
extern __LIBC_LOCALELCONV           __libc_gLocaleLconv;
/* Locale information structure for the 'C'/'POSIX' locale. */
extern const __LIBC_LOCALELCONV     __libc_gLocaleLconvDefault;
/** Date / time formatting rules. */
extern __LIBC_LOCALETIME            __libc_gLocaleTime;
/** Date / time formatting rules for the 'C'/'POSIX' locale. */
extern const __LIBC_LOCALETIME      __libc_gLocaleTimeDefault;
/** Message locale information. */
extern __LIBC_LOCALEMSG             __libc_gLocaleMsg;
/** Message locale information for the 'C'/'POSIX' locale. */
extern const __LIBC_LOCALEMSG       __libc_gLocaleMsgDefault;

/** Macros to lock the different locale structures.
 * @{
 */
#define LOCALE_LOCK()               do {} while (0)
#define LOCALE_UNLOCK()             do {} while (0)
#define LOCALE_CTYPE_RW_LOCK()      do {} while (0)
#define LOCALE_CTYPE_RW_UNLOCK()    do {} while (0)
#define LOCALE_CTYPE_RW_LOCK()      do {} while (0)
#define LOCALE_CTYPE_RW_UNLOCK()    do {} while (0)
/** @} */

/** Convert a string to Unicode, apply some transform and convert back. */
extern void __libc_ucs2Do(UconvObject *uconv, char *s, void *arg, int (*xform)(UniChar *, void *));
/** Convert a MBCS character to Unicode; returns number of bytes in MBCS char. */
extern int  __libc_ucs2To(UconvObject, const unsigned char *, size_t, UniChar *);
/** Convert a Unicode character to MBCS. */
extern int  __libc_ucs2From(UconvObject, UniChar, unsigned char *, size_t);
/** Converts a codepage string to unichar and something libuni might recognize. */
extern void __libc_TranslateCodepage(const char *cp, UniChar *ucp);

extern int __libc_localeCreateObjects(const char *pszLocale, const char *pszCodepage, char *pszCodepageActual, LocaleObject *plobj, UconvObject *puobj);

extern void __libc_localeFuncsSBCS(__LIBC_PLOCALECTYPEFUNCS pFuncs);
extern void __libc_localeFuncsDBCS(__LIBC_PLOCALECTYPEFUNCS pFuncs);
extern void __libc_localeFuncsMBCS(__LIBC_PLOCALECTYPEFUNCS pFuncs);
extern void __libc_localeFuncsUCS2(__LIBC_PLOCALECTYPEFUNCS pFuncs);
extern void __libc_localeFuncsUTF8(__LIBC_PLOCALECTYPEFUNCS pFuncs);
extern void __libc_localeFuncsDefault(__LIBC_PLOCALECTYPEFUNCS pFuncs);

extern size_t  __libc_localeFuncsGeneric_mbsnrtowcs(size_t  (*pfnmbrtowc)(__wchar_t * __restrict, const char * __restrict, size_t, __mbstate_t * __restrict),
                                                    __wchar_t * __restrict dst, const char ** __restrict src, size_t nms, size_t len, __mbstate_t * __restrict ps);
extern size_t  __libc_localeFuncsGeneric_wcsnrtombs(size_t  (*pfnwcrtomb)(char * __restrict, __wchar_t, __mbstate_t * __restrict),
                                                    char * __restrict dst, const __wchar_t ** __restrict src, size_t nwc, size_t len, __mbstate_t * __restrict ps);

extern void     __libc_localeFuncsNone(__LIBC_PLOCALECTYPEFUNCS pFuncs);
extern size_t   __libc_locale_none_mbrtowc(__wchar_t * __restrict, const char * __restrict, size_t, __mbstate_t * __restrict);
extern int      __libc_locale_none_mbsinit(const __mbstate_t *);
extern size_t   __libc_locale_none_mbsnrtowcs(__wchar_t * __restrict dst, const char ** __restrict src, size_t nms, size_t len, __mbstate_t * __restrict ps __unused);
extern size_t   __libc_locale_none_wcrtomb(char * __restrict, __wchar_t, __mbstate_t * __restrict);
extern size_t   __libc_locale_none_wcsnrtombs(char * __restrict, const __wchar_t ** __restrict, size_t, size_t, __mbstate_t * __restrict);


/** Handy macros for working with the au2MBCSPrefixs members of
 * the locale data structures. The au2MBCSPrefixs members are
 * array which elements are 2 bits long.
 * @{
 */
#define SET_MBCS_PREFIX(a, c, v) \
    a[((unsigned char)(c)) >> 2] |= (v) << (2 * ((c) & 3))

#define LEN_MBCS_PREFIX(a, c) \
    ((a[((unsigned char)(c)) >> 2] >> (2 * (((c) & 3) ^ 3))) & 3)

#define IS_MBCS_PREFIX(p, c) \
    (LEN_MBCS_PREFIX((p)->au2MBCSPrefixs, c) != 1)

#define CHK_MBCS_PREFIX(p, c, v) \
    ((v = LEN_MBCS_PREFIX((p)->au2MBCSPrefixs, c)) > 1)
/** @} */

#include <ctype.h>

/**
 * Convert the type info we get from the unicode lib to libc talk.
 * ASSUMES that none of the locals differs from the unicode spec
 *
 * @returns libc ctype flags.
 * @param   pUniType    The unicode type info to translate.
 * @param   wc          The unicode code point.
 */
static inline unsigned ___wctype_uni(const UNICTYPE *pUniType, wchar_t wc)
{
    unsigned    ufType = 0;
    /* ASSUMES CT_* << 8 == __* ! */
    ufType = ((unsigned)pUniType->itype << 8)
           & (__CT_UPPER  | __CT_LOWER  | __CT_DIGIT | __CT_SPACE |
              __CT_PUNCT  | __CT_CNTRL  | __CT_BLANK | __CT_XDIGIT |
              __CT_ALPHA  | __CT_ALNUM  | __CT_GRAPH | __CT_PRINT |
              __CT_NUMBER | __CT_SYMBOL | __CT_ASCII);
    if (pUniType->extend & C3_IDEOGRAPH)
        ufType |= __CT_IDEOGRAM;
    if (ufType & (__CT_XDIGIT | __CT_DIGIT))
    {
        if (     (unsigned)wc - 0x30U <= (0x39 - 0x30))
            ufType |= (unsigned)wc - 0x30;
        else if ((unsigned)wc - 0x41U <= (0x46 - 0x41))
            ufType |= (unsigned)wc - 0x41 + 0xa;
        else
        {
            unsigned uVal = UniQueryNumericValue(wc);
            if (!(uVal & ~0xffU))
                ufType |= uVal;
        }
    }
    ufType |= (pUniType->bidi & 0xf << 24);

    /** @todo screen width. */
    return ufType;
}

__END_DECLS

#endif /* __SYS_LOCALE_H__ */

