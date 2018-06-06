/* fgets.c (emx+gcc) -- Copyright (c) 1990-1996 by Eberhard Mattes */

#include "libc-alias.h"
#include <sys/builtin.h>        /* For <sys/fmutex.h> */
#include <sys/fmutex.h>
#include <stdio.h>
#include <stdlib.h>
#include <emx/io.h>
#include "getputc.h"

char *_STD(fgets)(char *buffer, int n, FILE *stream)
{
    STREAM_LOCK(stream);
    char *psz = fgets_unlocked(buffer, n, stream);
    STREAM_UNLOCK(stream);
    return psz;
}


char *_STD(fgets_unlocked)(char *buffer, int n, FILE *stream)
{
    int c;
    char *dst;

    if (n <= 0)
        return NULL;
    dst = buffer;

    while (n > 1)
    {
        c = _getc_inline(stream);
        if (c == EOF)
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
        *dst++ = (char)c;
        if (c == '\n')
            break;
        --n;
    }
    *dst = '\0';
    return buffer;
}

