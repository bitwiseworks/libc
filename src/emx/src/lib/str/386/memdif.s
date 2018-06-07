/ memdif.s (emx+gcc) -- Copyright (c) 1993-1996 by Eberhard Mattes

#include <emx/asm386.h>

        .globl __memdif

/ size_t _memdif (const void *mem1, const void *mem2, size_t n)
/ {
/   size_t i;
/     
/   for (i = 0; i < n; ++i)
/     if (((char *)mem1)[i] != ((char *)mem2)[i])
/       return i;
/   return _MEMDIF_EQ;
/ }

/ assumes ds=es!

        .text

        ALIGN

__memdif:
        PROFILE_NOFRAME
        pushl   %esi
        pushl   %edi
        movl    3*4(%esp), %edi         /* mem1 */
        movl    4*4(%esp), %esi         /* mem2 */
        movl    5*4(%esp), %ecx         /* n */
        xorl    %eax, %eax
        jecxz   Lequal
        repe
        cmpsb
        je      Lequal
        decl    %edi
        subl    3*4(%esp), %edi
        movl    %edi, %eax
        jmp     Ldone

        ALIGN
Lequal: movl    $-1, %eax               /* _MEMDIF_EQ */
Ldone:  popl    %edi
        popl    %esi
        EPILOGUE(_memdif)
