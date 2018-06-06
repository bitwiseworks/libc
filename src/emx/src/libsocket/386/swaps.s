/ swaps.s (emx+gcc) -- Copyright (c) 1994 by Eberhard Mattes

        .globl  __swaps

        .text

        .align  2, 0x90

__swaps:
        movl    1*4(%esp), %eax         /* x */
        xchgb   %al, %ah
        ret
