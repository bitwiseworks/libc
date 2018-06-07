;
; EXTAPI.ASM -- DOS extender API
;
; Copyright (c) 1995 by Eberhard Mattes
;
; This file is part of emx.
;
; emx is free software; you can redistribute it and/or modify it
; under the terms of the GNU General Public License as published by
; the Free Software Foundation; either version 2, or (at your option)
; any later version.
;
; emx is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; GNU General Public License for more details.
;
; You should have received a copy of the GNU General Public License
; along with emx; see the file COPYING.  If not, write to
; the Free Software Foundation, 59 Temple Place - Suite 330,
; Boston, MA 02111-1307, USA.
;
; See emx.asm for a special exception.
;

                INCLUDE EMX.INC
                INCLUDE PMINT.INC

                PUBLIC  EXTAPI

SV_CODE         SEGMENT

                .386P

                ASSUME  CS:SV_CODE, DS:NOTHING

EXT_JUMP        WORD    EXT00, EXT01, EXT02, EXT03, EXT04
                WORD    EXT05, EXT06, EXT07, EXT08, EXT09
                WORD    EXT0A, EXT0B, EXT0C, EXT0D, EXT0E

                ASSUME  DS:SV_DATA
                ASSUME  BP:PTR ISTACKFRAME
EXTAPI          PROC    NEAR
                AND     I_EFLAGS, NOT FLAG_C    ; Clear carry flag (no error)
                MOV     AX, WORD PTR I_EAX
                CMP     AH, 0EH
                JA      SHORT EXT_INVALID
                MOV     BL, AH
                MOV     BH, 0
                SHL     BX, 1
                JMP     EXT_JUMP[BX]

EXT00::
EXT01::
EXT02::
EXT03::
EXT05::
EXT06::
EXT07::
EXT08::
EXT09::
EXT0A::
EXT0B::
EXT0C::
EXT0D::
EXT0E::
EXT_INVALID:    MOV     AX, 8001H               ; Unsupported function
EXT_ERROR:      AND     EAX, 0FFFFH
                MOV     I_EAX, EAX
                OR      I_EFLAGS, FLAG_C        ; Set carray flag
                RET

EXT04::         CMP     AL, 00H
                JE      SHORT EXT0400
                CMP     AL, 01H
                JE      SHORT EXT0401
                JMP     EXT_INVALID

;
; Get version
;
EXT0400:        MOV     I_EAX, 0100H            ; DPMI 1.0
                MOV     I_EBX, 0005H            ; TODO
                MOV     I_ECX, 0003H            ; TODO
                MOV     I_EDX, 0000H            ; TODO
                RET

;
; Get DPMI capabilities
;
EXT0401:        MOV     EDI, I_EDI
                MOV     ES, I_ES
                MOV     I_EAX, 0000H            ; TODO
                MOV     I_ECX, 0                ; Reserved
                MOV     I_EDX, 0                ; Reserved
                MOV     BYTE PTR ES:[EDI+0], 0  ; Major version (TODO)
                MOV     BYTE PTR ES:[EDI+1], 0  ; Minor version (TODO)
                MOV     BYTE PTR ES:[EDI+2], "e"
                MOV     BYTE PTR ES:[EDI+3], "m"
                MOV     BYTE PTR ES:[EDI+4], "x"
                MOV     BYTE PTR ES:[EDI+5], 0
                RET

EXTAPI          ENDP

SV_CODE         ENDS

                END
