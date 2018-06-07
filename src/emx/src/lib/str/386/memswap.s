/ memswap.s (emx+gcc) -- Copyright (c) 1992-1996 by Eberhard Mattes

#include <emx/asm386.h>

        .globl __memswap

/ void _memswap (void *s1, void *s2, size_t n)
/ {
/   char *c1, *c2, c;
/   int *i1, *i2, i;
/ 
/   i1 = s1; i2 = s2;
/   while (n >= sizeof (int))
/     {
/       i = *i1; *i1++ = *i2; *i2++ = i;
/       n -= sizeof (int);
/       }
/   c1 = (char *)i1; c2 = (char *)i2;
/   while (n >= 1)
/     {
/       c = *c1; *c1++ = *c2; *c2++ = c;
/       --n;
/     }
/ }

/ assumes ds=es!

        .text

        ALIGN

__memswap:
        PROFILE_NOFRAME
        pushl   %esi
        pushl   %edi
        movl    3*4(%esp), %edi         /* s1 */
        movl    4*4(%esp), %esi         /* s2 */
        movl    5*4(%esp), %ecx         /* n */
        shrl    $2, %ecx
        jz      2f
        ALIGN
1:      movl    (%edi), %eax
        xchgl   (%esi), %eax
        stosl
        addl    $4, %esi
        loop    1b
2:      movl    5*4(%esp), %ecx         /* n */
        andl    $3, %ecx
        jz      Lreturn
        ALIGN
3:      movb    (%edi), %al
        xchgb   (%esi), %al
        stosb
        incl    %esi
        loop    3b
Lreturn:popl    %edi
        popl    %esi
        EPILOGUE(_memswap)
