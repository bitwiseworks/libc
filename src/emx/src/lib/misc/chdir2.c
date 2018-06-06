/* chdir2.c (emx+gcc) -- Copyright (c) 1992-1996 by Eberhard Mattes */

#include "libc-alias.h"
#include <stdlib.h>

int _chdir2 (const char *name)
{
    return chdir(name);
}
