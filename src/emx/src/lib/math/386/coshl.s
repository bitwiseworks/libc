/ cosh.s (emx+gcc) -- Copyright (c) 1992-1996 by Eberhard Mattes

#define LONG_DOUBLE
#include <emx/asm386.h>

#define FUNC    _STD(coshl)

        .globl  FUNC

        .text

        ALIGN

/ double cosh (double x)

#define cw1      0(%esp)
#define cw2      2(%esp)
/define ret_addr 4(%esp)
#define x        8(%esp)

/ cosh(x) = 0.5 * (exp(x) + exp(-x)) = 0.5 * (exp(x) + 1/exp(-x))

FUNC:
        PROFILE_NOFRAME
        subl    $4, %esp                /* space for control words */
        FLD     x                       /* x */
        fldl2e                          /* log2 (e) */
        fmulp                           /* y := x * log2 (e) */
        fstcw   cw1
        movw    cw1, %ax
        andw    $0xf3ff, %ax
        orw     $0x0400, %ax            /* round down towards -inf */
        movw    %ax, cw2
        fldcw   cw2
        fld     %st                     /* y, y */
        frndint                         /* int (y), y */
        fldcw   cw1
        fxch    %st(1)                  /* y, int (y) */
        fsub    %st(1), %st             /* frac (y), int (y) */
        f2xm1                           /* 2^frac (y) - 1, int (y) */
        faddl   __const_ONE             /* 2^frac (y), int (y) */
        fscale                          /* 2^frac (y) * 2^int (y), int (y) */
        fst     %st(1)                  /* exp(x), exp(x) */
        fdivrl  __const_ONE             /* exp(-x), exp(x) */
        faddp                           /* exp(x) + exp(-x) */
        fmull   __const_HALF            /* 0.5 * (exp(x) + exp(-x)) */
        CONV(x)                         /* convert to double */
        addl    $4, %esp                /* Remove control words */
        _xam
        j_inf   Lerror
Lreturn:EPILOGUE(FUNC)

        ALIGN
Lerror: SET_ERRNO_CONST($ERANGE)
        jmp     Lreturn
