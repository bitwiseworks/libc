/ status.s (emx+gcc) -- Copyright (c) 1993-1996 by Eberhard Mattes

#include <emx/asm386.h>

        .globl  __status87

        .text

        ALIGN

/ unsigned _status87 (void)

__status87:
        PROFILE_NOFRAME
        xor     %eax, %eax              /* Clear upper 16 bits */
        fstsw   %ax
        EPILOGUE(__status87)
