/ memchr.s (emx+gcc) -- Copyright (c) 1990-1996 by Eberhard Mattes

#include <emx/asm386.h>

        .globl _STD(memchr)

/ void *memchr (const void *s, int c, size_t n)
/ {
/   size_t i;
/   const char *p;
/
/   p = s;
/   for (i = 0; i < n; ++i)
/       if (p[i] == (char)c)
/           return (void *)(p+i);
/   return NULL;
/ }

/ assumes ds=es!

        .text

        ALIGN

_STD(memchr):
        PROFILE_NOFRAME
        pushl   %edi
        movl    2*4(%esp), %edi         /* s */
        movb    3*4(%esp), %al          /* c */
        movl    4*4(%esp), %ecx         /* n */
        jecxz   Lnull                   /* not found */
        repne
        scasb
        jne     Lnull                   /* not found */
        lea     -1(%edi), %eax
        jmp     Ldone

        ALIGN
Lnull:  xorl    %eax, %eax              /* return NULL */
        ALIGN
Ldone:  popl    %edi
        EPILOGUE(_STD(memchr))
