/* cuserid.c (emx+gcc) -- Copyright (c) 1994-1995 by Eberhard Mattes */

#include "libc-alias.h"
#include <stdio.h>
#include <unistd.h>
#include <unistd.h>

char *_STD(cuserid)(char *string)
{
    static char s_sz[L_cuserid];
    if (!string)
        string = s_sz;
    /** @todo this is wrong, it may be a different user! s*/
    if (!getlogin_r(string, L_cuserid))
        return string;
    return NULL;
}

