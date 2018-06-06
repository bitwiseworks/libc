/* ferror.c (emx+gcc) -- Copyright (c) 1998 by Eberhard Mattes */

#include "libc-alias.h"
#include <stdio.h>

int _STD(ferror)(FILE *s)
{
    return s->_flags & _IOERR ? 1 : 0;
}

int _STD(ferror_unlocked)(FILE *s)
{
    return s->_flags & _IOERR ? 1 : 0;
}
