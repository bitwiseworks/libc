/ wcslen.s (emx+gcc) -- Copyright (c) 1990-1996 by Eberhard Mattes
/                    -- Copyright (c) 2005      by Knut St. Osmundsen

#include <emx/asm386.h>

        .globl _STD(wcslen)

/ assumes ds=es!
/ assumes sizeof(wchar_t) == 2

        .text

        ALIGN

/ size_t wcslen(const wchar_t *wcs)
_STD(wcslen):
        PROFILE_NOFRAME
        movl    %edi, %edx
        movl    1*4(%esp), %edi         /* wcs */
        movl    $-1, %ecx
        xorl    %eax, %eax
        repne
        scasw
        movl    $-2, %eax
        subl    %ecx, %eax
        movl    %edx, %edi
        EPILOGUE(_STD(wcslen))
