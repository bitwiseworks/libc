/ gettid.s (emx+gcc) -- Copyright (c) 1996 by Eberhard Mattes

#include <emx/asm386.h>

        .globl  __gettid

        .text

        ALIGN

__gettid:
        push    %fs
        movl    $DosTIB, %eax
        movl    %eax, %fs
        fs
        movl    12, %eax                /* ptib2 */
        movl    0(%eax), %eax           /* TID */
        pop     %fs
        EPILOGUE(_gettid)
