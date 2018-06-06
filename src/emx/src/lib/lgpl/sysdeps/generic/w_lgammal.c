/* w_lgammal.c -- long double version of w_lgamma.c.
 * Conversion to long double by Ulrich Drepper,
 * Cygnus Support, drepper@cygnus.com.
 */

/*
 * ====================================================
 * Copyright (C) 1993 by Sun Microsystems, Inc. All rights reserved.
 *
 * Developed at SunPro, a Sun Microsystems, Inc. business.
 * Permission to use, copy, modify, and distribute this
 * software is freely granted, provided that this notice
 * is preserved.
 * ====================================================
 */

#include "libc-alias-glibc.h"
#if defined(LIBM_SCCS) && !defined(lint)
static char rcsid[] = "$NetBSD: $";
#endif

/* long double lgammal(long double x)
 * Return the logarithm of the Gamma function of x.
 *
 * Method: call __ieee754_lgammal_r
 */

#include "math.h"
#include "math_private.h"

#ifdef __STDC__
	long double __lgammal(long double x)
#else
	long double __lgammal(x)
	long double x;
#endif
{
#if defined _IEEE_LIBM || defined __INNOTEK_LIBC__
	return __ieee754_lgammal_r(x,&signgam);
#else
        long double y;
	int local_signgam = 0;
        y = __ieee754_lgammal_r(x,&local_signgam);
	if (_LIB_VERSION != _ISOC_)
	  /* ISO C99 does not define the global variable.  */
	  signgam = local_signgam;
        if(_LIB_VERSION == _IEEE_) return y;
        if(!__finitel(y)&&__finitel(x)) {
            if(__floorl(x)==x&&x<=0.0)
                return __kernel_standard(x,x,215); /* lgamma pole */
            else
                return __kernel_standard(x,x,214); /* lgamma overflow */
        } else
            return y;
#endif
}
/* w e a k _alias (__lgammal, lgammal) */
strong_alias (_std_lgammal, _std_gammal)
/* w e a k _alias (__gammal, gammal) */
