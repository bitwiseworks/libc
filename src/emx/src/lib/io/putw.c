/* putw.c (emx+gcc) -- Copyright (c) 1992-1995 by Kai Uwe Rommel */

#include "libc-alias.h"
#include <stdio.h>

int _STD(putw) (int x, FILE *stream)
{
  return (fwrite (&x, sizeof (x), 1, stream) == 1 ? x : EOF);
}
