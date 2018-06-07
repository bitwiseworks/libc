/ memcmp.s (emx+gcc) -- Copyright (c) 1990-1994 by Eberhard Mattes

#include <emx/asm386.h>

        .globl _STD(memcmp)

/ int memcmp (const void *s1, const void *s2, size_t n)
/ {
/   size_t i;
/   int d;
/     
/   for (i = 0; i < n; ++i)
/     {
/       d = (int)((unsigned char *)s1)[i] - (int)((unsigned char *)s2)[i];
/       if (d != 0)
/         return d;
/     }
/   return 0;
/ }

/ assumes ds=es!

        .text

        ALIGN

_STD(memcmp):
        PROFILE_NOFRAME
        pushl   %esi
        pushl   %edi
        movl    3*4(%esp), %edi         /* s1 */
        movl    4*4(%esp), %esi         /* s2 */
        movl    5*4(%esp), %ecx         /* n */
        xorl    %eax, %eax
        jecxz   Ldone
        repe
        cmpsb
        movb    -1(%edi), %al
        movzbl  -1(%esi), %ecx
        subl    %ecx, %eax
Ldone:  popl    %edi
        popl    %esi
        EPILOGUE(_STD(memcmp))
