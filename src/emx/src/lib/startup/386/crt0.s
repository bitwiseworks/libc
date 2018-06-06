/* $Id: crt0.s 3900 2014-10-22 20:26:07Z bird $ */
/** @file
 *
 * Application entry point.
 *
 * This is the routine that gets control first. It should prepare
 * argc/argv/envp and call the main function.
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

__text:

    /*
     * Fix the FPU control word since it may have been perverted by
     * statically imported DLLs.
     */
    fldcw   __crt0_fpucw

#if !defined(NOFORK) && !defined(NOUNIX)
    /* Registering the fork module. If we're forking this call will not return. */
    pushl   $1
    pushl   $ForkModule
    call    ___libc_ForkRegisterModule
    addl    $8, %esp
#endif
    pushl   $(FLAG_HIGHMEM + FLAG_NOUNIX)
    call    ___init_app
    /* esp points to main() call frame. */
    cld
#ifdef ARGS_RESP
    lea     4(%esp), %eax               /* &argv */
    movl    %esp, %ecx                  /* &argc */
    pushl   %eax
    pushl   %ecx
    call    __response
    addl    $8, %esp
#endif
#ifdef ARGS_WILD
    lea     4(%esp), %eax               /* &argv */
    movl    %esp, %ecx                  /* &argc */
    pushl   %eax
    pushl   %ecx
    call    __wildcard
    addl    $8, %esp
#endif
    call    __CRT_init
    call    _main
    addl    $3*4, %esp
do_exit:
    pushl   %eax
    call    _exit
1:  jmp     1b      /* Just in case exit() returns :-) */

/** Initial FPU CW value. */
    .align  2, 0xcc
__crt0_fpucw:
    .long   0x37f

    .data
___data_start:
__data:
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
    .stabs "___fork_parent1__", 21, 0, 0, 0xffffffff
    .stabs "___fork_child1__", 21, 0, 0, 0xffffffff

ForkModule:
    .long  0x00010000                   // uVersion (__LIBC_FORK_MODULE_VERSION)
    .long  __atfork_callback            // pfnAtFork
    .long  ___fork_parent1__            // papParent1
    .long  ___fork_child1__             // papChild1
    .long  ___data_start                // pvDataSegBase
    .long  _end                         // pvDataSegEnd
    .long  0x00000001                   // fFlags - __LIBC_FORKMODULE_FLAGS_EXECUTABLE
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
    .stabs "__end",1,0,0,0              /* Force libend inclusion for -Zomf. */

    .bss
___bss_start:

