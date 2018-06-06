/* $Id: 572asm.s 601 2003-08-15 23:50:34Z bird $ */
/** @file
 *
 * _Optlink declaration and definition testcases.
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


.globl CVoid
CVoid:
   jmp verify

.globl CPVoid
CPVoid:
   jmp verify

.globl CInt
CInt:
   jmp verify

.globl CPType
CPType:
   jmp verify

.globl _Z4Voidiiii
_Z4Voidiiii:
   jmp verify

.globl _Z5PVoidiiii
_Z5PVoidiiii:
   jmp verify

.globl _Z3Intiiii
_Z3Intiiii:
   jmp verify

.globl _Z5PTypeiiii
_Z5PTypeiiii:
   jmp verify


.globl _ZN3foo16StaticMemberVoidEiiii
_ZN3foo16StaticMemberVoidEiiii:
   jmp verify

.globl _ZN3foo17StaticMemberPVoidEiiii
_ZN3foo17StaticMemberPVoidEiiii:
   jmp verify

.globl _ZN3foo15StaticMemberIntEiiii
_ZN3foo15StaticMemberIntEiiii:
   jmp verify

.globl _ZN3foo17StaticMemberPTypeEiiii
_ZN3foo17StaticMemberPTypeEiiii:
   jmp verify


.globl _ZN3foo10MemberVoidEiiii
_ZN3foo10MemberVoidEiiii:
   jmp verify2

.globl _ZN3foo11MemberPVoidEiiii
_ZN3foo11MemberPVoidEiiii:
   jmp verify2

.globl _ZN3foo9MemberIntEiiii
_ZN3foo9MemberIntEiiii:
   jmp verify2

.globl _ZN3foo11MemberPTypeEiiii
_ZN3foo11MemberPTypeEiiii:
   jmp verify2



verify:
   pushl %ebp
   movl  %esp, %ebp

   cmpl  $1, %eax
   je    ok1
   int   $3

ok1:
   cmpl  $2, %edx
   je    ok2
   int   $3


ok2:
   cmpl  $3, %ecx
   je    ok3
   int   $3

ok3:
   movl  20(%ebp), %eax
   cmpl  $4, %eax
   je    ok4
   int   $3

ok4:
   xor   %eax, %eax
   dec   %eax
   mov   %eax, %edx
   mov   %eax, %ecx

   mov   %ebp, %ebp
   popl  %ebp
   ret


verify2:
   pushl %ebp
   movl  %esp, %ebp

   test   %eax, %eax
   jnz   okthis
   int   $3

okthis:
   cmpl  $1, %edx
   je    ok21
   int   $3

ok21:
   cmpl  $2, %ecx
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
   xor   %eax, %eax
   dec   %eax
   mov   %eax, %edx
   mov   %eax, %ecx

   mov   %ebp, %ebp
   popl  %ebp
   ret

