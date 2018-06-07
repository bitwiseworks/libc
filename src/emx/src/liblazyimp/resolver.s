/* $Id: resolver.s 1545 2004-10-07 06:36:42Z bird $ */
/** @file
 *
 * Lazy Import - resolver.
 *
 * Copyright (c) 2004 knut st. osmundsen <bird@innotek.de>
 *
 *
 * This file is part of InnoTek LIBC.
 *
 * InnoTek LIBC is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * InnoTek LIBC is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with InnoTek LIBC; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <emx/asm386.h>

    .text
    .globl ___lazyimp_resolver
___lazyimp_resolver:
    push    %eax
    push    %edx
    push    %ecx
    call    LABEL(__lazyimp_resolver2)
    // - eax is resolved function address.
    // stack:
    //      0   ecx
    //      4   edx
    //      8   edx
    //     12   phmod
    //     16   mod_name
    //     20   ppfn
    //     24   symbol
    mov     %eax, 24(%esp)
    pop     %ecx
    pop     %edx
    pop     %eax
    lea     12(%esp), %esp
    ret

