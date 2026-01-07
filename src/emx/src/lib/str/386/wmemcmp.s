/ wmemcmp.s (emx+gcc) -- Copyright (c) 1990-1994 by Eberhard Mattes
/                     -- Copyright (c) 2005      by Knut St. Osmundsen

#include <emx/asm386.h>

        .globl _STD(wmemcmp)

/ assumes ds=es!
/ assumes sizeof(wchar_t) == 2


        .text

        ALIGN

/ int wmemcmp (const wchar_t *wcs1, const wchar_t *wcs2, size_t cwc)
_STD(wmemcmp):
        PROFILE_NOFRAME
        pushl   %esi
        pushl   %edi
        movl    3*4(%esp), %edi         /* wcs1 */
        movl    4*4(%esp), %esi         /* wcs2 */
        movl    5*4(%esp), %ecx         /* cwc */
        xorl    %eax, %eax
        jecxz   Ldone
        repe
        cmpsw
        movw    -1*2(%edi), %ax
        movzwl  -1*2(%esi), %ecx
        subl    %ecx, %eax
Ldone:  popl    %edi
        popl    %esi
        EPILOGUE(_STD(wmemcmp))
