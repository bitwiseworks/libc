/* gets.c (emx+gcc) -- Copyright (c) 1990-1998 by Eberhard Mattes */

#include "libc-alias.h"
#include <sys/builtin.h>        /* For <sys/fmutex.h> */
#include <sys/fmutex.h>
#include <stdio.h>
#include <stdlib.h>
#include <emx/io.h>
#include "getputc.h"

char *_STD(gets_unlocked)(char *buffer)
{
    int c;
    char *dst;

    dst = buffer;
    for (;;)
    {
        c = _getc_inline(stdin);
        if (c == EOF)
        {
            if (dst == buffer)
            {
                *dst = '\0';
                return NULL;
            }
            break;
        }
        if (c == '\n')
            break;
        *dst++ = (char)c;
    }
    *dst = 0;
    return buffer;
}

char *_STD(gets)(char *buffer)
{
    STREAM_LOCK(stdin);
    char *psz = gets_unlocked(buffer);
    STREAM_UNLOCK(stdin);
    return psz;
}



