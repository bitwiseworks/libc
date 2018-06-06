/* getchar.c (emx+gcc) -- Copyright (c) 1998 by Eberhard Mattes */

#include "libc-alias.h"
#include <stdio.h>

int _STD(getchar)(void)
{
    return getc(stdin);
}

int _STD(getchar_unlocked)(void)
{
    return getc_unlocked(stdin);
}
