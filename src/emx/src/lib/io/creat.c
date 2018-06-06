/* creat.c (emx+gcc) -- Copyright (c) 1990-1995 by Eberhard Mattes */

#include "libc-alias.h"
#include <io.h>
#include <fcntl.h>

int _STD(creat) (const char *name, mode_t pmode)
{
  return open (name, O_WRONLY|O_TRUNC|O_CREAT, pmode);
}
