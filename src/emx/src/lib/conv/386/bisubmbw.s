/ bisubmbw.s (emx+gcc) -- Copyright (c) 1996 by Eberhard Mattes

#include <emx/asm386.h>

        .globl  __bi_sub_mul_bw

#define n       0
#define v       4

/ int _bi_sub_mul_bw (_bi_bigint *dst, const _bi_bigint *src, _bi_word factor)

#define dst             8(%ebp)
#define src             12(%ebp)
#define factor          16(%ebp)
#define dst_n_extra     -4(%ebp)

        .text
__bi_sub_mul_bw:
        pushl   %ebp
        movl    %esp, %ebp
        PROFILE_FRAME
        subl    $4, %esp
        pushl   %ebx
        pushl   %esi
        pushl   %edi
        cmpl    $0, factor
        je      Lnothing
        movl    src, %esi
        movl    n(%esi), %ecx
        testl   %ecx, %ecx
        jz      Lnothing
        movl    dst, %edi
        movl    n(%edi), %eax
        subl    %ecx, %eax
        jb      Lret_one
        movl    %eax, dst_n_extra
        movl    v(%edi), %edi
        movl    v(%esi), %esi
        xorl    %ebx, %ebx              /* EBX = mul_carry + sub_borrow */

/* This loops processes all the words of src. */

        ALIGN
L1:     movl    (%esi), %eax
        mull    factor
        leal    4(%esi), %esi
        addl    %ebx, %eax
        adcl    $0, %edx
        movl    %edx, %ebx
        subl    %eax, (%edi)
        adcl    $0, %ebx
        leal    4(%edi), %edi
        dec     %ecx
        jnz     L1

        movl    dst_n_extra, %ecx

/* Process carry from the multiplication.  This needs to be done
   only once, no loop. */

        testl   %ebx, %ebx
        jz      Lno_mul_carry
        testl   %ecx, %ecx
        jz      Lrestore                /* Overflow */
        subl    %ebx, (%edi)
        leal    4(%edi), %edi
        sbbl    %ebx, %ebx              /* EBX non-zero iff borrow */
        dec     %ecx
        ALIGN
Lno_mul_carry:

/* Process borrow for the remaining words of dst. */

        testl   %ebx, %ebx
        jz      Lno_borrow
        testl   %ecx, %ecx
        jz      Lrestore
        ALIGN
L2:     subl    $1, (%edi)
        decl    %ecx
        leal    4(%edi), %edi
        jnc     Lno_borrow
        jnz     L2
        jmp     Lrestore

        ALIGN
Lno_borrow:

/* Normalize. */

        movl    dst, %ebx
        movl    n(%ebx), %ecx
        ALIGN
L3:     cmpl    $0, -4(%edi)
        jnz     Ldone
        subl    $4, %edi
        dec     %ecx
        jnz     L3
        ALIGN
Ldone:  movl    %ecx, n(%ebx)
        xorl    %eax, %eax
        ALIGN
Lreturn:
        popl    %edi
        popl    %esi
        popl    %ebx
        leave
        EPILOGUE(_bi_sub_mul_bw)

/* The result would be negative.  Restore the original DST. */

        ALIGN
Lrestore:
        movl    src, %esi
        movl    dst, %edi
        movl    n(%esi), %ecx
        movl    v(%edi), %edi
        movl    v(%esi), %esi
        xorl    %ebx, %ebx
        ALIGN
L4:     movl    (%esi), %eax
        mull    factor
        leal    4(%esi), %esi
        addl    %ebx, %eax
        adcl    $0, %edx
        movl    %edx, %ebx
        addl    %eax, (%edi)
        adcl    $0, %ebx
        leal    4(%edi), %edi
        dec     %ecx
        jnz     L4

        cmpl    $0, dst_n_extra
        je      Lrestore_done
        addl    %ebx, (%edi)
        ALIGN
Lrestore_done:
        movl    $1, %eax
        jmp     Lreturn

        ALIGN
Lnothing:
        xorl    %eax, %eax
        jmp     Lreturn

        ALIGN
Lret_one:
        movl    $1, %eax
        jmp     Lreturn
