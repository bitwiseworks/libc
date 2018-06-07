/* feof.c (emx+gcc) -- Copyright (c) 1998 by Eberhard Mattes */

#include "libc-alias.h"
#include <stdio.h>

int _STD(feof)(FILE *s)
{
    return s->_flags & _IOEOF ? 1 : 0;
}

int _STD(feof_unlocked)(FILE *s)
{
    return s->_flags & _IOEOF ? 1 : 0;
}
