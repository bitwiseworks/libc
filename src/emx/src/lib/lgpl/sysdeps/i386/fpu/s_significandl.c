/*
 * Written by J.T. Conklin <jtc@netbsd.org>.
 * Changes for long double by Ulrich Drepper <drepper@cygnus.com>
 * Public domain.
 */

#include "libc-alias-glibc.h"
#include <math.h>
#include <math_private.h>

long double
_STD(significandl) (long double x)
{
  long double res;

  asm ("fxtract\n"
       "fstp	%%st(1)" : "=t" (res) : "0" (x));
  return res;
}

/*  w e a k _alias (__significandl, significandl) */
