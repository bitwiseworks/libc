/ bzero.s (emx+gcc) -- Copyright (c) 1990-1996 by Eberhard Mattes

#include <emx/asm386.h>

        .globl _STD(bzero)

/ void _bzero (void *s, size_t n)
/ {
/   size_t i;
/ 
/   for (i = 0; i < n; ++i)
/     ((char *)s)[i] = 0;
/ }

/ assumes ds=es!

        .text

        ALIGN

_STD(bzero):
        PROFILE_NOFRAME
        pushl   %edi
        movl    2*4(%esp), %edi         /* s */
        movl    3*4(%esp), %ecx         /* n */
        xorl    %eax, %eax
        shrl    $2, %ecx
        rep
        stosl
        movl    3*4(%esp), %ecx         /* n */
        andl    $3, %ecx
        rep
        stosb
        popl    %edi
        EPILOGUE(_STD(bzero))
