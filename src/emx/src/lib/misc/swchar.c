/* swchar.c (emx+gcc) -- Copyright (c) 1992-1996 by Eberhard Mattes */

#include "libc-alias.h"
#include <stdlib.h>
#include <emx/syscalls.h>

char _swchar (void)
{
  int x;

  x = __swchar (0, 0);
  if (x & 0xff)
    return 0;
  else
    return (char)((x>>8) & 0xff);
}
