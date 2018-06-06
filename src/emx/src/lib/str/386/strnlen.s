/ strnlen.s (emx+gcc) -- Copyright (c) 2004 knut st. osmundsen
/   based on strlen.s -- Copyright (c) 1990-1996 by Eberhard Mattes

#include <emx/asm386.h>

        .globl ___strnlen

/ size_t strnlen (const char *string, size_t maxlen)
/ {
/   size_t i;
/
/   i = 0;
/   while (string[i] != 0 && maxlen-- > 0) ++i;
/   return i;
/ }

/ assumes ds=es!

        .text

        ALIGN

___strnlen:
        PROFILE_NOFRAME
        pushl   %edi
        movl    3*4(%esp), %ecx         /* maxlen */
        xorl    %eax, %eax
        orl     %ecx, %ecx
        jnz     next
        jmp     done
next:
        movl    2*4(%esp), %edi         /* string */
        movl    %edi, %edx
        repne
        scasb
        setz    %cl
        addl    %edi, %eax
        subl    %edx, %eax
        movzx   %cl, %ecx
        subl    %ecx, %eax
done:
        popl    %edi
        EPILOGUE(___strnlen)
