/* Complex power of double values.
   Copyright (C) 1997 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Ulrich Drepper <drepper@cygnus.com>, 1997.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA.  */

#include "libc-alias-glibc.h"
#include <complex.h>
#include <math.h>

#define __nan(x) __builtin_nan(x)



__complex__ double
_STD(cpow) (__complex__ double x, __complex__ double c)
{
  return __cexp (c * __clog (x));
}
/* w e a k _ a l i a s  (__cpow, cpow) */
#ifdef NO_LONG_DOUBLE
strong_alias (__cpow, __cpowl)
/* w e a k _ a l i a s  (__cpow, cpowl) */
#endif
