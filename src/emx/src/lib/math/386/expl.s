/ exp.s (emx+gcc) -- Copyright (c) 1991-2000 by Eberhard Mattes

#define LONG_DOUBLE
#include <emx/asm386.h>

#define FUNC    _STD(expl)

        .globl  FUNC

        .text

        ALIGN

/ double exp (double x)

#define cw1      0(%esp)
#define cw2      2(%esp)
/define ret_addr 4(%esp)
#define x        8(%esp)

FUNC:
        PROFILE_NOFRAME
        subl    $4, %esp                /* space for control words */
        FLD     x                       /* x */
        fxam
        fstsw  %ax
        andb    $0x47, %ah
        cmpb    $5, %ah
        je      Lreturn                 /* x = +INF => return x */
        cmpb    $7, %ah
        je      Lminf                   /* x = -INF => return 0 */
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
        fstp    %st(1)                  /* 2^frac (y) * 2^int (y) */
        CONV(x)                         /* convert to double */
        _xam
        j_inf   Lerror
Lreturn:addl    $4, %esp                /* Remove control words */
        EPILOGUE(FUNC)

        ALIGN
Lerror: SET_ERRNO_CONST($ERANGE)
        jmp     Lreturn

        ALIGN
Lminf:  fstp    %st(0)                  /* Pop x */
        fldl    __const_ZERO            /* Return +0 */
        jmp     Lreturn
