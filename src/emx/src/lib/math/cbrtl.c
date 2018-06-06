/* cbrtl.c (emx+gcc) -- Copyright (c) 1992-1995 by Eberhard Mattes */

#include "libc-alias.h"
#include <math.h>

long double _STD(cbrtl) (long double x)
{
  if (x >= 0)
    return powl (x, 1.0 / 3.0);
  else
    return -powl (-x, 1.0 / 3.0);
}
