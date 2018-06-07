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
_STD(fmodl) (long double x, long double y)
{
  long double res;

  asm ("1:\tfprem\n"
       "fstsw   %%ax\n"
       "sahf\n"
       "jp      1b\n"
       "fstp    %%st(1)"
       : "=t" (res) : "0" (x), "u" (y) : "ax", "st(1)");
  return res;
}
