/ strlen.s (emx+gcc) -- Copyright (c) 1990-1996 by Eberhard Mattes

#include <emx/asm386.h>

        .globl _STD(strlen)

/ size_t strlen (const char *string)
/ {
/   size_t i;
/ 
/   i = 0;
/   while (string[i] != 0) ++i;
/   return i;
/ }

/ assumes ds=es!

        .text

        ALIGN

_STD(strlen):
        PROFILE_NOFRAME
        pushl   %edi
        movl    2*4(%esp), %edi         /* string */
        movl    $-1, %ecx
        xorb    %al, %al
        repne
        scasb
        movl    $-2, %eax
        subl    %ecx, %eax
        popl    %edi
        EPILOGUE(_STD(strlen))
