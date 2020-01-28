/*
    fputwc.c -- copied from fputc.c
    Copyright (c) 2020 bww bitwiseworks GmbH
*/

#define _GNU_SOURCE /* for _unlocked prototypes */
#define _WIDECHAR /* for getputc.h */

#include "libc-alias.h"
#include <sys/fmutex.h>
#include <stdio.h>
#include <stdlib.h>
#include <emx/io.h>
#include <wchar.h>
#include <limits.h>
#include "getputc.h"

wint_t _STD(putwc)(wchar_t c, FILE *stream)
{
    return fputwc(c, stream);
}

wint_t _STD(fputwc)(wchar_t c, FILE *stream)
{
    wint_t r;

    STREAM_LOCK(stream);
    r = _putwc_inline(c, stream);
    STREAM_UNLOCK(stream);
    return r;
}

wint_t _STD(putwc_unlocked)(wchar_t c, FILE *stream)
{
    return fputwc_unlocked(c, stream);
}

wint_t _STD(fputwc_unlocked)(wchar_t c, FILE *stream)
{
    wint_t r;

    STREAM_LOCK(stream);
    r = _putwc_inline(c, stream);
    STREAM_UNLOCK(stream);
    return r;
}

/* Internal function that converts wchar_t to multibyte
   before putting it to the stream. */
wint_t __libc_putwc_convert (wchar_t _c, FILE *_s)
{
    /* Zero character doesn't requre conversion. */
    if (!_c)
        return _putc_inline (_c, _s);

    char mb[MB_LEN_MAX];
    mbstate_t state = {{0}};
    size_t cb = wcrtomb (mb, _c, &state);
    if (cb == (size_t) -1)
        return WEOF;

    size_t i = 0;
    while (i < cb)
        if (_putc_inline (mb [i++], _s) == EOF)
            return WEOF;

    return _c;
}
