/* $Id: mb_libuni.c 2058 2005-06-23 05:58:05Z bird $ */
/** @file
 *
 * Generic MBSC <-> Unicode Converter using libuni.
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


/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#include "libc-alias.h"
#include <InnoTekLIBC/locale.h>
#define INCL_FSMACROS
#include <os2emx.h>
#include <uconv.h>
#include <errno.h>
#include <stdlib.h>
#include <limits.h>
#include <stdint.h>
#include <string.h>

typedef struct LIBUNIState
{
    char        chState;
} LIBUNISTATE, *PLIBUNISTATE;


static void libuni_errno(int rc)
{
    switch (rc)
    {
        case 0:
            break;
        case UCONV_E2BIG:           errno = E2BIG; break;
        case UCONV_EBADF:           errno = EBADF; break;
        case UCONV_EILSEQ:          errno = EILSEQ; break;
        case UCONV_EINVAL:          errno = EINVAL; break;
        case UCONV_EMFILE:          errno = EMFILE; break;
        case UCONV_ENFILE:          errno = ENFILE; break;
        case UCONV_ENOMEM:          errno = ENOMEM; break;
        case UCONV_BADATTR:
        case UCONV_EOTHER:
        case UCONV_NOTIMPLEMENTED:
        default:                    errno = EINVAL; break;
    }
}


static int     libuni_mbsinit(const __mbstate_t *ps)
{
    return ps == NULL
        || ((PLIBUNISTATE)ps)->chState == 0;
}


static size_t  libuni_mbrtowc(__wchar_t * __restrict pwc, const char * __restrict s, size_t n, __mbstate_t * __restrict ps)
{
    PLIBUNISTATE pState = (PLIBUNISTATE)ps;

    /*
     * Check input.
     */
    /* !s => mbrtowc(NULL, "", 1, ps) */
    if (s == NULL)
    {
        s = "";
        n = 1;
        pwc = NULL;
    }

    /* Incomplete multibyte sequence */
    if (n == 0)
        return ((size_t)-2);

    /*
     * Try the conversion table first.
     */
    __wchar_t wc = -1;
    unsigned char uch = *(const unsigned char *)s;
    if (    !pState->chState
        &&  (   (wc = __libc_GLocaleCtype.aucUnicode[uch]) != 0xffff
             || !IS_MBCS_PREFIX(&__libc_GLocaleCtype, uch)))

    {
        if (pwc != NULL)
            *pwc = wc;
        return wc != 0;
    }
    if (!__libc_GLocaleCtype.uobj) /* no possible conversion */
    {
        errno = EILSEQ;
        return ((size_t)-1);
    }

    /*
     * We must now get exclusive access to the conversion object,
     * and restore the state before we try convert anything.
     */
    wchar_t *pwcIn = &wc;
    if (n >= 0x7fffffff)
        n = 0x7ffffffe;                 /* UTF-8 decoder/encoder don't like -1, so let's play safe. */
    size_t  cbInLeft = n;
    size_t  cwcOutLeft = 1;
    size_t  cIgnore = 0;
    uconv_attribute_t attr;
    LOCALE_LOCK();
    if (!__libc_GLocaleCtype.uobj)
    {
        LOCALE_UNLOCK();
        errno = EILSEQ;
        return ((size_t)-1);
    }
    FS_VAR_SAVE_LOAD();
    int rc = UniQueryUconvObject(__libc_GLocaleCtype.uobj, &attr, sizeof(attr), NULL, NULL, NULL);
    if (!rc)
    {
        attr.state = pState->chState;
        attr.options = UCONV_OPTION_SUBSTITUTE_BOTH;
        attr.converttype &= ~CVTTYPE_PATH;
        attr.converttype = CVTTYPE_CDRA | CVTTYPE_CTRL7F;
        rc = UniSetUconvObject(__libc_GLocaleCtype.uobj, &attr);
        if (!rc)
        {
            rc = UniUconvToUcs(__libc_GLocaleCtype.uobj, (void **)(void *)&s, &cbInLeft, (UniChar **)&pwcIn, &cwcOutLeft, &cIgnore);
            if (    !rc
                ||  (rc == UCONV_E2BIG && !cwcOutLeft))
            {
                if (wc == 0xfffd)
                    wc = -1; /* this seems to make sense sometimes... */
                if (pwc)
                    *pwc = wc;
                rc = 0;
                if (!UniQueryUconvObject(__libc_GLocaleCtype.uobj, &attr, sizeof(attr), NULL, NULL, NULL))
                    pState->chState = attr.state;
            }
        }
    }
    LOCALE_UNLOCK();
    FS_RESTORE();

    if (!rc)
        return n - cbInLeft - !wc;
    /* failure */
    libuni_errno(rc);
    return -1;
}


static size_t libuni_wcrtomb(char * __restrict s, __wchar_t wc, __mbstate_t * __restrict ps)
{
    PLIBUNISTATE pState = (PLIBUNISTATE)ps;

    /*
     * Check state.
     */
    if (s == NULL)
    {
        pState->chState = 0; //??
        return 1; /* Reset to initial shift state (no-op) */
    }


    /*
     * Try use the conversion tables.
     */
    char    ch;
    if (!pState->chState)
    {
        if (wc < sizeof(__libc_GLocaleCtype.auchToSBCS0To128) / sizeof(__libc_GLocaleCtype.auchToSBCS0To128[0]))
        {
            ch = __libc_GLocaleCtype.auchToSBCS0To128[wc];
            if (ch || !wc)
            {
                if (s)
                    *s = ch;
                return 1;
            }
        }
        else
        {
            /** @todo make this a binary search! */
            int i = __libc_GLocaleCtype.cSBCSs;
            while (i-- > 0)
            {
                unsigned idx = (unsigned)wc - __libc_GLocaleCtype.aSBCSs[i].usStart;
                if (idx < __libc_GLocaleCtype.aSBCSs[i].cChars)
                {
                    ch = __libc_GLocaleCtype.aSBCSs[i].auch[idx];
                    if (ch)
                    {
                        if (s)
                            *s = ch;
                        return 1;
                    }
                    break;
                }
            }
        }
        /* not found */
    }
    if (!__libc_GLocaleCtype.uobj) /* no possible conversion */
    {
        errno = EILSEQ;
        return ((size_t)-1);
    }

    /*
     * We must now get exclusive access to the conversion object,
     * and restore the state before we try convert anything.
     */
    if (!s)
        s = alloca(MB_LEN_MAX);
    size_t      cbInLeft = 1;
    size_t      cIgnore = 0;
    UniChar    *pucIn = &wc;
    uconv_attribute_t attr;
    size_t      cchMbCurMax = MB_CUR_MAX;
    size_t      cchOutLeft = cchMbCurMax;
    LOCALE_LOCK();
    if (!__libc_GLocaleCtype.uobj)
    {
        LOCALE_UNLOCK();
        errno = EILSEQ;
        return ((size_t)-1);
    }
    FS_VAR_SAVE_LOAD();
    int rc = UniQueryUconvObject(__libc_GLocaleCtype.uobj, &attr, sizeof(attr), NULL, NULL, NULL);
    if (!rc)
    {
        attr.state = pState->chState;
        attr.options = UCONV_OPTION_SUBSTITUTE_BOTH;
        attr.converttype = CVTTYPE_CDRA | CVTTYPE_CTRL7F;
        rc = UniSetUconvObject(__libc_GLocaleCtype.uobj, &attr);
        if (!rc)
        {
            rc = UniUconvFromUcs(__libc_GLocaleCtype.uobj, &pucIn, &cbInLeft, (void **)(void *)&s, &cchOutLeft, &cIgnore);
            if (!rc)
            {
                if (!UniQueryUconvObject(__libc_GLocaleCtype.uobj, &attr, sizeof(attr), NULL, NULL, NULL))
                    pState->chState = attr.state;
            }
        }
    }
    LOCALE_UNLOCK();
    FS_RESTORE();

    if (!rc)
        return cchMbCurMax - cchOutLeft;
    /* failure */
    libuni_errno(rc);
    return -1;
}

static size_t  libuni_mbsnrtowcs(__wchar_t * __restrict dst, const char ** __restrict src, size_t nms, size_t len, __mbstate_t * __restrict ps)
{
    return __libc_localeFuncsGeneric_mbsnrtowcs(libuni_mbrtowc, dst, src, nms, len, ps);
}

static size_t  libuni_wcsnrtombs(char * __restrict dst, const __wchar_t ** __restrict src, size_t nwc, size_t len, __mbstate_t * __restrict ps)
{
    return __libc_localeFuncsGeneric_wcsnrtombs(libuni_wcrtomb, dst, src, nwc, len, ps);
}


/**
 * Fill in the default libuni base conversion functions.
 */
void __libc_localeFuncsDefault(__LIBC_PLOCALECTYPEFUNCS pFuncs)
{
    pFuncs->pfnmbsinit      = libuni_mbsinit;
    pFuncs->pfnmbrtowc      = libuni_mbrtowc;
    pFuncs->pfnmbsnrtowcs   = libuni_mbsnrtowcs;
    pFuncs->pfnwcrtomb      = libuni_wcrtomb;
    pFuncs->pfnwcsnrtombs   = libuni_wcsnrtombs;
}



size_t  __libc_localeFuncsGeneric_mbsnrtowcs(size_t  (*pfnmbrtowc)(__wchar_t * __restrict, const char * __restrict, size_t, __mbstate_t * __restrict),
                                             __wchar_t * __restrict dst, const char ** __restrict src, size_t nms, size_t len, __mbstate_t * __restrict ps)
{
    const char *pch = *src;
    size_t      cbIn = nms;
    if (dst == NULL)
    {
        __wchar_t wc;
        for (;;)
        {
            size_t cb = pfnmbrtowc(&wc, pch, nms, ps);
            if ((int)cb <= 0)
            {
                if (cb == (size_t)-1)
                    return (size_t)-1;    /* Invalid sequence - mbrtowc() sets errno. */
                else /* if (cb == 0 || cb == (size_t)-2) */
                    return cbIn - nms;
            }

            /* advance */
            pch += cb;
            nms -= cb;
        }
        /* (won't ever get here.) */
    }
    else
    {
        while (len-- > 0)
        {
            size_t cb = pfnmbrtowc(dst, pch, nms, ps);
            if ((int)cb <= 0)
            {
                if (!cb)
                {
                    *src = NULL;
                    return cbIn - nms;
                }
                else if (cb == (size_t)-2)
                {
                    *src = pch + nms;
                    return cbIn - nms;
                }
                else /*if (cb == (size_t)-1) */
                {
                    *src = pch;
                    return (size_t)-1;    /* Invalid sequence - mbrtowc() sets errno. */
                }
            }

            /* advance */
            pch += cb;
            nms -= cb;
            dst++;
        }

        *src = pch;
        return cbIn - nms;
    }
}


size_t  __libc_localeFuncsGeneric_wcsnrtombs(size_t  (*pfnwcrtomb)(char * __restrict, __wchar_t, __mbstate_t * __restrict),
                                             char * __restrict dst, const __wchar_t ** __restrict src, size_t nwc, size_t len, __mbstate_t * __restrict ps)
{
    char            achTmp[MB_LEN_MAX];
    const __wchar_t  *pwcs = *src;
    size_t          cbUsed = 0;
    if (dst == NULL)
    {
        while (nwc-- > 0)
        {
            __wchar_t wc = *pwcs;
            size_t cb = pfnwcrtomb(&achTmp[0], wc, ps);
            if (cb == (size_t)-1)
                return (size_t)-1; /* Invalid character - wcrtomb() sets errno. */
            if (wc == L'\0')
                return cbUsed + cb - 1;
            /* advance */
            pwcs++;
            cbUsed += cb;
        }
        return cbUsed;
    }
    else
    {
        while (len > 0 && nwc-- > 0)
        {
            size_t cb;
            __wchar_t wc = *pwcs;
            if (len > (size_t)MB_CUR_MAX)
            {
                cb = pfnwcrtomb(dst, wc, ps);
                if ((int)cb < 0)
                {
                    *src = pwcs;
                    return (size_t)-1;
                }
            }
            else
            {
                cb = pfnwcrtomb(&achTmp[0], wc, ps);
                if ((int)cb < 0)
                {
                    *src = pwcs;
                    return (size_t)-1;
                }
                if (cb > len)
                    break;  /* MB sequence for character won't fit. */
                switch (cb)
                {
                    case 4:
                        *(uint32_t *)dst = *(uint32_t *)&achTmp[0];
                        break;
                    case 3:
                        dst[2] = achTmp[2];
                        /* fall thru */
                    case 2:
                        *(uint16_t *)dst = *(uint16_t *)&achTmp[0];
                        break;
                    case 1:
                        dst[0] = achTmp[0];
                        break;
                    default:
                        memcpy(dst, achTmp, cb);
                        break;
                }
            }
            if (wc == L'\0')
            {
                *src = 0;
                return cbUsed + cb - 1;
            }

            /* advance */
            dst += cb;
            len -= cb;
            cbUsed += cb;
            pwcs++;
        }
        *src = pwcs;
        return cbUsed;

    }
}

/* later */
void __libc_localeFuncsSBCS(__LIBC_PLOCALECTYPEFUNCS pFuncs)
{
    __libc_localeFuncsDefault(pFuncs);
}

/* later */
void __libc_localeFuncsDBCS(__LIBC_PLOCALECTYPEFUNCS pFuncs)
{
    __libc_localeFuncsDefault(pFuncs);
}

/* later */
void __libc_localeFuncsMBCS(__LIBC_PLOCALECTYPEFUNCS pFuncs)
{
    __libc_localeFuncsDefault(pFuncs);
}

/* later */
void __libc_localeFuncsUCS2(__LIBC_PLOCALECTYPEFUNCS pFuncs)
{
    __libc_localeFuncsDefault(pFuncs);
}

