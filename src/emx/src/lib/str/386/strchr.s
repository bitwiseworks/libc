/ strchr.s (emx+gcc) -- Copyright (c) 1990-1996 by Eberhard Mattes

#include <emx/asm386.h>

        .globl _STD(strchr)

/ char *strchr (const char *string, int c)
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

_STD(strchr):
        PROFILE_NOFRAME
        pushl   %esi
        movl    2*4(%esp), %esi         /* string */
        movb    3*4(%esp), %ah          /* c */
        testb   %ah, %ah
        jz      Lstrlen
        ALIGN
1:      lodsb
        cmpb    %ah, %al
        jz      Lfound
        testb   %al, %al
        jnz     1b
        xorl    %eax, %eax
        jmp     Lreturn

/ Special case: search for 0 character

        ALIGN
Lstrlen:xchgl   %esi, %edi
        movl    $-1, %ecx
        xorb    %al, %al
        repne
        scasb
        xchgl   %esi, %edi

        ALIGN
Lfound: lea     -1(%esi), %eax
Lreturn:popl    %esi
        EPILOGUE(_STD(strchr))
