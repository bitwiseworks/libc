/ code2.s (emx+gcc) -- Copyright (c) 1992-1996 by Eberhard Mattes

#include <emx/asm386.h>

        .globl  DosGetMessage
        .globl  __msgseg32

__msgseg32:
        .byte   0xff
        .asciz  "MSGSEG32"
        .byte   0x01, 0x80, 0x00, 0x00
        .long   L_tab

        .align  2, 0x90

DosGetMessage:
        PROFILE_NOFRAME
        popl    %ecx                    /* return address */
        pushl   $__msgseg32
        pushl   %ecx
        jmp     DosTrueGetMessage

L_tab:  .short  0x0000
        .short  0xffff
