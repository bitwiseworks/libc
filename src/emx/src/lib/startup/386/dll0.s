/* $Id: dll0.s 3354 2007-05-07 03:05:17Z bird $ */
/** @file
 *
 * Standard entry point for dynamic libraries.
 * This routine gets control right after dynamic library is loaded.
 *
 *
 * Copyright (c) 1992-1998 by Eberhard Mattes
 * Copyright (c) 2003 InnoTek Systemberatung GmbH
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

    .globl  __text
    .globl  __data
    .globl  ___data_start
    .globl  ___bss_start

    .text


#if defined(HIGHMEM)
#define FLAG_HIGHMEM    1
#else
#define FLAG_HIGHMEM    0
#endif

#if defined(NOUNIX)
#define FLAG_NOUNIX     2
#else
#define FLAG_NOUNIX     0
#endif

#if defined(NOFORK)
#define FLAG_NOFORK     4
#else
#define FLAG_NOFORK     0
#endif

    /*
     * The DLL Entry Point.
     */
__text:
    cmpl    $0, 8(%esp)
    je      do_init
    cmpl    $1, 8(%esp)
    je      do_term
    jmp     _DLL_InitTerm


    /*
     * DLL Initialization.
     */
do_init:
#if !defined(NOFORK) && !defined(NOUNIX)
    pushl   $0
    pushl   $ForkModule
    call    ___libc_ForkRegisterModule
    addl    $8, %esp
    cmpl    $0, %eax
    je      do_init_not_forking
    jg      do_return_success   /* we're forking; no init. */
    jmp     do_return_failure   /* eax < 0 - failure. */

    /* normal dll init. */
do_init_not_forking:
#endif
    /* call __init_dll() */
    pushl   4(%esp)
    pushl   $(FLAG_HIGHMEM + FLAG_NOUNIX + FLAG_NOFORK)
    call    ___init_dll
    add     $8, %esp
    orl     %eax, %eax
    jnz     do_return_failure

    /* call _DLL_InitTerm */
    cld
    pushl   8(%esp)
    pushl   8(%esp)
    call    _DLL_InitTerm
    add     $8, %esp
    orl     %eax, %eax
    jnz     do_return_success

    /* _DLL_InitTerm failed, undo the module registration. */
#if !defined(NOFORK) && !defined(NOUNIX)
    pushl   $ForkModule
    call    ___libc_ForkDeregisterModule
    addl    $4, %esp
#endif
    /* jmp     do_return_failure; - fall thru */

do_return_failure:
    xorl    %eax, %eax
    ret
do_return_success:
    xorl    %eax, %eax
    inc     %al
    ret


    /*
     * DLL Termination.
     */
do_term:
    /* do init term first */
    cld
    pushl   8(%esp)
    pushl   8(%esp)
    call    _DLL_InitTerm
    add     $8, %esp
    orl     %eax, %eax
    jnz     do_term_dll
    ret

do_term_dll:
    /* call __libc_Back_termDll() */
    pushl   4(%esp)
    pushl   $(FLAG_HIGHMEM + FLAG_NOUNIX + FLAG_NOFORK)
    call    ___libc_Back_termDll
    add     $8, %esp

    /* call __libc_ForkDeregisterModule */
#if !defined(NOFORK) && !defined(NOUNIX)
    pushl   $ForkModule
    call    ___libc_ForkDeregisterModule
    addl    $4, %esp
#endif
    jmp     do_return_success


    .data
__data:
___data_start:
    .long   0xba0bab    // Magic number (error detection)
    .long   __os2dll    // list of OS/2 DLL references

    .stabs  "__os2dll", 21, 0, 0, 0xffffffff
    .stabs  "___CTOR_LIST__", 21, 0, 0, 0xffffffff
    .stabs  "___DTOR_LIST__", 21, 0, 0, 0xffffffff
    .stabs  "___crtinit1__", 21, 0, 0, 0xffffffff
    .stabs  "___crtexit1__", 21, 0, 0, 0xffffffff
    .stabs  "___eh_frame__", 21, 0, 0, 0xffffffff
    .stabs  "___eh_init__", 21, 0, 0, 0xffffffff
    .stabs  "___eh_term__", 21, 0, 0, 0xffffffff
#if !defined(NOFORK) && !defined(NOUNIX)
    .stabs  "___fork_parent1__", 21, 0, 0, 0xffffffff
    .stabs  "___fork_child1__", 21, 0, 0, 0xffffffff

ForkModule:
    .long  0x00010000                   // uVersion (__LIBC_FORK_MODULE_VERSION)
    .long  __atfork_callback            // pfnAtFork
    .long  ___fork_parent1__            // papParent1
    .long  ___fork_child1__             // papChild1
    .long  ___data_start                // pvDataSegBase
    .long  _end                         // pvDataSegEnd
    .long  0                            // fFlags
    .long  0                            // pNext
    .long  0                            // uReserved[8]
    .long  0                            // uReserved[8]
    .long  0                            // uReserved[8]
    .long  0                            // uReserved[8]
    .long  0                            // uReserved[8]
    .long  0                            // uReserved[8]
    .long  0                            // uReserved[8]
    .long  0                            // uReserved[8]
#endif
    .stabs "__end",1,0,0,0              /* force libend inclusion for -Zomf. */

    .bss
___bss_start:

