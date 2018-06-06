/ wmemcpy.s (emx+gcc) -- Copyright (c) 1990-1996 by Eberhard Mattes
/                     -- Copyright (c) 2005      by Knut St. Osmundsen

#include <emx/asm386.h>

        .globl _STD(wmemcpy)

/ assumes ds=es!
/ assumes sizeof(wchar_t) == 2

        .text

        ALIGN

/ wchar_t *wmemcpy (wchar_t *wcs1, const wchar_t *wcs2, size_t cwc)
_STD(wmemcpy):
        PROFILE_NOFRAME
        pushl   %esi
        pushl   %edi
        movl    3*4(%esp), %edi         /* wcs1 */
        movl    %edi, %eax              /* (return value) */
        movl    4*4(%esp), %esi         /* wcs2 */
        movl    5*4(%esp), %ecx         /* cwc */
        movl    %ecx, %edx
        shrl    $1, %ecx
        rep
        movsl
        movl    %edx, %ecx              /* cwc */
        andl    $1, %ecx
        rep
        movsw
        popl    %edi
        popl    %esi
        EPILOGUE(_STD(wmemcpy))
