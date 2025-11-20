/*
    fgetwc.c -- copied from fgetc.c
    Copyright (c) 2020 bww bitwise works GmbH
*/

#define _GNU_SOURCE /* for _unlocked prototypes */
#define _WIDECHAR /* for getputc.h */

#include "libc-alias.h"
#include <sys/builtin.h>        /* For <sys/fmutex.h> */
#include <sys/fmutex.h>
#include <stdio.h>
#include <stdlib.h>
#include <emx/io.h>
#include <wchar.h>
#include <limits.h>
#include "getputc.h"

wint_t _STD(getwc)(FILE *stream)
{
    return fgetwc(stream);
}

wint_t _STD(fgetwc)(FILE *stream)
{
    STREAM_LOCK(stream);
    wint_t rc = _getwc_inline(stream);
    STREAM_UNLOCK(stream);
    return rc;
}

wint_t _STD(getwc_unlocked)(FILE *stream)
{
    return fgetwc_unlocked(stream);
}

wint_t _STD(fgetwc_unlocked)(FILE *stream)
{
    return _getwc_inline(stream);
}

/* Internal function that converts multibyte to wchar_t
   after getting it from the stream. */
wint_t __libc_getwc_convert (FILE *_s)
{
    int c = _getc_inline (_s);
    if (c == EOF)
        return WEOF;

    /* Zero character doesn't requre conversion. */
    if (!c)
        return c;

    char mb[MB_LEN_MAX];
    mbstate_t state = {{0}};
    size_t len = 1;
    mb[0] = c;
    while (1)
    {
        wchar_t wc;
        size_t cb = mbrtowc (&wc, mb, len, &state);
        if (cb > 0)
            return wc;
        if (len == MB_LEN_MAX || cb != (size_t) -2)
           break;

        /* -2 is incomplete sequence, read further */
        c = _getc_inline (_s);
        if (c == EOF)
            break;
        mb[len++] = c;
    }

    return WEOF;
}
