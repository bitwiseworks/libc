/ swab.s (emx+gcc) -- Copyright (c) 1992-1996 by Eberhard Mattes
/                  -- Copyright (c) 2014 Knut St. Osmundsen <bird-srcspam@anduin.net>

#include <emx/asm386.h>

        .globl _STD(swab)

/ void _swab (const void *src, void *dst, ssize_t n)
/ {
/   char *s = src, *d = dst;
/ 
/   if (n & 1) return;
/   while (n > 0)
/     {
/       d[0] = s[1];
/       d[1] = s[0];
/       d += 2; s += 2; n -= 2;
/     }
/ }

/ assumes ds=es!

        .text

        ALIGN

_STD(swab):
        PROFILE_NOFRAME
        pushl   %esi
        pushl   %edi
        movl    3*4(%esp), %esi         /* src */
        movl    4*4(%esp), %edi         /* dst */
        movl    5*4(%esp), %ecx         /* n */

        testl   $0x80000000, %ecx       /* skip if negative count */
        jnz     Lreturn
        shrl    $1, %ecx
        jz      Lreturn

        ALIGN
1:      lodsw
        xchgb   %al, %ah
        stosw
        loop    1b

Lreturn:popl    %edi
        popl    %esi
        EPILOGUE(_STD(swab))
