/* puts.c (emx+gcc) -- Copyright (c) 1990-1996 by Eberhard Mattes */

#include "libc-alias.h"
#include <sys/builtin.h>        /* For <sys/fmutex.h> */
#include <sys/fmutex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <emx/io.h>
#include "getputc.h"

int _STD(puts) (const char *string)
{
    STREAM_LOCK(stdout);
    int rc = puts_unlocked(string);
    STREAM_UNLOCK(stdout);
    return rc;
}

int _STD(puts_unlocked) (const char *string)
{
    int result;

    int len = strlen(string);
    if (nbuf(stdout))
        _fbuf (stdout);
    void *tb;
    _tmpbuf(stdout, tb);
    if (    len == 0
        ||  fwrite_unlocked(string, len, 1, stdout) == 1)
        result = _putc_inline('\n', stdout);
    else
        result = EOF;
    if (_endbuf(stdout) != 0)
        result = EOF;
    return result;
}
