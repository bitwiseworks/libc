/ swapl.s (emx+gcc) -- Copyright (c) 1994 by Eberhard Mattes

        .globl  __swapl

        .text

        .align  2, 0x90

__swapl:
        movl    1*4(%esp), %eax         /* x */
        xchgb   %al, %ah
        roll    $16, %eax
        xchgb   %al, %ah
        ret
