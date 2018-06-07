/* core.c (emx+gcc) -- Copyright (c) 1990-1996 by Eberhard Mattes */

#include "libc-alias.h"
#include <stdlib.h>
#include <emx/syscalls.h>

int _core (int handle)
{
  (void)handle;
  /* Not implemented */
  return -1;
}
