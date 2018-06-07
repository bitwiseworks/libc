/ div.s (emx+gcc) -- Copyright (c) 1990-1996 by Eberhard Mattes

#include <emx/asm386.h>

        .globl _STD(div), _STD(ldiv)

        .text
        ALIGN

/ div_t div (int num, int den)
/ ldiv_t ldiv (long num, long den)

_STD(ldiv):
_STD(div):
        PROFILE_NOFRAME
        movl    1*4(%esp), %eax         /* num */
        cltd                            /* sign extension */
        idivl   2*4(%esp)               /* den */
        EPILOGUE(_STD(div))
