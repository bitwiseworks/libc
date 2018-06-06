/ ffs.s (emx+gcc) -- Copyright (c) 1993-1996 by Eberhard Mattes

#include <emx/asm386.h>

        .globl  _STD(ffs)

/ int _ffs (int i)
/ {
/   int j;
/
/   if (i == 0)
/     return (0);
/   for (j = 1; !(i & 1); ++j)
/     i >>= 1;
/   return j;
/ }

        .text

        ALIGN

_STD(ffs):
        PROFILE_NOFRAME
        movl    1*4(%esp), %edx         /* i */
        bsfl    %edx, %eax
        jz      Lzero
        incl    %eax
        jmp     Lreturn
        ALIGN
Lzero:  xorl    %eax, %eax
        ALIGN
Lreturn:EPILOGUE(_STD(ffs))
