/ memset.s (emx+gcc) -- Copyright (c) 1990-1996 by Eberhard Mattes

#include <emx/asm386.h>

        .globl _STD(memset)

/ void *memset (void *s, int c, size_t n)
/ {
/   size_t i;
/ 
/   for (i = 0; i < n; ++i)
/     ((char *)s)[i] = (char)c;
/   return s;
/ }

/ assumes ds=es!

        .text

        ALIGN

_STD(memset):
        PROFILE_NOFRAME
        pushl   %edi
        movl    2*4(%esp), %edi         /* s */
        movl    3*4(%esp), %eax         /* c */
        movl    4*4(%esp), %ecx         /* n */
        movb    %al, %ah
        movl    %eax, %edx
        shll    $16, %eax
        movw    %dx, %ax
        shrl    $2, %ecx
        rep
        stosl
        movl    4*4(%esp), %ecx         /* n */
        andl    $3, %ecx
        rep
        stosb
        movl    2*4(%esp), %eax         /* s */
        popl    %edi
        EPILOGUE(_STD(memset))
