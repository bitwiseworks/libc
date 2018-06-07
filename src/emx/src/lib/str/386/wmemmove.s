/ wmemmove.s (emx+gcc) -- Copyright (c) 1990-1996 by Eberhard Mattes
/                      -- Copyright (c) 2005      by Knut St. Osmundsen

#include <emx/asm386.h>

        .globl _STD(wmemmove)

/ wchar_t *wmemmove (wchar_t *wcs1, const wchar_T *wcs2, size_t cwc)
/ {
/   size_t i;
/
/   if ((size_t)wcs1 < (size_t)wcs2)
/     for (i = 0; i < wcs; ++i)
/       ((char *)wcs1)[i] = ((char *)wcs2)[i];
/   else
/     for (i = wcs; i > 0; --i)                          /* i is unsigned! */
/       ((char *)wcs1)[i-1] = ((char *)wcs2)[i-1];
/   return wcs1;
/ }

/ assumes ds=es!
/ assumes sizeof(wchar_t) == 2

        .text

        ALIGN

_STD(wmemmove):
        PROFILE_NOFRAME
        pushl   %esi
        pushl   %edi
        movl    5*4(%esp), %ecx         /* wcs */
        jecxz   Lreturn
        movl    3*4(%esp), %edi         /* wcs1 */
        movl    %edi, %eax              /* return value */
        movl    4*4(%esp), %esi         /* wcs2 */

        cmpl    %edi, %esi
        jb      2f

        shrl    $1, %ecx
        rep
        movsl
        movl    5*4(%esp), %ecx         /* wcs */
        andl    $1, %ecx
        rep
        movsw
        jmp     Lreturn

        ALIGN

2:      lea     -4(%esi, %ecx, 2), %esi
        lea     -4(%edi, %ecx, 2), %edi
        std
        shrl    $1, %ecx
        rep
        movsl
        addl    $2, %esi
        addl    $2, %edi
        movl    5*4(%esp), %ecx         /* wcs */
        andl    $1, %ecx
        rep
        movsw
        cld
Lreturn:
        popl    %edi
        popl    %esi
        EPILOGUE(_STD(wmemmove))
