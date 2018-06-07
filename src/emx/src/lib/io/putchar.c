/* putchar.c (emx+gcc) -- Copyright (c) 1998 by Eberhard Mattes */

#include "libc-alias.h"
#include <stdio.h>

int _STD(putchar)(int c)
{
    return putc(c, stdout);
}

int _STD(putchar_unlocked)(int c)
{
    return putc_unlocked(c, stdout);
}
