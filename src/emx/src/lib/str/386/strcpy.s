/ strcpy.s (emx+gcc) -- Copyright (c) 1990-1996 by Eberhard Mattes

#include <emx/asm386.h>

        .globl _STD(strcpy)

/ char *strcpy (char *string1, const char *string2)
/ {
/   char *dst;
/
/   dst = string1;
/   while ((*dst = *string2) != 0)
/     ++dst, ++string2;
/   return string1;
/ }

/ assumes ds=es!

        .text

        ALIGN

_STD(strcpy):
        PROFILE_NOFRAME
        pushl   %esi
        pushl   %edi
        movl    3*4(%esp), %edi         /* string1 */
        movl    %edi, %edx              /* save for later */
        movl    4*4(%esp), %esi         /* string2 */
        ALIGN
1:      lodsb
        stosb
        testb   %al, %al
        jnz     1b
        movl    %edx, %eax              /* string1 */
        popl    %edi
        popl    %esi
        EPILOGUE(_STD(strcpy))
