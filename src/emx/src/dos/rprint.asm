;
; RPRINT.ASM -- Text output via DOS (real mode)
;
; Copyright (c) 1991-1995 by Eberhard Mattes
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

;
; Note: This module must not use 386 instructions
;

                INCLUDE EMX.INC

                PUBLIC  RWORD, RBYTE, RNIBBLE
                PUBLIC  RCHAR, RTEXT, RCRLF


INIT_CODE       SEGMENT

                ASSUME  CS:INIT_CODE, DS:NOTHING

                .8086                           ; !!!

RWORD           PROC    NEAR
                XCHG    AL, AH
                CALL    RBYTE
                XCHG    AL, AH
                CALL    RBYTE
                RET
RWORD           ENDP


RBYTE           PROC    NEAR
                PUSH    AX
                REPT    4
                SHR     AL, 1
                ENDM
                CALL    RNIBBLE
                POP     AX
                PUSH    AX
                CALL    RNIBBLE
                POP     AX
                RET
RBYTE           ENDP

RNIBBLE         PROC    NEAR
                AND     AL, 0FH
                ADD     AL, 30H
                CMP     AL, 3AH
                JB      RNIB1
                ADD     AL, 7
RNIB1:          CALL    RCHAR
                RET
RNIBBLE         ENDP


;
; In:   DS:DX   Text (0 terminated)
;
RTEXT           PROC    NEAR
                PUSH    AX
                PUSH    BX
                PUSH    CX
                XCHG    BX, DX
                MOV     CX, BX
                XOR     AL, AL
RTEXT1:         CMP     AL, DS:[BX]
                JE      RTEXT2
                INC     BX
                JMP     RTEXT1
RTEXT2:         XCHG    BX, CX
                SUB     CX, BX
                XCHG    BX, DX
                MOV     BX, 1                   ; stdout
                MOV     AH, 40H
                INT     21H
                POP     CX
                POP     BX
                POP     AX
                RET
RTEXT           ENDP

RCHAR           PROC    NEAR
                PUSH    AX
                PUSH    DX
                MOV     DL, AL
                MOV     AH, 02H
                INT     21H
                POP     DX
                POP     AX
                RET
RCHAR           ENDP


RCRLF           PROC    NEAR
                PUSH    AX
                MOV     AL, CR
                CALL    RCHAR
                MOV     AL, LF
                CALL    RCHAR
                POP     AX
                RET
RCRLF           ENDP

INIT_CODE       ENDS

                END
