;
; OPRINT.ASM -- Text output via DOS
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

                INCLUDE EMX.INC
                INCLUDE SIGNAL.INC
                INCLUDE PROCESS.INC

                PUBLIC  ODWORD, OWORD, OBYTE, ONIBBLE
                PUBLIC  OCHAR, OTEXT, OCRLF


SV_DATA         SEGMENT

O_TMP_CHAR      DB      ?

$CRLF           DB      CR, LF, 0

SV_DATA         ENDS

SV_CODE         SEGMENT

                ASSUME  CS:SV_CODE, DS:NOTHING

;
; Output using DOS
;

                ASSUME  DS:SV_DATA
ODWORD          PROC    NEAR
                ROR     EAX, 16
                CALL    OWORD
                ROR     EAX, 16
                CALL    OWORD
                RET
ODWORD          ENDP

                ASSUME  DS:SV_DATA
OWORD           PROC    NEAR
                XCHG    AL, AH
                CALL    OBYTE
                XCHG    AL, AH
                CALL    OBYTE
                RET
OWORD           ENDP

                ASSUME  DS:SV_DATA
OBYTE           PROC    NEAR
                PUSH    AX
                SHR     AL, 4
                CALL    ONIBBLE
                POP     AX
                PUSH    AX
                CALL    ONIBBLE
                POP     AX
                RET
OBYTE           ENDP

                ASSUME  DS:SV_DATA
ONIBBLE         PROC    NEAR
                AND     AL, 0FH
                ADD     AL, 30H
                CMP     AL, 3AH
                JB      SHORT ONIB1
                ADD     AL, 7
ONIB1:          CALL    OCHAR
                RET
ONIBBLE         ENDP


;
; In:   EDX     Pointer to null-terminated string
;
                ASSUME  DS:SV_DATA
OTEXT           PROC    NEAR
                PUSH    EAX
                PUSH    EBX
                PUSH    ECX
                MOV     ECX, EDX
                XOR     AL, AL
OTEXT1:         CMP     AL, DS:[EDX]
                JE      SHORT OTEXT2
                INC     EDX
                JMP     SHORT OTEXT1
OTEXT2:         XCHG    EDX, ECX
                SUB     ECX, EDX
                MOV     BX, PROC0.P_HANDLES[1*2]
                MOV     AH, 40H
                PUSH    PROCESS_PTR
                MOV     PROCESS_PTR, NO_PROCESS
                INT     21H
                POP     PROCESS_PTR
                POP     ECX
                POP     EBX
                POP     EAX
                RET
OTEXT           ENDP

                ASSUME  DS:SV_DATA
OCHAR           PROC    NEAR
                PUSH    EAX
                PUSH    EBX
                PUSH    ECX
                PUSH    EDX
                MOV     O_TMP_CHAR, AL
                LEA     EDX, O_TMP_CHAR
                MOV     ECX, 1
                MOV     BX, PROC0.P_HANDLES[1*2]
                MOV     AH, 40H
                PUSH    PROCESS_PTR
                MOV     PROCESS_PTR, NO_PROCESS
                INT     21H
                POP     PROCESS_PTR
                POP     EDX
                POP     ECX
                POP     EBX
                POP     EAX
                RET
OCHAR           ENDP


                ASSUME  DS:SV_DATA
OCRLF           PROC    NEAR
                PUSH    EDX
                LEA     EDX, $CRLF
                CALL    OTEXT
                POP     EDX
                RET
OCRLF           ENDP

SV_CODE         ENDS

                END
