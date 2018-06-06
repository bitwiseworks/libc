/* $Id: 570asm.s 601 2003-08-15 23:50:34Z bird $ */
/** @file
 *
 * _System declaration and definition testcases.
 *
 * InnoTek Systemberatung GmbH confidential
 *
 * Copyright (c) 2003 InnoTek Systemberatung GmbH
 * Author: knut st. osmundsen <bird-srcspam@anduin.net>
 *
 * All Rights Reserved
 *
 */
   .file   "570asm.s"

   .text

.globl ExternCVoid
ExternCVoid:
   jmp verify

.globl ExternCPVoid
ExternCPVoid:
   jmp verify

.globl ExternCInt
ExternCInt:
   jmp verify

.globl ExternCPType
ExternCPType:
   jmp verify

.globl Void
Void:
   jmp verify

.globl PVoid
PVoid:
   jmp verify

.globl Int
Int:
   jmp verify

.globl PType
PType:
   jmp verify


.globl __ZN3foo16StaticMemberVoidEiiii
__ZN3foo16StaticMemberVoidEiiii:
   jmp verify

.globl __ZN3foo17StaticMemberPVoidEiiii
__ZN3foo17StaticMemberPVoidEiiii:
   jmp verify

.globl __ZN3foo15StaticMemberIntEiiii
__ZN3foo15StaticMemberIntEiiii:
   jmp verify

.globl __ZN3foo17StaticMemberPTypeEiiii
__ZN3foo17StaticMemberPTypeEiiii:
   jmp verify


.globl __ZN3foo10MemberVoidEiiii
__ZN3foo10MemberVoidEiiii:
   jmp verify2

.globl __ZN3foo11MemberPVoidEiiii
__ZN3foo11MemberPVoidEiiii:
   jmp verify2

.globl __ZN3foo9MemberIntEiiii
__ZN3foo9MemberIntEiiii:
   jmp verify2

.globl __ZN3foo11MemberPTypeEiiii
__ZN3foo11MemberPTypeEiiii:
   jmp verify2



verify:
   pushl %ebp
   movl  %esp, %ebp

   movl  8(%ebp), %eax
   cmpl  $1, %eax
   je    ok1
   int   $3

ok1:
   movl  12(%ebp), %eax
   cmpl  $2, %eax
   je    ok2
   int   $3


ok2:
   movl  16(%ebp), %eax
   cmpl  $3, %eax
   je    ok3
   int   $3

ok3:
   movl  20(%ebp), %eax
   cmpl  $4, %eax
   je    ok4
   int   $3

ok4:
   mov   %ebp, %ebp
   popl  %ebp
   ret


verify2:
   pushl %ebp
   movl  %esp, %ebp

   movl  4(%ebp), %eax
   test   %eax, %eax
   jnz   okthis
   int   $3

okthis:
   movl  12(%ebp), %eax
   cmpl  $1, %eax
   je    ok21
   int   $3

ok21:
   movl  16(%ebp), %eax
   cmpl  $2, %eax
   je    ok22
   int   $3


ok22:
   movl  20(%ebp), %eax
   cmpl  $3, %eax
   je    ok23
   int   $3

ok23:
   movl  24(%ebp), %eax
   cmpl  $4, %eax
   je    ok24
   int   $3

ok24:
   mov   %ebp, %ebp
   popl  %ebp
   ret

