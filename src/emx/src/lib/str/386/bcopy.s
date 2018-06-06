/ bcopy.s (emx+gcc) -- Copyright (c) 1990-1996 by Eberhard Mattes

#include <emx/asm386.h>

        .globl _STD(bcopy)

/ void _bcopy (const void *s1, void *s2, size_t n)
/ {
/   size_t i;
/ 
/   if ((size_t)s1 < (size_t)s2)
/     for (i = n; i > 0; --i)
/       ((char *)s2)[i-1] = ((char *)s1)[i-1];
/   else
/     for (i = 0; i < n; ++i)
/       ((char *)s2)[i] = ((char *)s1)[i];
/ }

/ assumes ds=es!

        .text

        ALIGN

_STD(bcopy):
        PROFILE_NOFRAME
        pushl   %esi
        pushl   %edi
        movl    3*4(%esp), %esi         /* s1 */
        movl    4*4(%esp), %edi         /* s2 */
        movl    5*4(%esp), %ecx         /* n */
        jecxz   Lreturn
        cmpl    %edi, %esi
        jb      2f
        shrl    $2, %ecx
        rep
        movsl
        movl    5*4(%esp), %ecx         /* n */
        andl    $3, %ecx
        rep
        movsb
        jmp     Lreturn

        ALIGN
2:      addl    %ecx, %esi
        addl    %ecx, %edi
        subl    $4, %esi
        subl    $4, %edi
        std
        shrl    $2, %ecx
        rep
        movsl
        addl    $3, %esi
        addl    $3, %edi
        movl    5*4(%esp), %ecx         /* n */
        andl    $3, %ecx
        rep
        movsb
        cld
Lreturn:popl    %edi
        popl    %esi
        EPILOGUE(_STD(bcopy))
