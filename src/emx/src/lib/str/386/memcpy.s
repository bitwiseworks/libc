/ memcpy.s (emx+gcc) -- Copyright (c) 1990-1996 by Eberhard Mattes

#include <emx/asm386.h>

        .globl _STD(memcpy)

/ void *memcpy (void *s1, const void *s2, size_t n)
/ {
/   size_t i;
/
/   for (i = 0; i < n; ++i)
/     ((char *)s1)[i] = ((char *)s2)[i];
/   return s1;
/ }

/ assumes ds=es!

        .text

        ALIGN

_STD(memcpy):
        PROFILE_NOFRAME
        pushl   %esi
        pushl   %edi
        movl    3*4(%esp), %edi         /* s1 */
        movl    %edi, %eax              /* return value */
        movl    4*4(%esp), %esi         /* s2 */
        movl    5*4(%esp), %ecx         /* n */
        movl    %ecx, %edx              /* save for later */
        shrl    $2, %ecx
        rep
        movsl
        movl    %edx, %ecx              /* n */
        andl    $3, %ecx
        rep
        movsb
        popl    %edi
        popl    %esi
        EPILOGUE(_STD(memcpy))
