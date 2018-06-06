/* getcwd2.c (emx+gcc) -- Copyright (c) 1992-1996 by Eberhard Mattes */

#include "libc-alias.h"
#include <unistd.h>
#include <stdlib.h>

/**
 * Obsolete.
 */
char *_getcwd2 (char *buffer, int size)
{
    return getcwd(buffer, size);
}
