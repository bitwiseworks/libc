/* atofl.c (emx+gcc) -- Copyright (c) 1993-1995 by Eberhard Mattes */

#include "libc-alias.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

/* this isn't a standard function declared anywhere */
long double atofl (__const__ char *);

long double _STD(atofl) (const char *s)
{
  return strtold (s, NULL);
}
