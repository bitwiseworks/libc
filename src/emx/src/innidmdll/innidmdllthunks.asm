; $Id: innidmdllthunks.asm 717 2003-09-23 23:42:11Z bird $
;
; 16-bit thunkers.
;
; Copyright (c) 2003 InnoTek Systemberatung GmbH
; Author: knut st. osmundsen <bird-srcspam@anduin.net>
;
;
; This program is free software; you can redistribute it and/or modify
; it under the terms of the GNU General Public License as published by
; the Free Software Foundation; either version 2 of the License, or
; (at your option) any later version.
;
; This program is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; GNU General Public License for more details.
;
; You should have received a copy of the GNU General Public License
; along with This program; if not, write to the Free Software
; Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
;
;

    .386


extrn InitDemangleID32:near
extrn DemangleID32:near

CODE16 segment dword use16 public 'CODE'
CODE16 ends
CODE32 segment para use32 public 'CODE'
CODE32 ends
DATA32 segment para use32 public 'DATA'
DATA32 ends


CODE16 segment
    ASSUME ss:NOTHING, ds:NOTHING, es:NOTHING, cs:CODE16

;;
; Init stuff.
; @cproto   unsigned short pascal far INITDEMANGLEID(char far * psInitParms);
INITDEMANGLEID proc far
    push    ebp
    movsx   ebp, sp

    jmp far ptr FLAT:INITDEMANGLEID_32bit
    ASSUME ss:nothing
CODE16 ends
CODE32 segment
INITDEMANGLEID_32bit::
    ;
    ; Thunk stack to 32bits!
    ;
    ; prepare switch back.
    mov     edx, esp
    push    ss
    push    edx
    ; the switch.
    xor     eax, eax
    mov     eax, ss
    shl     eax, 13
    mov     ax, sp
    mov     edx, seg FLAT:DATA32
    push    edx
    push    eax
    lss     esp, ss:[esp]
    ASSUME ss:FLAT

    ; DS / ES
    push    ds
    push    es
    mov     ds, edx
    mov     es, edx

    ; ebp high
    mov     edx, esp
    mov     dx, bp
    mov     ebp, edx

    ;
    ; Thunk parameters and call the 32bit version.
    ;
    mov     eax, [ebp + 08h]            ; psInitParms
    mov     edx, eax
    shr     eax, 3
    mov     ax, dx
    push    eax
    call    InitDemangleID32
    add     esp, 04h

    ; ES / DS
    pop     es
    pop     ds

    ;
    ; Switch the stack back to 16 bit.
    ;
    movzx   ebp, bp
    lss     esp, ss:[esp]
    ASSUME ss:NOTHING

    jmp     far ptr CODE16:INITDEMANGLEID_16bit
CODE32 ends
CODE16 segment
INITDEMANGLEID_16bit::

    mov     sp, bp
    pop     ebp
    ret 04h
INITDEMANGLEID endp

;;
; Demangle symbol.
;unsigned short pascal far DEMANGLEID(char far * psMangledName, char far * pszPrototype, unsigned long BufferLen);
DEMANGLEID proc far
    push    ebp
    movsx   ebp, sp

    jmp far ptr FLAT:DEMANGLEID_32bit
    ASSUME ss:nothing
CODE16 ends
CODE32 segment
DEMANGLEID_32bit::
    ;
    ; Thunk stack to 32bits!
    ;
    ; prepare switch back.
    mov     edx, esp
    push    ss
    push    edx
    ; the switch.
    xor     eax, eax
    mov     eax, ss
    shl     eax, 13
    mov     ax, sp
    mov     edx, seg FLAT:DATA32
    push    edx
    push    eax
    lss     esp, ss:[esp]
    ASSUME ss:FLAT

    ; DS / ES
    push    ds
    push    es
    mov     ds, edx
    mov     es, edx

    ; ebp high
    mov     edx, esp
    mov     dx, bp
    mov     ebp, edx


    ;
    ; Thunk parameters and call the 32bit version.
    ;
    mov     eax, [ebp + 8]              ; BufferLen
    push    eax

    mov     eax, [ebp + 0ch]            ; pszPrototype
    mov     edx, eax
    shr     eax, 3
    mov     ax, dx
    push    eax

    mov     eax, [ebp + 10h]            ; psMangledName
    mov     edx, eax
    shr     eax, 3
    mov     ax, dx
    push    eax
    call    DemangleID32
    add     esp, 0ch

    ; ES / DS
    pop     es
    pop     ds

    ;
    ; Switch the stack back to 16 bit.
    ;
    movzx   ebp, bp
    lss     esp, ss:[esp]
    ASSUME ss:NOTHING

    jmp     far ptr CODE16:DEMANGLEID_16bit
CODE32 ends
CODE16 segment
DEMANGLEID_16bit::

    mov     sp, bp
    pop     ebp
    ret 0ch
DEMANGLEID endp

CODE16 ends

end
