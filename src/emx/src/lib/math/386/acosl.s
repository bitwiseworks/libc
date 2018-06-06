/ acos.s (emx+gcc) -- Copyright (c) 1991-1996 by Eberhard Mattes

#define LONG_DOUBLE
#include <emx/asm386.h>

#define FUNC    _STD(acosl)

        .globl  FUNC

        .text

        ALIGN

/ double acos (double x)

/ acos(x) = atan2 (sqrt (1-x*x), x)

#define x       4(%esp)

FUNC:
        PROFILE_NOFRAME
        FLD     x                       /* x */
        fld     %st
        fmulp
        fsubrl  __const_ONE
        fsqrt
        FLD     x                       /* x */
        fpatan
        _xam
        j_nan   Lerror
Lreturn:EPILOGUE(FUNC)

        ALIGN
Lerror: SET_ERRNO_CONST($EDOM)
        jmp     Lreturn
