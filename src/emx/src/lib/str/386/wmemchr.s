/ wmemchr.s (emx+gcc) -- Copyright (c) 1990-1996 by Eberhard Mattes
/                     -- Copyright (c) 2005      by Knut St. Osmundsen

#include <emx/asm386.h>

        .globl _STD(wmemchr)


/ assumes ds=es!
/ assumes sizeof(wchar_t) == 2

        .text

        ALIGN

/ wchar_t *memchr (const wchar_t *wcs, wchar_t wc, size_t cwc)
_STD(wmemchr):
        PROFILE_NOFRAME
        pushl   %edi
        movl    2*4(%esp), %edi         /* wcs */
        movw    3*4(%esp), %ax          /* wc */
        movl    4*4(%esp), %ecx         /* cwc */
        jecxz   Lnull                   /* not found */
        repne
        scasw
        jne     Lnull                   /* not found */
        lea     -1(%edi), %eax
        jmp     Ldone

        ALIGN
Lnull:  xorl    %eax, %eax              /* return NULL */
        ALIGN
Ldone:  popl    %edi
        EPILOGUE(_STD(wmemchr))
