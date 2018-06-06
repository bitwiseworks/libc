/ bimulbw.s (emx+gcc) -- Copyright (c) 1996 by Eberhard Mattes

#include <emx/asm386.h>

        .globl  __bi_mul_bw

#define n       0
#define v       4

/ int _bi_mul_bw (_bi_bigint *dst, int dst_words,
/                 const _bi_bigint *src, _bi_word factor)

#define dst             8(%ebp)
#define dst_words       12(%ebp)
#define src             16(%ebp)
#define factor          20(%ebp)

        .text
__bi_mul_bw:
        pushl   %ebp
        movl    %esp, %ebp
        PROFILE_FRAME
        pushl   %ebx
        pushl   %esi
        pushl   %edi
        cmpl    $0, factor
        je      Lzero
        movl    src, %esi
        movl    n(%esi), %ecx
        cmpl    dst_words, %ecx
        ja      Lcarry
        testl   %ecx, %ecx
        jz      Lzero
        movl    dst, %edi
        movl    %ecx, n(%edi)
        movl    v(%edi), %edi
        movl    v(%esi), %esi
        xorl    %ebx, %ebx              /* EBX = carry */

/* This loops processes all the words of src. */

        ALIGN
L1:     movl    (%esi), %eax
        mull    factor
        leal    4(%esi), %esi
        addl    %ebx, %eax
        adcl    $0, %edx
        movl    %eax, (%edi)
        movl    %edx, %ebx
        leal    4(%edi), %edi
        dec     %ecx
        jnz     L1

        test    %ebx, %ebx
        jz      Ldone

/* Carry from the most significant word.  Check dst_words. */

        movl    dst, %edx
        movl    n(%edx), %eax
        cmpl    dst_words, %eax
        jae     Lcarry
        movl    %ebx, (%edi)
        incl    n(%edx)
        ALIGN
Ldone:  xorl    %eax, %eax
        ALIGN
Lreturn:
        popl    %edi
        popl    %esi
        popl    %ebx
        leave
        EPILOGUE(_bi_mul_bw)

        ALIGN
Lzero:  movl    dst, %edi
        movl    $0, 0(%edi)
        xorl    %eax, %eax
        jmp     Lreturn

        ALIGN
Lcarry: movl    $1, %eax
        jmp     Lreturn
