/ modf.s (emx+gcc) -- Copyright (c) 1991-1996 by Eberhard Mattes

#define LONG_DOUBLE
#include <emx/asm386.h>

#define FUNC    _STD(modfl)

        .globl  FUNC

        .text

        ALIGN

/ double modf (double x, double *intptr)

#define cw1       0(%esp)
#define cw2       2(%esp)
/define ret_addr  4(%esp)
#define x         8(%esp)
#if defined (LONG_DOUBLE)
#define intptr   20(%esp)
#else
#define intptr   16(%esp)
#endif

FUNC:
        PROFILE_NOFRAME
        subl    $4, %esp
        fstcw   cw1
        movw    cw1, %ax
        orw     $0x0c00, %ax            /* chop mode */
        movw    %ax, cw2
        movl    intptr, %eax
        fldcw   cw2
        FLD     x                       /* x */
        fld     %st
        frndint
#if defined (LONG_DOUBLE)
        fld     %st                     /* there is no fstt, use fstpt */
        fstpt   (%eax)
#else
        fstl    (%eax)
#endif
        fldcw   cw1
        fsubrp
        addl    $4, %esp
        EPILOGUE(FUNC)
