/* fputc.c (emx+gcc) -- Copyright (c) 1990-1996 by Eberhard Mattes */

#include "libc-alias.h"
#include <sys/fmutex.h>
#include <stdio.h>
#include <stdlib.h>
#include <emx/io.h>
#include "getputc.h"

int _STD(putc)(int c, FILE *stream)
{
    return fputc(c, stream);
}

int _STD(fputc)(int c, FILE *stream)
{
    int r;

    STREAM_LOCK(stream);
    r = _putc_inline(c, stream);
    STREAM_UNLOCK(stream);
    return r;
}

int _STD(putc_unlocked)(int c, FILE *stream)
{
    return fputc_unlocked(c, stream);
}

int _STD(fputc_unlocked)(int c, FILE *stream)
{
    int r;

    STREAM_LOCK(stream);
    r = _putc_inline(c, stream);
    STREAM_UNLOCK(stream);
    return r;
}
