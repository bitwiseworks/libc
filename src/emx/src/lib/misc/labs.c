/* labs.c (emx+gcc) -- Copyright (c) 1992-1993 by Eberhard Mattes */

#include "libc-alias.h"

long _STD(labs) (long); /* inlined in header */

long _STD(labs) (long n)
{
  return (n < 0 ? -n : n);
}
