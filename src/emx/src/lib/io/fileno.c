/* fileno.c (emx+gcc) -- Copyright (c) 1998 by Eberhard Mattes */

#include "libc-alias.h"
#include <stdio.h>

int _STD(fileno)(FILE *s)
{
    return s->_handle;
}

int _STD(fileno_unlocked)(FILE *s)
{
    return s->_handle;
}

