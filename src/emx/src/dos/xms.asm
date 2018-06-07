;
; XMS.ASM -- Manage extended memory
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
                INCLUDE MEMORY.INC
                INCLUDE RPRINT.INC
                INCLUDE OPTIONS.INC
                INCLUDE VCPI.INC

                PUBLIC  XMS_FLAG, DISABLE_XMS_MEM
                PUBLIC  CHECK_XMS, INIT_XMS, CLEANUP_XMS
                PUBLIC  XMS_A20_ON, XMS_A20_OFF

XMS_NULL_HANDLE =       0

SV_DATA         SEGMENT

XMS_ENTRY       LABEL   DWORD
XMS_OFF         DW      ?
XMS_SEG         DW      ?

XMS_FLAG        DB      FALSE
DISABLE_XMS_MEM DB      FALSE

$XMS_A20_ON     DB      "Cannot enable A20 via XMS", CR, LF, 0
$XMS_VER        DB      "Unsupported XMS version", CR, LF, 0

SV_DATA         ENDS

INIT_CODE       SEGMENT

                ASSUME  CS:INIT_CODE, DS:NOTHING

;
; Check presence of XMS driver
;
                ASSUME  DS:SV_DATA
CHECK_XMS       PROC    NEAR
                CMP     DISABLE_EXT_MEM, FALSE  ; Extended memory disabled?
                JNE     SHORT CX_RET            ; Yes -> don't use XMS
                MOV     AX, 4300H               ; XMS installation check
                INT     2FH
                CMP     AL, 80H                 ; XMS available?
                JNE     SHORT CX_RET            ; No  -> skip
                MOV     AX, 4310H               ; Get entry point
                INT     2FH
                MOV     XMS_OFF, BX             ; Store entry point
                MOV     XMS_SEG, ES
                MOV     AH, 00H                 ; Get version
                CALL    XMS_ENTRY
                CMP     OVERRIDE_FLAG, FALSE    ; Override version check?
                JNE     SHORT CX_OK             ; Yes -> skip (danger!)
                CMP     AX, 0200H               ; Specification 2.0 or later?
                JB      SHORT XMS_VER           ; No  -> abort
                CMP     BX, 0206H               ; Driver revision >= 2.06?
                JB      SHORT XMS_VER           ; No  -> abort (buggy driver!)
CX_OK:          MOV     XMS_FLAG, NOT FALSE
CX_RET:         RET

XMS_VER:        LEA     DX, $XMS_VER            ; Unsupported XMS version
                CALL    RTEXT                   ; Display message
                MOV     AX, 4CFFH               ; Abort
                INT     21H

CHECK_XMS       ENDP

;
; Initialize high memory (XMS)
;
                ASSUME  DS:SV_DATA
INIT_XMS        PROC    NEAR
                CMP     DISABLE_XMS_MEM, FALSE  ; Usage of XMS memory disabled?
                JNE     IX9                     ; Yes -> skip
                LEA     SI, HIMEM_TAB[0]
                ASSUME  SI:PTR HIMEM_ENTRY
IX1:            CMP     HIMEM_COUNT, HIMEM_MAX  ; Table full?
                JAE     SHORT IX9               ; Yes -> done
                MOV     AH, 08H                 ; Query free extended memory
                CALL    XMS_ENTRY
                CMP     AX, 7                   ; Don't use less than 7KB
                JB      SHORT IX9
                MOV     DX, AX
                MOVZX   EAX, AX
                SHL     EAX, 10                 ; Multiply by 1K
                MOV     [SI].HM_SIZE, EAX
                MOV     AH, 09H                 ; Allocate extended memory blk
                CALL    XMS_ENTRY
                OR      AX, AX                  ; Ok?
                JZ      SHORT IX9               ; No  -> quit loop
                MOV     [SI].HM_HANDLE, DX      ; Save handle
                MOV     AH, 0CH                 ; Lock extended memory block
                CALL    XMS_ENTRY
                OR      AX, AX                  ; Ok?
                JZ      SHORT IX9               ; No  -> quit loop
                MOV     WORD PTR [SI].HM_ADDR[0], BX
                MOV     WORD PTR [SI].HM_ADDR[2], DX
                ADD     SI, SIZE HIMEM_ENTRY
                INC     HIMEM_COUNT
                JMP     SHORT IX1
                ASSUME  SI:NOTHING
IX9:            RET
INIT_XMS        ENDP


                ASSUME  DS:SV_DATA
CLEANUP_XMS     PROC    NEAR
                CMP     XMS_FLAG, FALSE
                JE      SHORT CUX_RET
                LEA     SI, HIMEM_TAB
                ASSUME  SI:PTR HIMEM_ENTRY
                MOV     CX, HIMEM_COUNT
                JCXZ    CUX_RET
;
; Note: If XMS_FLAG is non-zero, we have only extended memory blocks
;       allocated via XMS!  There won't be a memory block allocated
;       by hooking INT 15H.
;
CUX_1:          MOV     DX, [SI].HM_HANDLE
                MOV     AH, 0DH                 ; Unlock extended memory block
                CALL    XMS_ENTRY
                MOV     AH, 0AH                 ; Free extended memory block
                CALL    XMS_ENTRY
                ADD     SI, SIZE HIMEM_ENTRY
                LOOP    CUX_1
                ASSUME  SI:NOTHING
CUX_RET:        RET
CLEANUP_XMS     ENDP


;
; Enable A20
;
                ASSUME  DS:SV_DATA
XMS_A20_ON      PROC    NEAR
                MOV     AH, 05H
                CALL    XMS_ENTRY
                OR      AX, AX
                JNZ     SHORT XMS_A20_ON_RET
                LEA     DX, $XMS_A20_ON
                CALL    RTEXT
                MOV     AX, 4CFFH
                INT     21H
XMS_A20_ON_RET: CLI
                RET
XMS_A20_ON      ENDP

;
; Disable A20
;
                ASSUME  DS:SV_DATA
XMS_A20_OFF     PROC    NEAR
                MOV     AH, 06H
                CALL    XMS_ENTRY
                CLI
                RET
XMS_A20_OFF     ENDP

INIT_CODE       ENDS

                END
