/ biaddbb.s (emx+gcc) -- Copyright (c) 1996 by Eberhard Mattes

#include <emx/asm386.h>

        .globl  __bi_add_bb

#define n       0
#define v       4

/ int _bi_add_bb (_bi_bigint *dst, int dst_words,
/                 const _bi_bigint *src1, const _bi_bigint *src2)

#define dst             8(%ebp)
#define dst_words       12(%ebp)
#define src1            16(%ebp)
#define src2            20(%ebp)

        .text
__bi_add_bb:
        pushl   %ebp
        movl    %esp, %ebp
        PROFILE_FRAME
        pushl   %ebx
        pushl   %esi
        pushl   %edi
        movl    src1, %ebx
        movl    src2, %esi
        movl    n(%ebx), %ecx           /* ECX := src1->n */
        movl    n(%esi), %edx           /* EDX := src2->n */
        cmpl    %edx, %ecx              /* src1->n < src2->n ? */
        jb      Lsetup                  /* yes -> skip */
        xchgl   %ebx, %esi
        xchgl   %edx, %ecx
        ALIGN

/* The longer number must fit in dst_words words. */

Lsetup: cmpl    dst_words, %edx         /* ESI->n > dst_words? */
        ja      Lcarry                  /* yes -> return 1 */
        subl    %ecx, %edx

/* Now, EBX is the shorter one of src1 and src2, ECX = EBX->n.  ESI is
   the other one, EDX = ESI->n - EBX->n. */

        movl    dst, %edi
        movl    v(%ebx), %ebx           /* EBX := EBX->v */
        movl    v(%esi), %esi           /* ESI := ESI->v */
        movl    v(%edi), %edi           /* EDI := dst->v */
        test    %ecx, %ecx              /* Clears carry! */
        jz      Lend1                   /* ecx = 0 -> skip first loop */

/* This loop adds words common to both src1 and src2. */

        ALIGN
L1:     movl    (%ebx), %eax
        leal    4(%ebx), %ebx
        adcl    (%esi), %eax
        leal    4(%esi), %esi
        movl    %eax, (%edi)
        leal    4(%edi), %edi
        dec     %ecx
        jnz     L1

/* Process the remaining words of the longer one. */

        ALIGN
Lend1:  decl    %edx                    /* EDX = 0? (keep carry!) */
        js      Lend2                   /* yes -> skip second loop */
        incl    %edx
        ALIGN
L2:     movl    (%esi), %eax
        leal    4(%esi), %esi
        adc     $0, %eax
        movl    %eax, (%edi)
        leal    4(%edi), %edi
        decl    %edx
        jnz     L2
        ALIGN
Lend2:  jnc     Ldone

/* Carry from the most significant word.  Check dst_words. */

        movl    %edi, %edx
        movl    dst, %ebx
        movl    v(%ebx), %eax
        subl    %eax, %edx
        shrl    $2, %edx
        cmpl    dst_words, %edx
        jae     Lcarry
        movl    $1, (%edi)
        incl    %edx
        movl    %edx, n(%ebx)
        xorl    %eax, %eax
        jmp     Lreturn

        ALIGN
Lcarry: movl    $1, %eax
        jmp     Lreturn

/* No carry from most significant word.  Set dst->n. */

        ALIGN
Ldone:  movl    dst, %ebx
        movl    v(%ebx), %eax
        subl    %eax, %edi
        shrl    $2, %edi
        movl    %edi, n(%ebx)
        xorl    %eax, %eax
        ALIGN
Lreturn:
        popl    %edi
        popl    %esi
        popl    %ebx
        leave
        EPILOGUE(_bi_add_bb)
