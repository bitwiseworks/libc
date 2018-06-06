/ int86.s (emx+gcc) -- Copyright (c) 1992-1996 by Eberhard Mattes

#include <emx/asm386.h>

        .globl __int86

        .text
        ALIGN

/ int _int86 (int int_num, union REGS *inp_regs, union REGS *out_regs)

__int86:
        PROFILE_NOFRAME
        pushl   %ebp
        pushl   %ebx
        pushl   %esi
        pushl   %edi
        movl    $0xc3c300cd, %eax       /* int 0; ret; ret */
        movb    5*4(%esp), %ah          /* int_num */
        pushl   %eax                    /* Push code */
        pushl   %esp                    /* Push address of code */
        movl    8*4(%esp), %ebx         /* inp_regs */
        movl    0*4(%ebx), %eax
        movl    2*4(%ebx), %ecx
        movl    3*4(%ebx), %edx
        movl    4*4(%ebx), %esi
        movl    5*4(%ebx), %edi
        movl    1*4(%ebx), %ebx
        call    *(%esp)                 /* "call %esp" is undefined */
        movl    %ebx, 1*4(%esp)
        movl    9*4(%esp), %ebx         /* out_regs */
        movl    %eax, 0*4(%ebx)
        popl    %eax                    /* Remove address of code */
        popl    1*4(%ebx)               /* %ebx */
        movl    %ecx, 2*4(%ebx)
        movl    %edx, 3*4(%ebx)
        movl    %esi, 4*4(%ebx)
        movl    %edi, 5*4(%ebx)
        pushf
        popl    6*4(%ebx)
        movl    0*4(%ebx), %eax         /* Return %eax */
        popl    %edi
        popl    %esi
        popl    %ebx
        popl    %ebp
        EPILOGUE(_int86)
