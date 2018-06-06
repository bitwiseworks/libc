/* fputchar.c (emx+gcc) -- Copyright (c) 1990-1995 by Eberhard Mattes */

#include "libc-alias.h"
#include <stdio.h>

int _STD(fputchar) (int c)
{
  return putchar (c);
}
