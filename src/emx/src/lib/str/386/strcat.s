/ strcat.s (emx+gcc) -- Copyright (c) 1990-1996 by Eberhard Mattes

#include <emx/asm386.h>

        .globl _STD(strcat)

/ char *strcat (char *string1, const char *string2)
/ {
/   char *dst;
/ 
/   dst = string1;
/   while (*dst != 0)
/     ++dst;
/   while ((*dst = *string2) != 0)
/     ++dst, ++string2;
/   return string1;
/ }

/ assumes ds=es!

        .text

        ALIGN

_STD(strcat):
        PROFILE_NOFRAME
        pushl   %esi
        pushl   %edi
        movl    3*4(%esp), %edi         /* string1 */
        movl    $-1, %ecx
        movb    $0, %al
        repne
        scasb
        decl    %edi
        movl    4*4(%esp), %esi         /* string2 */
        ALIGN
1:      lodsb
        stosb
        testb   %al, %al
        jnz     1b
        movl    3*4(%esp), %eax         /* string1 */
        popl    %edi
        popl    %esi
        EPILOGUE(_STD(strcat))
