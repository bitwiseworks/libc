/* clearerr.c (emx+gcc) -- Copyright (c) 1990-1995 by Eberhard Mattes */

#include "libc-alias.h"
#include <stdio.h>

void _STD(clearerr)(FILE *stream)
{
    stream->_flags &= ~(_IOERR|_IOEOF);
}

void _STD(clearerr_unlocked)(FILE *stream)
{
    stream->_flags &= ~(_IOERR|_IOEOF);
}
