/* atof.c (emx+gcc) -- Copyright (c) 1990-1995 by Eberhard Mattes */

#include "libc-alias.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

double _STD(atof) (const char *s)
{
  return strtod (s, NULL);
}
