;
; A20.ASM -- Control the A20 gate
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
                INCLUDE XMS.INC
                INCLUDE VCPI.INC
                INCLUDE OPTIONS.INC

                PUBLIC  USE_A20_PATCH, USE_FAST_A20
                PUBLIC  INIT_A20, CLEANUP_A20, CHECK_A20
                PUBLIC  A20_ON, A20_OFF

SV_DATA         SEGMENT

;
; Enable/disable A20: address of functions
;
A20_ON_JMP      DW      ?
A20_OFF_JMP     DW      ?

A20_FLAG        DB      FALSE
USE_A20_PATCH   DB      FALSE
USE_FAST_A20    DB      FALSE

SV_DATA         ENDS


INIT_CODE       SEGMENT

                ASSUME  CS:INIT_CODE, DS:NOTHING

;
; Find out which machine we're running on
; and set A20 enable/disable function addresses
;
                ASSUME  DS:SV_DATA
INIT_A20        PROC    NEAR
                PUSH    ES                      ; Don't clobber ES (ARGS!)
                LEA     BX, PATCH_A20_ON
                LEA     DX, PATCH_A20_OFF
                CMP     USE_A20_PATCH, FALSE
                JNE     SHORT INIT_A20_SET
                LEA     BX, FAST_A20_ON
                LEA     DX, FAST_A20_OFF
                CMP     USE_FAST_A20, FALSE
                JNE     SHORT INIT_A20_SET
                LEA     BX, XMS_A20_ON
                LEA     DX, XMS_A20_OFF
                CMP     XMS_FLAG, FALSE
                JNE     SHORT INIT_A20_SET
                LEA     BX, FMR70_A20_ON
                LEA     DX, FMR70_A20_OFF
                CMP     MACHINE, MACH_FMR70
                JE      SHORT INIT_A20_SET
                LEA     BX, PC98_A20_ON
                LEA     DX, PC98_A20_OFF
                CMP     MACHINE, MACH_PC98
                JE      SHORT INIT_A20_SET
                LEA     BX, INBOARD_A20_ON
                LEA     DX, INBOARD_A20_OFF
                CMP     MACHINE, MACH_INBOARD
                JE      SHORT INIT_A20_SET
                MOV     AH, 0C0H                ; Return config. parameters
                STC                             ; Don't trust BIOS
                INT     15H                     ; `Cassette I/O'
                JC      SHORT INIT_A20_1        ; Cannot happen, assume AT
                MOV     AL, ES:[BX+5]           ; Get flags byte
                TEST    AL, 00000010B           ; Micro Channel?
                JZ      SHORT INIT_A20_1        ; No -> AT compatible
;
; It's a PS/2
;
                CALL    PS2_A20_OFF             ; Turn off A20
                LEA     BX, PS2_A20_ON          ; Why? HIMEM.SYS does
                LEA     DX, PS2_A20_OFF
                JMP     SHORT INIT_A20_SET

;
; It's an AT compatible machine
;
INIT_A20_1:     LEA     BX, AT_A20_ON
                LEA     DX, AT_A20_OFF

INIT_A20_SET:   CMP     VCPI_FLAG, FALSE        ; VCPI ?
                JE      SHORT INIT_A20_SET_1    ; No  -> skip
                LEA     BX, NO_A20_ON           ; Don't enable A20
                LEA     DX, NO_A20_OFF          ; Don't disable A20
INIT_A20_SET_1: MOV     A20_ON_JMP, BX
                MOV     A20_OFF_JMP, DX
                POP     ES
                RET
INIT_A20        ENDP


DATA_8042       =       60H                     ; Keyboard processor
STATUS_8042     =       64H                     ; Keyboard processor

;
; Enable address line A20
;
; Note: disables interrupts
;
                ASSUME  DS:SV_DATA
A20_ON          PROC    NEAR
                PUSH    AX
                CLI
                CALL    A20_ON_JMP
                MOV     A20_FLAG, NOT FALSE
                POP     AX
                RET
A20_ON          ENDP

;
; Disable address line A20
;
; Note: disables interrupts
;
                ASSUME  DS:SV_DATA
A20_OFF         PROC    NEAR
                PUSH    AX
                CLI
                CALL    A20_OFF_JMP
                MOV     A20_FLAG, FALSE
                POP     AX
                RET
A20_OFF         ENDP

;
; Don't enable address line A20
;
NO_A20_ON       PROC    NEAR
                RET
NO_A20_ON       ENDP

;
; Don't disable address line A20
;
NO_A20_OFF      PROC    NEAR
                RET
NO_A20_OFF      ENDP

;
; Enable address line A20 (AT compatible)
;
AT_A20_ON       PROC    NEAR
                MOV     AH, 0DFH
                CALL    AT_SET_A20
                RET
AT_A20_ON       ENDP

;
; Disable address line A20 (AT compatible)
;
AT_A20_OFF      PROC    NEAR
                MOV     AH, 0DDH
                CALL    AT_SET_A20
                RET
AT_A20_OFF      ENDP


;
; Enable address line A20 (PS/2)
;
PS2_A20_ON      PROC    NEAR
                IN      AL, 92H
                OR      AL, 02H
                OUT     92H, AL
                RET
PS2_A20_ON      ENDP


;
; Disable address line A20 (PS/2)
;
PS2_A20_OFF     PROC    NEAR
                IN      AL, 92H
                AND     AL, NOT 02H
                OUT     92H, AL
                RET
PS2_A20_OFF     ENDP


;
; Enable address line A20 (Inboard 386/PC)
;
INBOARD_A20_ON  PROC    NEAR
                MOV     AL, 0DFH
                OUT     60H, AL
                RET
INBOARD_A20_ON  ENDP


;
; Disable address line A20 (Inboard 386/PC)
;
INBOARD_A20_OFF PROC    NEAR
                MOV     AL, 0DDH
                OUT     60H, AL
                RET
INBOARD_A20_OFF ENDP


;
; Enable/disable address line A20 (AT compatible)
;
; In:   AL=0DDH Disable
;       AL=0DFH Enable
;
AT_SET_A20      PROC    NEAR
                PUSH    CX
                CALL    READY_8042
                JNZ     SHORT AT_SET_A20_END
                MOV     AL, 0D1H                ; Write output port
                OUT     STATUS_8042, AL
                CALL    READY_8042
                JNZ     SHORT AT_SET_A20_END
                MOV     AL, AH                  ; Data
                OUT     DATA_8042, AL
                CALL    READY_8042
                MOV     AL, 0FFH                ; Pulse nothing (just wait)
                OUT     STATUS_8042, AL
                CALL    READY_8042
AT_SET_A20_END: POP     CX
                RET
AT_SET_A20      ENDP


;
; Wait until we can send a command to the keyboard controller
;
READY_8042      PROC    NEAR
                XOR     CX, CX
                TALIGN  4                       ; Avoid surprises
READY_1:        IN      AL, STATUS_8042
                TEST    AL, 02H                 ; Input buffer full?
                LOOPNZ  READY_1                 ; Yes -> wait
                RET
READY_8042      ENDP


FAST_IO_DELAY   MACRO
                LOCAL   L1, L2
                JMP     SHORT L1
L1:             JMP     SHORT L2
L2:
                ENDM

;
; Enable address line A20 (fast)
;
FAST_A20_ON     PROC    NEAR
                MOV     AL, 0D1H
                OUT     STATUS_8042, AL
                FAST_IO_DELAY
                MOV     AL, 0DFH                ; Enable
                OUT     DATA_8042, AL
                RET
FAST_A20_ON     ENDP

;
; Disable address line A20 (fast)
;
FAST_A20_OFF    PROC    NEAR
                MOV     AL, 0D1H
                OUT     STATUS_8042, AL
                FAST_IO_DELAY
                MOV     AL, 0DDH                ; Disable
                OUT     DATA_8042, AL
                RET
FAST_A20_OFF    ENDP


;
; Enable address line A20 (Fujitsu FMR70)
;
FMR70_A20_ON    proc    near
                MOV     AL, 00110000B
                OUT     20H, AL
                RET
FMR70_A20_ON    ENDP
                
;
; Disable address line A20 (Fujitsu FMR70)
;
FMR70_A20_OFF   PROC    NEAR
                MOV     AL, 0
                OUT     20H, AL
                RET
FMR70_A20_OFF   ENDP


;
; Enable address line A20 (NEC PC-98)
;
PC98_A20_ON     PROC    NEAR
                MOV     AL, 0
                OUT     0F2H, AL
                RET
PC98_A20_ON     ENDP
                
;
; Disable address line A20 (NEC PC-98)
;
PC98_A20_OFF    PROC    NEAR
                MOV     AL, 0FFH
                OUT     0F2H, AL
                RET
PC98_A20_OFF    ENDP


;
; Enable address line A20 (patched)
;
PATCH_A20_ON    PROC    NEAR
                RET
                DB      "PATCH_A20_ON "
                RET
PATCH_A20_ON    ENDP


;
; Disable address line A20 (patched)
;
PATCH_A20_OFF   PROC    NEAR
                RET
                DB      "PATCH_A20_OFF"
                RET
PATCH_A20_OFF   ENDP


;
;
;
                ASSUME  DS:SV_DATA
CLEANUP_A20     PROC    NEAR
                CMP     A20_FLAG, FALSE
                JE      SHORT CA20_RET
                CALL    A20_OFF
CA20_RET:       RET
CLEANUP_A20     ENDP


;
; Check if A20 is enabled
;
; Returns AX=0 iff A20 is disabled
;
; Note: disables interrupts
;
                ASSUME  DS:SV_DATA

CHECK_A20       PROC    NEAR
                PUSH    DS
                PUSH    ES
                MOV     AX, 1                   ; Assume A20 is enabled
                CMP     VCPI_FLAG, FALSE        ; VCPI active?
                JNE     SHORT CKA20_RET         ; Yes -> done
                MOV     AX, 0000H
                MOV     DS, AX                  ; We compare data at 0000:0000
                ASSUME  DS:NOTHING
                MOV     AX, 0FFFFH              ; to data at FFFF:0010
                MOV     ES, AX
                CLI                             ; Avoid unexpected things
                MOV     AX, DS:[00H]            ; (Divide exception)
                XOR     AX, ES:[10H]            ; Equal contents?
                JNZ     SHORT CKA20_RET         ; No -> A20 on (or no mem)
                NOT     WORD PTR DS:[00H]       ; Change word at 0000:0000
                MOV     AX, DS:[00H]            ; Get new value
                XOR     AX, ES:[10H]            ; Still equal contents?
                NOT     WORD PTR DS:[00H]       ; Restore word at 0000:0000
CKA20_RET:      POP     ES
                POP     DS
                RET
CHECK_A20       ENDP

INIT_CODE       ENDS

                END
