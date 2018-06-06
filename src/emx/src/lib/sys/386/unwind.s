/ sys/unwind.s (emx+gcc) -- Copyright (c) 1992-1994 by Eberhard Mattes */

        .globl  ___unwind2

/ void __unwind2 (void *xcpt_reg_ptr)

___unwind2:
        movl    1*4(%esp), %eax         /* xcpt_reg_ptr */
        pushl   $0
        pushl   $L_cont
        pushl   %eax
        call    DosUnwindException
L_cont: addl    $3*4, %esp
        ret
