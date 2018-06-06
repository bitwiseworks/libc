/* getw.c (emx+gcc) -- Copyright (c) 1992-1995 by Kai Uwe Rommel */

#include "libc-alias.h"
#include <stdio.h>

int _STD(getw) (FILE *stream)
{
  int x;

  return (fread (&x, sizeof (x), 1, stream) == 1 ? x : EOF);
}
