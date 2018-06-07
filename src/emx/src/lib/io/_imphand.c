/* _imphand.c (emx+gcc) -- Copyright (c) 1994-1996 by Eberhard Mattes */

#include "libc-alias.h"
#include <io.h>
#include <emx/syscalls.h>

int _imphandle (int handle)
{
    return __imphandle(handle);
}
