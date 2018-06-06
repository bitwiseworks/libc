/ memcount.s (emx+gcc) -- Copyright (c) 1992-1996 by Eberhard Mattes

#include <emx/asm386.h>

        .globl __memcount

/ size_t _memcount (const void *mem, int c, size_t n)
/ {
/   size_t i;
/ 
/   i = 0;
/   while (n > 0)
/     {
/       if (*mem == c) ++i;
/       ++mem; --n;
/     }
/   return i;
/ }

/ assumes ds=es!

        .text

        ALIGN

__memcount:
        PROFILE_NOFRAME
        pushl   %edi
        movl    2*4(%esp), %edi         /* mem */
        movl    3*4(%esp), %eax         /* c */
        movl    4*4(%esp), %ecx         /* n */
        xorl    %edx, %edx
        jecxz   Ldone
         ALIGN
1:      repne
        scasb
        jne     Ldone
        incl    %edx
        testl   %ecx, %ecx
        jne     1b
Ldone:  movl    %edx, %eax
        popl    %edi
        EPILOGUE(_memcount)
