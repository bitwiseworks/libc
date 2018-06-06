/ wmemset.s (emx+gcc) -- Copyright (c) 1990-1996 by Eberhard Mattes
/                     -- Copyright (c) 2005      by Knut St. Osmundsen

#include <emx/asm386.h>

        .globl _STD(wmemset)


/ assumes ds=es!
/ assumes sizeof(wchar_t) == 2.

        .text

        ALIGN

/ wchar_t *wmemset (wchar_t *wcs, wchar_t wc, size_t cwc)
_STD(wmemset):
        PROFILE_NOFRAME
        pushl   %edi


        movl    3*4(%esp), %eax         /* wc */
        movl    %eax, %edx              /* expand wc to a 32-bit value (assumes it wchar_t == 16-bit!) */
        shll    $16, %eax
        movw    %dx, %ax

        movl    2*4(%esp), %edi         /* wcs */
        movl    %edi, %edx              /* save for return */
        movl    4*4(%esp), %ecx         /* cwc */
        shrl    $1, %ecx
        rep
        stosl
        movl    4*4(%esp), %ecx         /* cwc */
        andl    $1, %ecx
        rep
        stosw
        movl    %edx, %eax              /* wcs */
        popl    %edi
        EPILOGUE(_STD(wmemset))
