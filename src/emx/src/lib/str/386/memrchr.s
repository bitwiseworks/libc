/ memrchr.s (emx+gcc) -- Copyright (c) 1996 by Eberhard Mattes

#include <emx/asm386.h>

        .globl __memrchr

/ void *_memrchr (const void *s, int c, size_t n)
/ {
/   size_t i;
/   const char *p;
/
/   p = s;
/   for (i = n; i != 0; --i)
/       if (p[i-1] == (char)c)
/           return (void *)(p+i-1);
/   return NULL;
/ }

/ assumes ds=es!

        .text

        ALIGN

__memrchr:
        PROFILE_NOFRAME
        pushl   %edi
        movl    2*4(%esp), %edi         /* s */
        movb    3*4(%esp), %al          /* c */
        movl    4*4(%esp), %ecx         /* n */
        jecxz   Lnull                   /* not found */
        addl    %ecx, %edi
        decl    %edi
        std
        repne
        scasb
        cld
        jne     Lnull                   /* not found */
        lea     1(%edi), %eax
        jmp     Lreturn

        ALIGN
Lnull:  xorl    %eax, %eax              /* return NULL */
Lreturn:popl    %edi
        EPILOGUE(_memrchr)
