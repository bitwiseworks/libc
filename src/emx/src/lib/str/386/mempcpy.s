/ mempcpy.s (emx+gcc) -- Copyright (c) 1990-1996 by Eberhard Mattes

#include <emx/asm386.h>

        .globl ___mempcpy

/ void *mempcpy (void *s1, const void *s2, size_t n)
/ {
/   size_t i;
/
/   for (i = 0; i < n; ++i)
/     ((char *)s1)[i] = ((char *)s2)[i];
/   return s1+n;
/ }

/ assumes ds=es!

        .text

        ALIGN

___mempcpy:
        PROFILE_NOFRAME
        pushl   %esi
        pushl   %edi
        movl    3*4(%esp), %edi         /* s1 */
        movl    4*4(%esp), %esi         /* s2 */
        movl    5*4(%esp), %ecx         /* n */
        shrl    $2, %ecx
        rep
        movsl
        movl    5*4(%esp), %ecx         /* n */
        andl    $3, %ecx
        rep
        movsb
        movl    %edi, %eax              /* s1+n */
        popl    %edi
        popl    %esi
        EPILOGUE(___mempcpy)
