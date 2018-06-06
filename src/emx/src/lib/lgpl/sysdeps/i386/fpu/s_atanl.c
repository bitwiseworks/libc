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
_STD(atanl) (long double x)
{
  long double res;

  asm ("fld1\n"
       "fpatan"
       : "=t" (res) : "0" (x));

  return res;
}

/*  w e a k _ a l i a s (__atanl, atanl) */
