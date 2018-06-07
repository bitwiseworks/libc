/ bimulbb.s (emx+gcc) -- Copyright (c) 1996 by Eberhard Mattes

#include <emx/asm386.h>

        .globl  __bi_mul_bb

#define n       0
#define v       4

/ int _bi_mul_bb (_bi_bigint *dst, int dst_words,
/                 const _bi_bigint *src1, const _bi_bigint *src2)

#define dst             8(%ebp)
#define dst_words       12(%ebp)
#define src1            16(%ebp)
#define src2            20(%ebp)
#define n1              -4(%ebp)
#define n2              -8(%ebp)
#define dn              -12(%ebp)
#define v1              -16(%ebp)
#define v2              -20(%ebp)
#define dv              -24(%ebp)
#define factor          -28(%ebp)

        .text
__bi_mul_bb:
        pushl   %ebp
        movl    %esp, %ebp
        PROFILE_FRAME
        subl    $28, %esp
        pushl   %ebx
        pushl   %esi
        pushl   %edi
        movl    dst, %edi
        movl    src1, %esi              /* ESI := src1 */
        movl    src2, %ebx              /* EBX := src2 */
        movl    n(%esi), %ecx           /* ECX := src1->n */
        movl    n(%ebx), %eax           /* EAX := src2->n */

/* Set dst to zero if either src1 or src2 point to zero.  The code
   below does not work if the word count of either factor is zero. */

        testl   %ecx, %ecx              /* src1->n == 0? */
        jz      Lzero
        testl   %eax, %eax              /* src2->n == 0? */
        jz      Lzero

        movl    %ecx, n1                /* n1 := src1->n */
        movl    %eax, n2                /* n2 := src2->n */
        addl    %eax, %ecx
        cmpl    dst_words, %ecx
        ja      Loverflow

        movl    v(%esi), %eax
        movl    v(%ebx), %edx
        movl    %eax, v1                /* v1 := src1->v */
        movl    %edx, v2                /* v2 := src2->v */

        movl    %ecx, dn                /* dn := src1->n + src2->n */
        movl    v(%edi), %edi
        xorl    %eax, %eax
        movl    %edi, dv                /* dv := dst->v */
        rep
        stosl

/* Outer loop, over words of src2. */

        ALIGN
L1:     movl    dv, %edi                /* EDI := &dst->v[i] */
        movl    v1, %esi                /* ESI := &src1->v[0] */
        movl    v2, %eax
        movl    n1, %ecx
        movl    (%eax), %edx
        movl    %edx, factor            /* factor := src2->v[i] */
        xorl    %ebx, %ebx              /* carry := 0 */

/* Inner loop, over words of src1. */

        ALIGN
L2:     movl    (%esi), %eax
        mull    factor
        addl    $4, %esi
        addl    %ebx, %eax
        adcl    $0, %edx
        addl    %eax, (%edi)
        leal    4(%edi), %edi
        adcl    $0, %edx
        decl    %ecx
        movl    %edx, %ebx
        jnz     L2

/* Handle carry (EBX). */

        test    %ebx, %ebx
        jz      Lnext
        add     %ebx, (%edi)
        leal    4(%edi), %edi

/* Handle carry (CY). */

        jnc     Lnext
        ALIGN
L3:     add     $1, (%edi)
        leal    4(%edi), %edi
        jc      L3

/* End of outer loop. */

        ALIGN
Lnext:  addl    $4, v2
        addl    $4, dv
        decl    n2
        jnz     L1

/* Normalize product.  Note that dn is non-zero. */

        movl    dst, %edi
        movl    dn, %eax
        movl    v(%edi), %esi
        cmpl    $1, -4(%esi,%eax,4)
        sbbl    $0, %eax                /* Decr EAX if dst->v[dn-1] == 0 */
        movl    %eax, n(%edi)
        ALIGN
Ldone:  xorl    %eax, %eax
        ALIGN
Lreturn:
        popl    %edi
        popl    %esi
        popl    %ebx
        leave
        EPILOGUE(_bi_mul_bb)

        ALIGN
Loverflow:
        movl    $1, %eax
        jmp     Lreturn

        ALIGN
Lzero:  movl    $0, n(%edi)
        jmp     Ldone
