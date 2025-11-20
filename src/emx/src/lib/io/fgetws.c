/*
    fgetws.c -- copied from fgets.c
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
#include "getputc.h"

wchar_t *_STD(fgetws)(wchar_t *buffer, int n, FILE *stream)
{
    STREAM_LOCK(stream);
    wchar_t *psz = fgetws_unlocked(buffer, n, stream);
    STREAM_UNLOCK(stream);
    return psz;
}

wchar_t *_STD(fgetws_unlocked)(wchar_t *buffer, int n, FILE *stream)
{
    /* This should never be called on pseudo-streams which are swscanf buffers
       so we always call a converting getwc variant. */

    wint_t c;
    wchar_t *dst;

    if (n <= 0)
        return NULL;
    dst = buffer;

    while (n > 1)
    {
        c = __libc_getwc_convert(stream);
        if (c == WEOF)
        {
            /* ISO/IEC 9899:1990, 7.9.7.2: "If end-of-file is
               encountered and no characters have been read into the
               array, the contents of the array remain unchanged and a
               null pointer is returned.  If a read error occurs during
               the operation, the array contents are indeterminate and a
               null pointer is returned." */

            if (dst == buffer || ferror(stream))
                return NULL;
            break;              /* EOF after reading at least one char */
        }
        *dst++ = c;
        if (c == '\n')
            break;
        --n;
    }
    *dst = '\0';
    return buffer;
}
