/* sys/swchar.c (emx+gcc) -- Copyright (c) 1992-1996 by Eberhard Mattes */

#include "libc-alias.h"
#include <emx/syscalls.h>

int __swchar (int flag, int new)
{
  return 0xff;
}
