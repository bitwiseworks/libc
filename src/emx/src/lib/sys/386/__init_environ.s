/* $Id: __init_environ.s 810 2003-10-06 00:55:10Z bird $
 *
 * Assembly stuff used by __initdll.c
 *
 * Copyright (c) 2003 InnoTek Systemberatung GmbH
 * Author: knut st. osmundsen <bird-srcspam@anduin.net>
 *
 *
 * This file is part of Innotek LIBC.
 *
 * Innotek LIBC is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Innotek LIBC is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with Innotek LIBC; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <emx/asm386.h>

	.text

/**
 * Initialize __org_environ and _STD(environ).
 *
 * @cproto  int __init_dll_env(const char *pszEnv);
 * @sketch Assumes large amounts of stack so we can construct the a reverse array there.
 *
 */
    .globl __sys_init_environ
__sys_init_environ:
    pushl   %ebp
    movl    %esp, %ebp
    pushl   %edi
    pushl   %esi

    /*
     * Init registers.
     */
    movl    %esp, %esi
    xorl    %eax, %eax
    xorl    %ecx, %ecx
    decl    %ecx
    movl    8(%ebp), %edi
    cld

    /*
     * Scan loop.
     */
env_loop:
    push    %edi
    repnz   scasb
    cmpb    %al, (%edi)
    jnz     env_loop
    push    %eax

    /*
     * Calculate array size and allocate it.
     */
    movl    %esi, %ecx
    subl    %esp, %ecx
    / _org_environ = _hmalloc(size);
    pushl   %ecx
    call    __hmalloc
    addl    $4, %esp
    orl     %eax, %eax
    jz      env_ret
    mov     %eax, __org_environ

    /*
     * Do a reverse copy of the array on the stack.
     */
    movl    __org_environ, %edi
    movl    %edi, _STD(environ)
    movl    %esi, %edx
env_copy:
    subl    $4, %edx
    movl    (%edx), %eax
    stosl
    orl     %eax, %eax
    jnz     env_copy

env_ret:
    mov     %esi, %esp
    popl    %esi
    popl    %edi
    leave
    ret

