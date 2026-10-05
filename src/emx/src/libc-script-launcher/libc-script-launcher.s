/** @file
 *
 * Resolve __libcn_script_launcher in the LIBC DLL used by crt0 and call it.
 *
 * Copyright (c) 2026 Dmitrii Kuminov <coding@dmik.org>
 *
 *
 * This file is part of LIBC Next.
 *
 * LIBC Next is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * LIBC Next is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with LIBC Next; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/*
 * NOTE: There is a trivial approach to declare ___libcn_script_launcher as a
 * _main alias like this:
 *
 *  .stabs "_main",11,0,0,0
 *  .stabs "___libcn_script_launcher",1,0,0,0
 *
 * which produces the smallest possible EXE (~700 bytes) but it creates a
 * static DLL import dependency, and for an older DLL which does not export
 * __libcn_script_launcher, OS/2 will silently terminate the process with
 * TC_HARDERROR and no message other than an entry in POPUPLOG.OS2.
 *
 * Dynamic resolution works around that by printing an error message instead
 * (and exiting with status code 128 to match what __libcn_script_launcher does
 * on pre-condition failures), increasing the EXE size to only ~1000 bytes (~800
 * bytes compressed), but it's much more user-friendly.
 */

    .text
    .globl  _main

_main:
    pushl   %ebp
    movl    %esp, %ebp
    subl    $268, %esp              /* module, procedure and 260-byte DLL name */
    movb    $0, (%esp)              /* empty name if the module query fails */

    /* _CRT_init is imported by crt0, so its address identifies the DLL. */
    leal    -4(%ebp), %eax
    movl    %esp, %ecx              /* module name buffer */
    pushl   $__CRT_init             /* address within the loaded LIBCn DLL */
    pushl   $0                      /* object offset */
    pushl   %ecx                    /* module name buffer */
    pushl   $260                    /* module name buffer size */
    pushl   $0                      /* object number */
    pushl   %eax                    /* module handle */
    call    DosQueryModFromEIP
    addl    $24, %esp
    movb    $0, -9(%ebp)            /* bound the module-name scan */
    testl   %eax, %eax
    jnz     .L_error

    call    .L_query_launcher
    testl   %eax, %eax
    jz      .L_launch
    cmpl    $6, %eax                /* ERROR_INVALID_HANDLE */
    jne     .L_error

    /* The DLL is mapped but cannot be queried until explicitly loaded. */
    leal    -4(%ebp), %eax
    movl    %esp, %ecx              /* module name buffer */
    pushl   %eax                    /* module handle */
    pushl   %ecx                    /* exact DLL name returned above */
    pushl   $0                      /* error buffer size */
    pushl   $0                      /* error buffer */
    call    DosLoadModule
    addl    $16, %esp
    testl   %eax, %eax
    jnz     .L_error

    call    .L_query_launcher
    testl   %eax, %eax
    jnz     .L_error

.L_launch:
    pushl   12(%ebp)                /* argv */
    pushl   8(%ebp)                 /* argc */
    call    *-8(%ebp)
    leave
    ret

.L_query_launcher:
    leal    -8(%ebp), %eax
    pushl   %eax                    /* procedure address */
    pushl   $0                      /* query by ordinal */
    pushl   $2035                   /* ___libcn_script_launcher */
    pushl   -4(%ebp)                /* module handle */
    call    DosQueryProcAddr
    addl    $16, %esp
    ret

.L_error:
    call    __getprogname
    movl    %esp, %ecx              /* module name buffer */
    pushl   %ecx
    pushl   %eax                    /* program name */
    pushl   $.L_message
    pushl   ___stderrp
    call    __std_fprintf
    addl    $16, %esp
    pushl   $128
    popl    %eax
    leave
    ret

    .data
.L_message:
    .asciz  "%s: Unsupported %s.DLL version (0.1.15+ required)\r\n"
