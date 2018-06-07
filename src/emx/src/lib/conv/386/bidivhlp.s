/ bidivhlp.s (emx+gcc) -- Copyright (c) 1996 by Eberhard Mattes

#include <emx/asm386.h>

        .globl  __bi_div_estimate
        .globl  __bi_div_subtract
        .globl  __bi_div_add_back

        .text

/* This function isn't exactly that time-critical, but it can be
   written more elegantly in assembly language than in C. */

/ _bi_word _bi_div_estimate (_bi_word *u, _bi_word v1, _bi_word v2)

#define u               8(%ebp)
#define v1              12(%ebp)
#define v2              16(%ebp)

        ALIGN
__bi_div_estimate:
        pushl   %ebp
        movl    %esp, %ebp
        PROFILE_FRAME
        pushl   %ebx
        pushl   %edi
        movl    u, %edi
        movl    -4(%edi), %eax          /* EAX := u[-1] */
        movl    0(%edi), %edx           /* EDX := u[0] */
        cmpl    v1, %edx                /* u[0] == v1? */
        je      Lmax                    /* Yes => use maximum quotient */
        divl    v1

/* EBX = q, EDX = r */

Lcommon:movl    %edx, %ecx              /* Save ECX (r) */
        movl    %eax, %ebx              /* Save EAX (q) */
        mull    v2                      /* EDX:EAX := v2 * q */
        ALIGN
Ltest:  cmpl    %ecx, %edx
        jb      Ldone
        ja      Ldecr
        cmpl    -8(%edi), %eax
        jbe     Ldone
        ALIGN
Ldecr:  decl    %ebx                    /* q := q - 1 */
        addl    v1, %ecx                /* Compute new r for new q */
        jc      Ldone
        subl    v2, %eax                /* EDX:EAX := v2 * q */
        sbbl    $0, %edx
        jmp     Ltest

        ALIGN
Ldone:  movl    %ebx, %eax
Lreturn:popl    %edi
        popl    %ebx
        leave
        EPILOGUE(_bi_div_estimate)

        ALIGN
Lmax:   movl    %edx, %ecx              /* EBX:ECX := EDX:EAX */
        movl    %eax, %ebx
        movl    $-1, %eax
        mull    %ecx
        subl    %ebx, %eax
        sbbl    %ecx, %edx
        movl    %eax, %edx
        movl    $-1, %eax
        jz      Lcommon
        jmp     Lreturn                 /* r>=b, so the test is always false */

#undef u
#undef v1
#undef v2

/* This function is where _bi_div_bb() spends most of its time. */

/ int _bi_div_subtract (_bi_word *u, const _bi_word *v, int n, _bi_word q)

#define u               8(%ebp)
#define v               12(%ebp)
#define n               16(%ebp)
#define q               20(%ebp)

        ALIGN
__bi_div_subtract:
        pushl   %ebp
        movl    %esp, %ebp
        PROFILE_FRAME
        pushl   %ebx
        pushl   %esi
        pushl   %edi
        movl    u, %edi                 /* EDI := u */
        movl    v, %esi                 /* ESI := v */
        movl    n, %ecx                 /* ECX := n */
        xorl    %ebx, %ebx              /* mul_carry := 0, sub_borrow := 0 */
        ALIGN
L1:     movl    (%esi), %eax
        leal    4(%esi), %esi
        mull    q
        addl    %ebx, %eax
        adcl    $0, %edx
        movl    %edx, %ebx
        subl    %eax, (%edi)
        adcl    $0, %ebx
        leal    4(%edi), %edi
        decl    %ecx
        jnz     L1
        subl    %ebx, (%edi)
        sbbl    %eax, %eax
        popl    %edi
        popl    %esi
        popl    %ebx
        leave
        EPILOGUE(_bi_div_subtract)

#undef u
#undef v
#undef n
#undef q

/* This function isn't exactly that time-critical, but it can be
   written more elegantly in assembly language than in C. */

/ int _bi_div_add_back (_bi_word *u, const _bi_word *v, int n)

#define u               8(%ebp)
#define v               12(%ebp)
#define n               16(%ebp)

        ALIGN
__bi_div_add_back:
        pushl   %ebp
        movl    %esp, %ebp
        PROFILE_FRAME
        pushl   %esi
        pushl   %edi
        movl    u, %edi                 /* EDI := u */
        movl    v, %esi                 /* ESI := v */
        movl    n, %ecx                 /* ECX := n */
        incl    %ecx
        clc
        ALIGN
L2:     movl    (%esi), %eax
        leal    4(%esi), %esi
        adcl    %eax, (%edi)
        leal    4(%edi), %edi
        decl    %ecx
        jnz     L2
        popl    %edi
        popl    %esi
        leave
        EPILOGUE(_bi_div_add_back)
