/ memmove.s (emx+gcc) -- Copyright (c) 1990-1996 by Eberhard Mattes

#include <emx/asm386.h>

        .globl _STD(memmove)

/ void *memmove (void *s1, const void *s2, size_t n)
/ {
/   size_t i;
/   
/   if ((size_t)s1 < (size_t)s2)
/     for (i = 0; i < n; ++i)
/       ((char *)s1)[i] = ((char *)s2)[i];
/   else
/     for (i = n; i > 0; --i)                          /* i is unsigned! */
/       ((char *)s1)[i-1] = ((char *)s2)[i-1];
/   return s1;
/ }

/ assumes ds=es!

        .text

        ALIGN

_STD(memmove):
        PROFILE_NOFRAME
        pushl   %esi
        pushl   %edi
        movl    3*4(%esp), %edi         /* s1 */
        movl    4*4(%esp), %esi         /* s2 */
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
Lreturn:movl    3*4(%esp), %eax         /* s1 */
        popl    %edi
        popl    %esi
        EPILOGUE(_STD(memmove))
