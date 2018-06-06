; $Id: signal16bit.asm 3004 2007-04-07 00:41:40Z bird $
;; @file
;
; LIBC SYS Backend - Signals, 16-bit handler.
;
; Copyright (c) 2004 knut st. osmundsen <bird@innotek.de>
;
;
; This file is part of InnoTek LIBC.
;
; InnoTek LIBC is free software; you can redistribute it and/or modify
; it under the terms of the GNU Lesser General Public License as published
; by the Free Software Foundation; either version 2 of the License, or
; (at your option) any later version.
;
; InnoTek LIBC is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; GNU Lesser General Public License for more details.
;
; You should have received a copy of the GNU Lesser General Public License
; along with InnoTek LIBC; if not, write to the Free Software
; Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
;
;

    .386

extrn Dos32TIB:abs
extrn ___libc_back_signalOS2V1Handler32bit:near


CODE16 segment use16 para public 'CODE'

;;
;
; It's a signal handler with this 16-bit PASCAL declaration:
;    void FAR PASCAL __libc_back_signalOS2V1Handler16bit(USHORT sig_arg, USHORT sig_num);
;
public ___libc_back_signalOS2V1Handler16bit
___libc_back_signalOS2V1Handler16bit:
    ; save registers.
    push    eax
    push    ecx
    push    edx
    push    ds
    push    es
    push    fs
    push    gs

    ; save old stack
    xor     eax, eax
    mov     ax, ss
    mov     edx, esp
    push    eax                         ; dword old ss
    push    edx                         ; dword old sp.

    jmp far ptr FLAT:CODE32:thunked_32
CODE16 ends
CODE32 segment use32 para public 'CODE'
thunked_32:
    ; thunk the stack.
    mov     edx, ss
    and     edx, 0fff8h
    shl     edx, 13
    mov     dx, sp
    mov     eax, seg DGROUP
    mov     ss, eax
    mov     esp, edx

    ; load selectors.
    mov     ds, eax
    mov     es, eax
    mov     edx, Dos32TIB
    mov     fs, edx

    ; call 32-bit worker
    movzx   eax, word ptr [esp + (4 * 5) + (2 * 4) + 6] ; sig_arg
    movzx   edx, word ptr [esp + (4 * 5) + (2 * 4) + 4] ; sig_num
    push    eax
    push    edx
    call    ___libc_back_signalOS2V1Handler32bit
    add     esp, 8h

    ; restore the stack and jump to 16-bit.
    lss     esp, [esp]
    jmp far ptr CODE16:thunked_16
CODE32 ends
CODE16 segment
thunked_16:

    ; restore registers.
    pop     gs
    pop     fs
    pop     es
    pop     ds
    pop     edx
    pop     ecx
    pop     eax

    retf    4
CODE16 ends

DATA32 segment use32 para public 'DATA'
DATA32 ends

BSS32 segment use32 para public 'BSS'
BSS32 ends

DGROUP GROUP DATA32, BSS32

end
