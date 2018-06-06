/* abs.c (emx+gcc) -- Copyright (c) 1992-1993 by Eberhard Mattes */

#include "libc-alias.h"

int _STD(abs) (int n); /* inlined in header. */

int _STD(abs) (int n)
{
  return (n < 0 ? -n : n);
}
