/* float.h,v 1.5 2004/09/14 22:27:33 bird Exp */
/** @file
 * EMX + GCC + Some FreeBSD Bits.
 *
 * @remark Must be used instead of the GCC one.
 */

#ifndef _FLOAT_H
#define _FLOAT_H

#include <sys/cdefs.h>

__BEGIN_DECLS
extern int __flt_rounds(void);
__END_DECLS

#define FLT_RADIX	2		/* b */
#define FLT_ROUNDS	__flt_rounds()

#define	FLT_EVAL_METHOD	(-1)		/* i387 semantics are...interesting */
#define	DECIMAL_DIG	21		/* max precision in decimal digits */

#define FLT_MANT_DIG    24
#define FLT_MIN_EXP     (-125)
#define FLT_MAX_EXP     128
#define FLT_DIG         6
#define FLT_MIN_10_EXP  (-37)
#define FLT_MAX_10_EXP  38
#define FLT_MIN         1.17549435e-38F
#define FLT_MAX         3.40282347e+38F
#define FLT_EPSILON     1.19209290e-07F

#define DBL_MANT_DIG    53
#define DBL_MIN_EXP     (-1021)
#define DBL_MAX_EXP     1024
#define DBL_DIG         15
#define DBL_MIN_10_EXP  (-307)
#define DBL_MAX_10_EXP  308
#define DBL_MIN         2.2250738585072014e-308
#define DBL_MAX         1.7976931348623157e+308
#define DBL_EPSILON     2.2204460492503131e-016

/* bird: There is some divergence in the values for this section.
 *       We'll go for the GCC values. */
#define LDBL_MANT_DIG   64
#define LDBL_MIN_EXP    (-16381)
#define LDBL_MAX_EXP    16384
#define LDBL_DIG        18
#define LDBL_MIN_10_EXP (-4931)
#define LDBL_MAX_10_EXP 4932
#if 0
/* emx: */
#define LDBL_MIN        3.3621031431120935063e-4932L
#define LDBL_MAX        1.1897314953572317650e+4932L
#define LDBL_EPSILON    1.08420217248550443400745280086994171142578125e-0019L
#else
/* gcc: i386 config */
#define LDBL_MIN        3.36210314311209350626e-4932L
#define LDBL_MAX        1.18973149535723176502e+4932L
#define LDBL_EPSILON    1.08420217248550443401e-19L
#endif


/* bird: FreeBSD stuff at top, this is from gcc, wonder who's right...
#if defined (__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
#define FLT_EVAL_METHOD	0
#define DECIMAL_DIG	17
#endif */ /* C99 */


#if !defined (__STRICT_ANSI__) && !defined (_POSIX_SOURCE) || defined(__USE_EMX)

#if !defined (_OMIT_387_STUFF)

#define MCW_EM                  0x003f
#define EM_INVALID              0x0001
#define EM_DENORMAL             0x0002
#define EM_ZERODIVIDE           0x0004
#define EM_OVERFLOW             0x0008
#define EM_UNDERFLOW            0x0010
#define EM_INEXACT              0x0020

#define MCW_IC                  0x1000
#define IC_PROJECTIVE           0x0000
#define IC_AFFINE               0x1000

#define MCW_RC                  0x0c00
#define RC_NEAR                 0x0000
#define RC_DOWN                 0x0400
#define RC_UP                   0x0800
#define RC_CHOP                 0x0c00

#define MCW_PC                  0x0300
#define PC_24                   0x0000
#define PC_53                   0x0200
#define PC_64                   0x0300

#define CW_DEFAULT              (RC_NEAR | PC_64 | MCW_EM)

#define SW_INVALID              0x0001
#define SW_DENORMAL             0x0002
#define SW_ZERODIVIDE           0x0004
#define SW_OVERFLOW             0x0008
#define SW_UNDERFLOW            0x0010
#define SW_INEXACT              0x0020
#define SW_STACKFAULT           0x0040
#define SW_STACKOVERFLOW        0x0200

__BEGIN_DECLS
unsigned _clear87 (void);
unsigned _control87 (unsigned, unsigned);
void _fpreset (void);
unsigned _status87 (void);
__END_DECLS

#endif /* !defined (_OMIT_387_STUFF) */
#endif

#endif /* not _FLOAT_H */
