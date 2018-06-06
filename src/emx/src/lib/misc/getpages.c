/* getpages.c (emx+gcc) -- Copyright (c) 1990-1995 by Eberhard Mattes */

#include "libc-alias.h"
#include <stdlib.h>

int _STD(getpagesize) (void)
{
  return 4096;
}
