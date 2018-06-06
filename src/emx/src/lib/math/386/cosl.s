/ cos.s (emx+gcc) -- Copyright (c) 1992-1993 by Steffen Haecker
/                    Modified 1993-1996 by Eberhard Mattes

#define LONG_DOUBLE
#include <emx/asm386.h>

#define FUNC    _STD(cosl)

        .globl  FUNC

        .text

        ALIGN

/ double cos (double x)

#define x       4(%esp)

FUNC:
        PROFILE_NOFRAME
        FLD     x                       /* x */
        fcos                            /* cos(x) */
        fstsw   %ax
        testb   $0x04, %ah
        jnz     Llarge                  /* C2 != 0 ? */
Lreturn:EPILOGUE(FUNC)

        ALIGN
Llarge: fldl    __const_ONE             /* cos(large):=1 */
        fstp    %st(1)
        jmp     Lreturn
