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
_STD(atan2l) (long double y, long double x)
{
  long double res;

  asm ("fpatan" : "=t" (res) : "u" (y), "0" (x) : "st(1)");

  return res;
}
