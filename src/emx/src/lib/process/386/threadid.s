/ threadid.s (emx+gcc) -- Copyright (c) 1993-1996 by Eberhard Mattes

#include <emx/asm386.h>

        .globl  ___threadid

        .text

        ALIGN

___threadid:
        push    %fs
        movl    $DosTIB, %eax
        movl    %eax, %fs
        fs
        movl    12, %eax                /* ptib2 */
        pop     %fs
        EPILOGUE(__threadid)
