/ uldiv.s (emx+gcc) -- Copyright (c) 1992-1996 by Eberhard Mattes

#include <emx/asm386.h>

        .globl __uldiv

        .text
        ALIGN

/ _uldiv_t _uldiv (long num, long den)

__uldiv:
        PROFILE_NOFRAME
        movl    1*4(%esp), %eax         /* num */
        xorl    %edx, %edx              /* zero extension */
        divl    2*4(%esp)               /* den */
        EPILOGUE(_uldiv)
