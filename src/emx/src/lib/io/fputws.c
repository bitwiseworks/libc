/*
    fputws.c -- copied (in part) from fputs.c
    Copyright (c) 2020 bww bitwiseworks GmbH
*/

#define _GNU_SOURCE /* for _unlocked prototypes */
#define _WIDECHAR /* for getputc.h */

#include "libc-alias.h"
#include <sys/builtin.h>        /* For <sys/fmutex.h> */
#include <sys/fmutex.h>
#include <stdio.h>
#include <string.h>
#include <emx/io.h>
#include <wchar.h>
#include "getputc.h"

int _STD(fputws)(const wchar_t *string, FILE *stream)
{
    STREAM_LOCK(stream);
    int rc = fputws_unlocked(string, stream);
    STREAM_UNLOCK(stream);
    return rc;
}

int _STD(fputws_unlocked)(const wchar_t *string, FILE *stream)
{
    /* This should never be called on pseudo-streams which are swprintf buffers
       so we always call a converting putwc variant. */

    const wchar_t *s = string;
    while (*s)
        if (__libc_putwc_convert(*s++, stream) == WEOF)
            return WEOF;
    return 0;
}
