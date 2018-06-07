/*
 * Written by J.T. Conklin <jtc@netbsd.org>.
 * Public domain.
 *
 * Adapted for `long double' by Ulrich Drepper <drepper@cygnus.com>.
 */

#include "libc-alias-glibc.h"
#include <math.h>
#include <math_private.h>

long double
_STD(sqrtl) (long double x)
{
  long double res;

  asm ("fsqrt" : "=t" (res) : "0" (x));

  return res;
}
