/ strcmp.s (emx+gcc) -- Copyright (c) 1990-1996 by Eberhard Mattes

#include <emx/asm386.h>

        .globl _STD(strcmp)

/ int strcmp (const char *string1, const char *string2)
/ {
/   int d;
/   
/   for (;;)
/     {
/       d = (int)(unsigned char)*string1 - (int)(unsigned char)*string2;
/       if (d != 0 || *string1 == 0 || *string2 == 0)
/         return d;
/       ++string1; ++string2;
/     }
/ }

/ assumes ds=es!

        .text

        ALIGN

_STD(strcmp):
        PROFILE_NOFRAME
        pushl   %esi
        pushl   %edi
        movl    3*4(%esp), %esi         /* string1 */
        movl    4*4(%esp), %edi         /* string2 */
        xorl    %eax, %eax
        ALIGN
1:      lodsb
        scasb
        jne     Ldiff
        testb   %al, %al
        jne     1b
        jmp     Lreturn

        ALIGN
Ldiff:  movzbl  -1(%edi), %ecx
        subl    %ecx, %eax
Lreturn:popl    %edi
        popl    %esi
        EPILOGUE(_STD(strcmp))
