/* fputs.c (emx+gcc) -- Copyright (c) 1990-1993 by Eberhard Mattes */

#include "libc-alias.h"
#include <stdio.h>
#include <string.h>

int _STD(fputs)(const char *string, FILE *stream)
{
    int len = strlen (string);
    return len == 0 || fwrite(string, len, 1, stream) == 1
        ? 0 : EOF;
}

int _STD(fputs_unlocked)(const char *string, FILE *stream)
{
    int len = strlen(string);
    return len == 0 || fwrite_unlocked(string, len, 1, stream) == 1
        ? 0 : EOF;
}

