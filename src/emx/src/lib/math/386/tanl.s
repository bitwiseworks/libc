/ tan.s (emx+gcc) -- Copyright (c) 1992-1993 by Steffen Haecker
/                    Modified 1993-1996 by Eberhard Mattes

#define LONG_DOUBLE
#include <emx/asm386.h>

#define FUNC    _STD(tanl)

        .globl  FUNC

        .text

        ALIGN

/ double tan (double x)

#define x       4(%esp)

FUNC:
        PROFILE_NOFRAME
        FLD     x                       /* x */
        fptan

/* Note: fptan is followed by fstsw , avoiding a bug in the 486. */

        fstsw   %ax
        testb   $0x04, %ah
        jnz     Llarge                  /* C2 != 0 ? */

/* This used to be fdivrp.  However, on the 387, as opposed to the 8087,
   st(0) is always 1 after fptan. */

        fstp    %st(0)
Lreturn:EPILOGUE(FUNC)

        ALIGN
Llarge: fldl    __const_ZERO            /* tan(large):=0 */
        fstp    %st(1)
        fstp    %st(1)
        jmp     Lreturn
