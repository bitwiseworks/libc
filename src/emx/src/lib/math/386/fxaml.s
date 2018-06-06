/ fxam.s (emx+gcc) -- Copyright (c) 1991-1996 by Eberhard Mattes

#define LONG_DOUBLE
#include <emx/asm386.h>

#define FUNC    __fxaml

        .globl  FUNC

        .text

        ALIGN

/ int _fxam (double x)
/ int _fxaml (long double x)

#define x       4(%esp)

FUNC:
        PROFILE_NOFRAME
        FLD     x                       /* x */
        fxam
        fstsw   %ax
        fstp    %st(0)
        movl    %eax, %edx
        shrl    $8, %eax
        andl    $7, %eax
        test    $0x4000, %edx
        jz      Lreturn
        orl     $8, %eax
Lreturn:
        EPILOGUE(FUNC)
