/* fgetchar.c (emx+gcc) -- Copyright (c) 1990-1995 by Eberhard Mattes */

#include "libc-alias.h"
#include <stdio.h>

int _STD(fgetchar) (void)
{
  return getchar ();
}
