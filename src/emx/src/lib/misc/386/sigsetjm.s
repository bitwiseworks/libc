/ sigsetjm.s (emx+gcc) -- Copyright (c) 1990-1996 by Eberhard Mattes

#include <emx/asm386.h>

        .globl _STD(sigsetjmp), _STD(siglongjmp)

        .text
        ALIGN

#define J_EBX  0
#define J_ESI  4
#define J_EDI  8
#define J_ESP 12
#define J_EBP 16
#define J_EIP 20
#define J_XCP 24
#define J_SAVE 48
#define J_MASK 52

/ Words at offsets 28..44 are reserved

/ int sigsetjmp (sigjmp_buf here, int savemask)

_STD(sigsetjmp):
        PROFILE_NOFRAME
        movl    1*4(%esp), %edx         /* here */
        movl    %ebx, J_EBX(%edx)
        movl    %esi, J_ESI(%edx)
        movl    %edi, J_EDI(%edx)
        movl    %ebp, J_EBP(%edx)
        movl    %esp, J_ESP(%edx)
        movl    0*4(%esp), %eax         /* return address */
        movl    %eax, J_EIP(%edx)
        fs
        movl    0, %eax                 /* Exception handler */
        movl    %eax, J_XCP(%edx)
        movl    2*4(%esp), %eax         /* savemask */
        movl    %eax, J_SAVE(%edx)
        testl   %eax, %eax
        je      1f
        leal    J_MASK(%edx), %eax
        pushl   %eax                    /* oset */
        pushl   $0                      /* iset */
        pushl   $3                      /* how: SIG_SETMASK */
        call    _sigprocmask
        addl    $12, %esp
1:      xorl    %eax, %eax
        EPILOGUE(_STD(sigsetjmp))

        ALIGN

/ void siglongjmp (sigjmp_buf there, int n)

_STD(siglongjmp):
        PROFILE_NOFRAME
        movl    1*4(%esp), %eax         /* there */
        cmpl    $0, J_SAVE(%eax)
        je      1f
        leal    J_MASK(%eax), %eax
        pushl   $0                      /* oset */
        pushl   %eax                    /* iset */
        pushl   $3                      /* how: SIG_SETMASK */
        call    _sigprocmask
        addl    $12, %esp
1:      movl    1*4(%esp), %eax         /* there */
        pushl   J_XCP(%eax)
        call    ___unwind2              /* unwind signal handlers */
        addl    $4, %esp
        movl    1*4(%esp), %edx         /* there */
        movl    2*4(%esp), %eax         /* n */
        testl   %eax, %eax
        jne     2f
        incl    %eax
2:      movl    J_EBX(%edx), %ebx
        movl    J_ESI(%edx), %esi
        movl    J_EDI(%edx), %edi
        movl    J_EBP(%edx), %ebp
        movl    J_ESP(%edx), %esp
        movl    J_EIP(%edx), %edx
        movl    %edx, 0*4(%esp)         /* return address */
        EPILOGUE(_STD(siglongjmp))      /* well, ... */
