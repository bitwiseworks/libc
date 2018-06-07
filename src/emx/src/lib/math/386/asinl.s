/ asin.s (emx+gcc) -- Copyright (c) 1991-1996 by Eberhard Mattes

#define LONG_DOUBLE
#include <emx/asm386.h>

#define FUNC    _STD(asinl)

        .globl  FUNC

        .text

        ALIGN

/ double asin (double x)

/ asin(x) = atan (x / sqrt (1-x*x))

#define x       4(%esp)

FUNC:
        PROFILE_NOFRAME
        FLD     x                       /* x */
        fld     %st
        fld     %st
        fmulp
        fsubrl  __const_ONE
        fsqrt
        fdivrp
        fldl    __const_ONE
        fpatan
        _xam
        j_nan   Lerror
Lreturn:EPILOGUE(FUNC)

        ALIGN
Lerror: SET_ERRNO_CONST($EDOM)
        jmp     Lreturn
