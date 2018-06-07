/ hypot.s (emx+gcc) -- Copyright (c) 1992-1996 by Eberhard Mattes

/ As the arguments are double and the computation is done in extended
/ format, an iterative method or scaling is not required to avoid overflow.
/ However, such an algorithm should perhaps be used for _hypotl().

#define LONG_DOUBLE
#include <emx/asm386.h>

#define FUNC    _STD(hypotl)

        .globl  FUNC

        .text

        ALIGN

/ double _hypot (double x, double y)

#define x        4(%esp)
#if defined (LONG_DOUBLE)
#define y       16(%esp)
#else
#define y       12(%esp)
#endif

FUNC:
        PROFILE_NOFRAME
        FLD     x                       /* x */
        fld     %st                     /* x, x */
        fmulp                           /* x*x */
        FLD     y                       /* y, x*x */
        fld     %st                     /* y, y, x*x */
        fmulp                           /* y*y, x*x */
        faddp                           /* y*y + x*x */
        fsqrt                           /* hypot (x, y) */
        CONV(x)                         /* convert to double */
        _xam
        j_inf   Lerror
Lreturn:
        EPILOGUE(FUNC)

        ALIGN
Lerror: SET_ERRNO_CONST($ERANGE)
        jmp     Lreturn
