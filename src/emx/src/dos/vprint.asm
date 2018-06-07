;
; VPRINT.ASM -- Text output directly to video memory
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
                INCLUDE TABLES.INC
                INCLUDE SEGMENTS.INC

                PUBLIC  VWIDTH, VHEIGHT
                PUBLIC  VDWORD, VWORD, VBYTE, VNIBBLE
                PUBLIC  VTEXT, VCHAR, VCRLF, VCLS, VGETCOL, VWRAP
                PUBLIC  VINIT

SV_DATA         SEGMENT

VWIDTH          DW      80                      ; Number of character columns
VHEIGHT         DW      25                      ; Number of character rows
VPTR            DW      0                       ; Destination pointer

SV_DATA         ENDS


SV_CODE         SEGMENT

                ASSUME  CS:SV_CODE, DS:NOTHING


                ASSUME  DS:SV_DATA
VDWORD          PROC    NEAR
                ROR     EAX, 16
                CALL    VWORD
                ROR     EAX, 16
                CALL    VWORD
                RET
VDWORD          ENDP


                ASSUME  DS:SV_DATA
VWORD           PROC    NEAR
                XCHG    AL, AH
                CALL    VBYTE
                XCHG    AL, AH
                CALL    VBYTE
                RET
VWORD           ENDP


                ASSUME  DS:SV_DATA
VBYTE           PROC    NEAR
                PUSH    AX
                SHR     AL, 4
                CALL    VNIBBLE
                POP     AX
                PUSH    AX
                CALL    VNIBBLE
                POP     AX
                RET
VBYTE           ENDP


                ASSUME  DS:SV_DATA
VNIBBLE         PROC    NEAR
                AND     AL, 0FH
                ADD     AL, 30H
                CMP     AL, 3AH
                JB      SHORT VNIB1
                ADD     AL, 7
VNIB1:          CALL    VCHAR
                RET
VNIBBLE         ENDP


                ASSUME  DS:SV_DATA
VTEXT           PROC    NEAR
                PUSH    AX
VTEXT1:         MOV     AL, DS:[EDX]
                OR      AL, AL
                JZ      SHORT VTEXT9
                CALL    VCHAR
                INC     EDX
                JMP     SHORT VTEXT1
VTEXT9:         POP     AX
                RET
VTEXT           ENDP


                ASSUME  DS:SV_DATA
VCHAR           PROC    NEAR
                PUSH    BX
                MOV     BX, VPTR
                CMP     AL, CR
                JE      SHORT VCHAR_CR
                CMP     AL, LF
                JE      SHORT VCHAR_LF
                CMP     AL, TAB
                JE      SHORT VCHAR_TAB
                MOV     ES:[BX], AL
                ADD     BX, 2
VCHAR_RET:      MOV     VPTR, BX
                POP     BX
                RET

VCHAR_CR:       PUSH    AX
                PUSH    DX
                MOV     AX, BX
                XOR     DX, DX
                MOV     BX, VWIDTH
                SHL     BX, 1
                DIV     BX
                MUL     BX
                MOV     BX, AX
                POP     DX
                POP     AX
                JMP     VCHAR_RET

VCHAR_LF:       ADD     BX, VWIDTH
                ADD     BX, VWIDTH
                JMP     VCHAR_RET

VCHAR_TAB:      PUSH    AX
                PUSH    DX
                MOV     AX, BX
                SHR     AX, 1                   ; Compute character number
                XOR     DX, DX
                DIV     VWIDTH                  ; Compute column number (DX)
                MOV     AX, DX
                OR      AX, 8-1
                INC     AX                      ; Next tab position
                SUB     AX, DX                  ; Distance to next tab position
                SHL     AX, 1                   ; Number of bytes to skip
                ADD     BX, AX                  ; Update pointer
                POP     DX
                POP     AX
                JMP     VCHAR_RET
VCHAR           ENDP


                ASSUME  DS:SV_DATA
VCRLF           PROC    NEAR
                PUSH    AX
                MOV     AL, CR
                CALL    VCHAR
                MOV     AL, LF
                CALL    VCHAR
                POP     AX
                RET
VCRLF           ENDP


                ASSUME  DS:SV_DATA
VCLS            PROC    NEAR
                MOV     AX, VWIDTH
                MUL     VHEIGHT
                MOV     CX, AX
                MOV     AX, 0720H
                MOV     DI, 0
                CLD
                REP     STOSW
                MOV     VPTR, 0                 ; Set destination pointer
                RET
VCLS            ENDP

;
; Return the current column number
;
; Out:  AX      Column number (0 through VWIDTH - 1)
;
                ASSUME  DS:SV_DATA
VGETCOL         PROC    NEAR
                PUSH    DX
                MOV     AX, VPTR
                SHR     AX, 1                   ; Compute character number
                XOR     DX, DX
                DIV     VWIDTH                  ; Compute column number (DX)
                MOV     AX, DX
                POP     DX
                RET
VGETCOL         ENDP

;
; Move to the beginning of the next line if there are less than AX
; characters remaining on the line (don't print in the last column).
;
                ASSUME  DS:SV_DATA
VWRAP           PROC    NEAR
                PUSH    DX
                PUSH    AX
                CALL    VGETCOL
                POP     DX
                ADD     AX, DX
                CMP     AX, VWIDTH
                JB      SHORT VWRAP_1
                CALL    VCRLF
VWRAP_1:        POP     DX
                RET
VWRAP           ENDP

SV_CODE         ENDS


INIT_CODE       SEGMENT

                ASSUME  CS:INIT_CODE, DS:NOTHING

;
; Set VWIDTH and VHEIGHT
;
                ASSUME  DS:SV_DATA
VINIT           PROC    NEAR
;
; Get the width in character columns
;
                MOV     AH, 0FH                 ; Get current video mode
                INT     10H
                MOV     AL, AH                  ; Number of character columns
                XOR     AH, AH
                MOV     VWIDTH, AX
;
; Get the height in character rows
;
                MOV     AH, 11H                 ; Get font information
                MOV     AL, 30H                 ; (Check for EGA or better)
                MOV     BH, 0
                MOV     CX, 1234H
                PUSH    ES
                PUSH    BP
                INT     10H
                POP     BP
                POP     ES
                MOV     AX, 25
                CMP     CX, 1234H               ; Function implemented?
                JE      SHORT VINIT_1           ; No  -> old adapter (25 rows)
                PUSH    ES
                MOV     AX, 40H
                MOV     ES, AX
                MOVZX   AX, BYTE PTR ES:[84H]
                INC     AX
                POP     ES
VINIT_1:        MOV     VHEIGHT, AX
                MUL     VWIDTH
                MOVZX   ECX, AX
                SHL     ECX, 1
                LEA     DI, G_VIDEO_DESC
                CALL    RM_SEG_SIZE
                RET
VINIT           ENDP

INIT_CODE       ENDS

                END
