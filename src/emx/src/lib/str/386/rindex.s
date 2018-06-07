/ rindex.s (emx+gcc) -- Copyright (c) 1990-1996 by Eberhard Mattes

#include <emx/asm386.h>

        .globl _STD(rindex)
  
/ char *_rindex (const char *string, int c)
/ {
/   int i;
/   
/   i = 0;
/   while (string[i] != 0) ++i;
/   while (i >= 0)
/     if (string[i] == (char)c)
/       return ((char *)(string+i));
/     else
/       --i;
/   return NULL;
/ }

        .text

        ALIGN

_STD(rindex):
        PROFILE_NOFRAME
        pushl   %edi
        movl    2*4(%esp), %edi         /* string */
        movl    $-1, %ecx
        xorb    %al, %al
        repne
        scasb
        notl    %ecx                    /* ecx = strlen (string) + 1 */
        decl    %edi                    /* edi = string + strlen (string) */
        movb    3*4(%esp), %al          /* c */
        std
        repne
        scasb
        cld
        jne     Lnull
        lea     1(%edi), %eax
        jmp     Lreturn

        ALIGN
Lnull:  xorl    %eax, %eax
Lreturn:popl    %edi
        EPILOGUE(_STD(rindex))
