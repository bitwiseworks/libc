/ clear.s (emx+gcc) -- Copyright (c) 1993-1996 by Eberhard Mattes

#include <emx/asm386.h>

        .globl  __clear87

        .text

        ALIGN

/ unsigned _clear87 (void)

__clear87:
        PROFILE_NOFRAME
        xor     %eax, %eax              /* Clear upper 16 bits */
        fstsw   %ax
        fclex
        EPILOGUE(__clear87)
