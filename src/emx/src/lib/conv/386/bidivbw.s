/ bidivbw.s (emx+gcc) -- Copyright (c) 1996 by Eberhard Mattes

#include <emx/asm386.h>

        .globl  __bi_div_rem_bw

#define n       0
#define v       4

/ int _bi_div_rem_bw (_bi_bigint *quot, int quot_words, _bi_word *rem,
/                     const _bi_bigint *num, _bi_word den);

#define quot            8(%ebp)
#define quot_words      12(%ebp)
#define rem             16(%ebp)
#define num             20(%ebp)
#define den             24(%ebp)

        .text
__bi_div_rem_bw:
        pushl   %ebp
        movl    %esp, %ebp
        PROFILE_FRAME
        pushl   %ebx
        pushl   %esi
        pushl   %edi
        movl    quot, %edi              /* EDI := quot */
        cmpl    $0, den                 /* Division by zero? */
        je      Loverflow               /* Yes => return 1 */
        movl    num, %esi               /* ESI := num */
        movl    n(%esi), %ecx           /* ECX := num->n */
        cmpl    quot_words, %ecx        /* num->n > quot_words? */
        ja      Loverflow               /* Yes => return 1 */
        testl   %ecx, %ecx              /* Is the numerator zero? */
        jz      Lnum_zero               /* Yes => quotient is zero */

/* See Donald Ervin Knuth, The Art of Computer Programming, Volume 2,
   4.3.1, Exercise 16.  See also /emx/src/lib/gcc/386/ulldiv.s. */

        movl    v(%esi), %esi
        movl    v(%edi), %edi
        leal    -4(%esi,%ecx,4), %esi   /* ESI := &num->v[num->n-1] */
        leal    -4(%edi,%ecx,4), %edi   /* EDI := &quot->v[quot->n-1] */
        xorl    %edx, %edx              /* r := 0 */
        movl    den, %ebx               /* EBX := den */
        ALIGN
L1:     movl    (%esi), %eax
        divl    %ebx                    /* r := (rb+num->v[j]) % den */
        leal    -4(%esi), %esi          /* q[j] := (rb+num->v[j]) / den */
        movl    %eax, (%edi)
        leal    -4(%edi), %edi
        decl    %ecx
        jnz     L1

        movl    rem, %ebx
        movl    num, %esi
        movl    quot, %edi
        movl    %edx, (%ebx)            /* *rem := r */
        movl    n(%esi), %ecx           /* ECX := num->n */
        movl    v(%edi), %ebx
        cmpl    $1, -4(%ebx,%ecx,4)     /* quot->v[num->n-1] != 0? */
        sbbl    $0, %ecx                /* Yes => ECX := ECX-1 */
        movl    %ecx, n(%edi)           /* quot->n := num->n or num->n-1 */
        xorl    %eax, %eax
        ALIGN
Lreturn:
        popl    %edi
        popl    %esi
        popl    %ebx
        leave
        EPILOGUE(_bi_div_rem_bw)

        ALIGN
Loverflow:
        movl    $1, %eax
        jmp     Lreturn

        ALIGN
Lnum_zero:
        movl    rem, %ebx
        movl    $0, (%ebx)
        movl    $0, n(%edi)
        xorl    %eax, %eax
        jmp     Lreturn
