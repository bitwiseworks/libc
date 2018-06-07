/* fgetc.c (emx+gcc) -- Copyright (c) 1990-1996 by Eberhard Mattes */

#include "libc-alias.h"
#include <sys/builtin.h>        /* For <sys/fmutex.h> */
#include <sys/fmutex.h>
#include <stdio.h>
#include <stdlib.h>
#include <emx/io.h>
#include "getputc.h"

int _STD(getc)(FILE *stream)
{
    return fgetc(stream);
}

int _STD(fgetc)(FILE *stream)
{
    STREAM_LOCK(stream);
    int rc = _getc_inline(stream);
    STREAM_UNLOCK(stream);
    return rc;
}

int _STD(getc_unlocked)(FILE *stream)
{
    return fgetc_unlocked(stream);
}

int _STD(fgetc_unlocked)(FILE *stream)
{
    return _getc_inline(stream);
}
