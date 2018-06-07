/* dup.c (emx+gcc) -- Copyright (c) 1992-1996 by Eberhard Mattes
                   -- Copyright (c) 2003 by Knut St. Osmunden */

#include "libc-alias.h"
#include <io.h>
#include <emx/syscalls.h>

int _STD(dup)(int fh)
{
    return __dup(fh);
}
