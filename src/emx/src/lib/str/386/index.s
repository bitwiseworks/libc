/ index.s (emx+gcc) -- Copyright (c) 1990-1996 by Eberhard Mattes

#include <emx/asm386.h>

        .globl _STD(index)

/ char *_index (const char *string, int c)
/ {
/   do
/     {
/       if (*string == (char)c)
/         return (char *)string;
/     } while (*string++ != 0);
/   return NULL;
/ }

        .text

        ALIGN

_STD(index):
        PROFILE_NOFRAME
        pushl   %esi
        movl    2*4(%esp), %esi         /* string */
        movb    3*4(%esp), %ah          /* c */
        testb   %ah, %ah
        jz      2f
        ALIGN
1:      lodsb
        cmpb    %ah, %al
        jz      3f
        testb   %al, %al
        jnz     1b
        xorl    %eax, %eax
        jmp     Ldone

/ Special case: search for 0 character

        ALIGN
2:      xchgl   %esi, %edi
        movl    $-1, %ecx
        xorb    %al, %al
        repne
        scasb
        xchgl   %esi, %edi

        ALIGN
3:      lea     -1(%esi), %eax
Ldone:  popl    %esi
        EPILOGUE(_STD(index))
