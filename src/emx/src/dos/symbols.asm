;
; SYMBOLS.ASM -- Manage symbol tables
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

__SYMBOLS       =       1

                INCLUDE EMX.INC
                INCLUDE SYMBOLS.INC
                INCLUDE SIGNAL.INC
                INCLUDE PROCESS.INC

                PUBLIC  SYM_BY_NAME, SYM_BY_ADDR, SYM_MODULE, SYM_BEFORE

SV_CODE         SEGMENT

                ASSUME  CS:SV_CODE, DS:NOTHING

;
; In:   DS:DI   Pointer to process table entry
;       DS:BX   Name of symbol (0 terminated)
;       LDTR    Set for process
;
; Out:  EDX     Address
;       AL      Code/data
;       CY      Not found
;
; Note: Assumes that symbols in the string table are unique
; 
                ASSUME  DI:PTR PROCESS
SYM_BY_NAME     PROC    NEAR
                PUSH    ES
                PUSH    ECX
                PUSH    ESI
                PUSH    EBP
                MOV     DX, BX
                CMP     [DI].P_SYMBOLS, 0
                JE      SBN_LOOSE
                MOV     AX, L_SYM_SEL
                .386P
                VERR    AX
                .386
                JNZ     SBN_LOOSE
                MOV     ES, AX
                MOV     ESI, [DI].P_STR_OFF
                MOV     ECX, ES:[ESI]
                ADD     ECX, ESI
                ADD     ESI, 4
SBN1:           MOV     BX, DX
                CMP     ESI, ECX
                JAE     SHORT SBN_LOOSE
                MOV     EBP, ESI
SBN2:           MOV     AL, ES:[ESI]
                CMP     AL, [BX]
                JNE     SHORT SBN3
                INC     BX
                INC     ESI
                OR      AL, AL
                JNZ     SHORT SBN2
                JMP     SHORT SBN4

SBN3:           INC     ESI
                CMP     BYTE PTR ES:[ESI-1], 0
                JNE     SHORT SBN3
                JMP     SHORT SBN1

SBN4:           SUB     EBP, [DI].P_STR_OFF
                MOV     ESI, 0
                ASSUME  ESI:NEAR32 PTR NLIST
                MOV     ECX, [DI].P_SYMBOLS
SBN5:           CMP     ES:[ESI].N_STRING, EBP
                JE      SHORT SBN7
SBN6:           ADD     ESI, SIZE NLIST
                LOOPD   SBN5
                JMP     SHORT SBN_LOOSE

SBN7:           MOV     AL, ES:[ESI].N_TYPE
                AND     AL, NOT N_EXT
                MOV     AH, SYM_TEXT
                CMP     AL, N_TEXT
                JE      SHORT SBN8
                MOV     AH, SYM_DATA
                CMP     AL, N_DATA
                JE      SHORT SBN8
                CMP     AL, N_BSS
                JE      SHORT SBN8
                JMP     SHORT SBN6

SBN8:           MOV     AL, AH
                MOV     BX, DX
                MOV     EDX, ES:[ESI].N_VALUE
                CLC
                JMP     SHORT SBN_RET

SBN_LOOSE:      STC
                MOV     BX, DX
SBN_RET:        POP     EBP
                POP     ESI
                POP     ECX
                POP     ES
                RET
                ASSUME  ESI:NOTHING
                ASSUME  DI:NOTHING
SYM_BY_NAME     ENDP


;
; In:   DS:DI   Pointer to process table entry
;       EDX     Address
;       AL      Code/data
;       LDTR    Set for process
;
; Out:  ES:EBX  Pointer to name
;       DX      Descriptor (line number)
;       CY      Not found
;
; This function searches the list backwards, to meet a `real' symbol
; before a file name (eg, __time before time.o). Yes, it's a hack.
;
                ASSUME  DI:PTR PROCESS
SYM_BY_ADDR     PROC    NEAR
                PUSH    ECX
                PUSH    ESI
                PUSH    EBP
                MOV     BL, AL
                CMP     [DI].P_SYMBOLS, 0
                JE      SBA_LOOSE
                MOV     AX, L_SYM_SEL
                .386P
                VERR    AX
                .386
                JNZ     SBA_LOOSE
                MOV     ES, AX
                MOV     ECX, [DI].P_SYMBOLS
                MOV     ESI, ECX
                DEC     ESI
                IMUL    ESI, SIZE NLIST
                ASSUME  ESI:NEAR32 PTR NLIST
SBA1:           CMP     ES:[ESI].N_VALUE, EDX
                JE      SHORT SBA3
SBA2:           SUB     ESI, SIZE NLIST
                LOOPD   SBA1
                JMP     SHORT SBA_LOOSE

SBA3:           MOV     AL, ES:[ESI].N_TYPE
                AND     AL, NOT N_EXT
                MOV     AH, SYM_TEXT
                CMP     AL, N_TEXT
                JE      SHORT SBA4
                MOV     AH, SYM_DATA
                CMP     AL, N_DATA
                JE      SHORT SBA4
                CMP     AL, N_BSS
                JE      SHORT SBA4
                CMP     AL, N_SLINE
                JNE     SHORT SBA2
                CMP     BL, SYM_LINE
                JNE     SHORT SBA2
                JMP     SHORT SBA5

SBA4:           CMP     BL, SYM_ANY
                JE      SHORT SBA5
                CMP     BL, AH
                JNE     SHORT SBA2
SBA5:           MOV     EBX, ES:[ESI].N_STRING
                ADD     EBX, [DI].P_STR_OFF
                MOV     DX, ES:[ESI].N_DESC
                CLC
                JMP     SHORT SBA_RET

SBA_LOOSE:      STC
SBA_RET:        POP     EBP
                POP     ESI
                POP     ECX
                RET
                ASSUME  ESI:NOTHING
                ASSUME  DI:NOTHING
SYM_BY_ADDR     ENDP


;
; In:   DS:DI   Pointer to process table entry
;       EDX     Address
;       LDTR    Set for process
;
; Out:  ES:EBX  Pointer to file name
;       CY      Not found
;
; This functions assumes that the list is sorted by address.
;
                ASSUME  DI:PTR PROCESS
SYM_MODULE      PROC    NEAR
                PUSH    ECX
                PUSH    ESI
                PUSH    EBP
                CMP     [DI].P_SYMBOLS, 0
                JE      SBM_LOOSE
                MOV     AX, L_SYM_SEL
                .386P
                VERR    AX
                .386
                JNZ     SBM_LOOSE
                MOV     ES, AX
                MOV     ECX, [DI].P_SYMBOLS
                MOV     ESI, 0
                ASSUME  ESI:NEAR32 PTR NLIST
                MOV     EBP, -1                 ; Nothing found yet
SBM1:           MOV     AL, ES:[ESI].N_TYPE
                CMP     AL, 64H                 ; Main module
                JE      SHORT SBM4
                CMP     AL, 84H                 ; Submodule
                JE      SHORT SBM4
SBM2:           ADD     ESI, SIZE NLIST
                LOOPD   SBM1
SBM3:           CMP     EBP, -1
                JE      SHORT SBM_LOOSE
                MOV     ESI, EBP
                MOV     EBX, ES:[ESI].N_STRING
                ADD     EBX, [DI].P_STR_OFF
                CLC
                JMP     SHORT SBM_RET

SBM4:           CMP     ES:[ESI].N_VALUE, EDX
                JA      SHORT SBM3
                MOV     EBP, ESI
                JMP     SHORT SBM2

SBM_LOOSE:      STC
SBM_RET:        POP     EBP
                POP     ESI
                POP     ECX
                RET
                ASSUME  ESI:NOTHING
                ASSUME  DI:NOTHING
SYM_MODULE      ENDP


;
; In:   DS:DI   Pointer to process table entry
;       EDX     Address
;       LDTR    Set for process
;
; Out:  ES:EBX  Pointer to name
;       EAX     Address
;       CY      Not found
;
; This functions assumes that file names come after other symbols.
;
                ASSUME  DI:PTR PROCESS
SYM_BEFORE      PROC    NEAR
                PUSH    ECX
                PUSH    ESI
                PUSH    EBP
                CMP     [DI].P_SYMBOLS, 0
                JE      SBB_LOOSE
                MOV     AX, L_SYM_SEL
                .386P
                VERR    AX
                .386
                JNZ     SBB_LOOSE
                MOV     ES, AX
                MOV     EBP, -1                 ; Nothing found yet
                MOV     ECX, [DI].P_SYMBOLS
                MOV     ESI, 0
                ASSUME  ESI:NEAR32 PTR NLIST
                MOV     EBP, -1
                MOV     EBX, 0
SBB1:           MOV     AL, ES:[ESI].N_TYPE
                AND     AL, NOT N_EXT
                CMP     AL, N_TEXT
                JE      SHORT SBB3
                CMP     AL, N_DATA
                JE      SHORT SBB3
                CMP     AL, N_BSS
                JE      SHORT SBB3
SBB2:           ADD     ESI, SIZE NLIST
                LOOPD   SBB1
                CMP     EBP, -1
                JE      SHORT SBB_LOOSE
                MOV     ESI, EBP
                MOV     EBX, ES:[ESI].N_STRING
                ADD     EBX, [DI].P_STR_OFF
                MOV     EAX, ES:[ESI].N_VALUE
                CLC
                JMP     SHORT SBB_RET


SBB3:           CMP     ES:[ESI].N_VALUE, EDX
                JA      SHORT SBB2
                CMP     ES:[ESI].N_VALUE, EBX
                JB      SHORT SBB2
                MOV     EBP, ESI
                MOV     EBX, ES:[ESI].N_VALUE
                JMP     SHORT SBB2

SBB_LOOSE:      STC
SBB_RET:        POP     EBP
                POP     ESI
                POP     ECX
                RET
                ASSUME  ESI:NOTHING
                ASSUME  DI:NOTHING
SYM_BEFORE      ENDP




SV_CODE         ENDS

                END
