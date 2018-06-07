;
; EMXL.ASM -- emx loader (emxl.exe)
;
; Copyright (c) 1991-1996 by Eberhard Mattes
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
; <START_OF_EXCEPTION>
; As a special exception, if you bind emxl.exe to an executable file
; (using emxbind), this does not cause the resulting executable file
; to be covered by the GNU General Public License.  This exception
; does not however invalidate any other reasons why the executable
; file might be covered by the GNU General Public License.  However,
; if you bind a modified copy of emxl.exe to an executable file, you
; have to include source code for emxl.exe.  When distributing
; emxl.exe as separate file, the GPL applies without exceptions to
; emxl.exe.
;
; If you modify this file, you have to replace the text between
; the outer <START_OF_EXCEPTION> and <END_OF_EXCEPTION> markers with
; the following paragraph (which currently does not apply as you have
; not modified this file):
;
; As a special exception, if you bind emxl.exe to an executable file
; (using emxbind), this does not cause the resulting executable file
; to be covered by the GNU General Public License.  The source code
; for emxl.exe must be distributed with the executable file.  This
; exception does not however invalidate any other reasons why the
; executable file might be covered by the GNU General Public License.
; <END_OF_EXCEPTION>
;

                INCLUDE EMX.INC
                INCLUDE HEADERS.INC
                INCLUDE VERSION.INC

                .8086

;
; Data (the name SV_DATA stems from emx, we're using EMX.INC!)
;
SV_DATA         SEGMENT

PSP_SEG         WORD    ?                       ; Program prefix segment
ENV_SEG         WORD    ?                       ; Environment segment
NAME_OFF        WORD    ?                       ; Offset of prog name (ENV_SEG)
NAME_LEN        WORD    ?                       ; Length of prog name

;
; Parameter block for INT 21H, AX=4B00H
;
PAR_BLOCK       WORD    0                       ; Copy parent's environment
                WORD    OFFSET CMD_LINE         ; Command line
                WORD    SEG CMD_LINE
                WORD    5CH                     ; FCB1
PB_SEG1         WORD    ?
                WORD    6CH                     ; FCB2
PB_SEG2         WORD    ?

;
; DPMI_FLAG is non-zero if a DPMI server has been found, but no
; VCPI server
;
DPMI_FLAG       BYTE    FALSE                   ; Initially FALSE

;
; Messages
;
$EMX_NOT_FOUND  BYTE    "emx not found", CR, LF, "$"
$RSX_NOT_FOUND  BYTE    "rsx not found, DPMI not supported by emx", CR, LF, "$"
$USE_EMXBIND    BYTE    "Use emxbind", CR, LF, "$"
$BAD_ENV        BYTE    "Bad environment", CR, LF, "$"

;
; Names of devices implemented by expanded memory managers
;
EMS_FNAME       BYTE    "EMMXXXX0", 0           ; EMS enabled
NOEMS_FNAME1    BYTE    "EMMQXXX0", 0           ; EMS disabled
NOEMS_FNAME2    BYTE    "$MMXXXX0",0            ; EMS disabled

;
; Names of environment variables
;
$EMX            BYTE    "EMX", 0
$PATH           BYTE    "PATH", 0

;
; Name of emx.exe for searching in the current working directory and PATH
;
$EMX_EXE        BYTE    "emx.exe", 0
EMX_EXE_LEN     =       THIS BYTE - $EMX_EXE    ; Length of name, with 0

;
; This command line is passed to emx.exe
;
CMD_LINE        BYTE    7                       ; 7 bytes
                BYTE    "-/"                    ; Special marker
CMD_LINE_PSP    BYTE    "0000"                  ; PSP_SEG, hexadecimal
                BYTE    "/"                     ; Another special marker
                BYTE    CR                      ; End of command line

;
; Build path name here when using directory from PATH environment variable
;
PGM_NAME        BYTE    128 DUP (?)
PGM_NAME_END    LABEL   BYTE

SV_DATA         ENDS


;
; This area will be patched by emxbind
;
HDR_SEG         SEGMENT
PATCH           LABEL   BIND_HEADER
                BYTE    "emx ", VERSION, 0
                BYTE    (SIZE BIND_HEADER - HDR_VERSION_LEN) DUP (0)
HDR_SEG         ENDS

;
; Code
;
INIT_CODE       SEGMENT

                ASSUME  CS:INIT_CODE
                ASSUME  DS:NOTHING, ES:NOTHING

;
; Setup data segment.  Note: This macro modifies AX.
;
SET_DS          MACRO
                MOV     AX, SV_DATA
                MOV     DS, AX
                ASSUME  DS:SV_DATA
                ENDM

;
; The program starts here
;
ENTRY:          SET_DS                          ; Setup data segment
                MOV     PSP_SEG, ES             ; Save program prefix segment
                CALL    INIT                    ; Initialize
                CALL    CHECK_DPMI              ; Check for DPMI
                CALL    GET_NAME                ; Get program name
                CALL    BIND_HDR                ; Check patch area
                CALL    TRY_EMX_ENV             ; Use EMX environment variable
                CALL    TRY_CWD                 ; Search current directory
                CALL    TRY_PATH                ; Use PATH environment variable
                LEA     DX, $EMX_NOT_FOUND      ; Give up
                CMP     DPMI_FLAG, FALSE
                JE      FAIL
                LEA     DX, $RSX_NOT_FOUND
FAIL:           JMP     ABORT

;
; Initialization
;
INIT            PROC    NEAR
                MOV     AX, PSP_SEG
                MOV     PB_SEG1, AX             ; Fill in parameter block
                MOV     PB_SEG2, AX             ; for DOS function 4BH (EXEC)
                MOV     ES, AX
                MOV     AX, ES:[2CH]            ; Get environment segment
                MOV     ENV_SEG, AX
                LEA     BX, CMD_LINE_PSP        ; Insert PSP_SEG, hexadecimal
                MOV     CX, 0404H               ; 4 digits, shift by 4 bits
                MOV     DX, PSP_SEG
INIT_1:         ROL     DX, CL
                MOV     AL, DL
                AND     AL, 0FH
                ADD     AL, "0"
                CMP     AL, "9"
                JBE     INIT_2
                ADD     AL, "A" - ("0" + 10)
INIT_2:         MOV     [BX], AL
                INC     BX
                DEC     CH
                JNZ     INIT_1
                RET
INIT            ENDP


;
; Check for DPMI server
;
CHECK_DPMI      PROC    NEAR
                MOV     AX, 1687H               ; Check for DPMI server
                INT     2FH
                OR      AX, AX                  ; Server present?
                JNZ     DPMI_RET                ; No  -> call emx.exe
                TEST    BX, 1                   ; 32-bit programs supported?
                JZ      DPMI_RET                ; No  -> hope we can get VCPI
;
; Now we have detected a DPMI server.  Run rsx if we don't find a
; VCPI server.  First we have to check for an EMM.
;
; First, check the EMS interrupt vector
;
                MOV     AX, 3567H               ; Get interrupt vector 67H
                INT     21H
                MOV     AX, ES
                OR      AX, BX                  ; Is the vector 0:0 ?
                JZ      NO_VCPI                 ; Yes -> no VCPI
;
; Check for an EMM with EMS enabled
;
                LEA     DX, EMS_FNAME
                CALL    CHECK_EMM               ; EMS present?
                JC      EMM_DISABLED            ; No  -> try disabled EMS
;
; Check EMM status
;
                MOV     AH, 40H                 ; Get EMM status
                INT     67H
                CMP     AH, 0                   ; Successful?
                JE      EMM_OK                  ; Yes -> check VCPI
                JMP     NO_VCPI                 ; No VCPI

;
; Check for an EMM with EMS disabled (using undocumented "features")
;
EMM_DISABLED:   LEA     DX, NOEMS_FNAME1
                CALL    CHECK_EMM
                JNC     EMM_OK                  ; EMM present
                LEA     DX, NOEMS_FNAME2
                CALL    CHECK_EMM
                JC      NO_VCPI                 ; No EMM present -> no VCPI
;
; There is an EMM with EMS enabled or disabled.  Check for VCPI
;
EMM_OK:         MOV     AX, 0DE00H              ; VCPI presence detection
                INT     67H
                CMP     AH, 0                   ; VCPI present?
                JE      DPMI_RET                ; Yes -> use emx
;
; Now we have found a DPMI server, but no VCPI server.  We
; should use rsx instead of emx
;
NO_VCPI:        MOV     DPMI_FLAG, NOT FALSE    ; Set flag
                MOV     AX, "SR"                ; First 2 letters of "RSX"
                MOV     WORD PTR $EMX, AX       ; Patch name of env. variable
                MOV     AX, "sr"                ; First 2 letters of "rsx.exe"
                MOV     WORD PTR $EMX_EXE, AX   ; Patch name of executable
DPMI_RET:       RET
CHECK_DPMI      ENDP


;
; Check for the presence of an expanded memory manager (emx.exe
; will perform more complete checks)
;
; In:   DX      Pointer to device name
;
; Out:  NC      EMM present
;
CHECK_EMM       PROC    NEAR
                MOV     AX, 3D00H               ; Open
                INT     21H
                JC      CE_RET
;
; A file or device with that name exists.  Check for a device
;
                MOV     BX, AX                  ; Handle
                MOV     AX, 4400H               ; IOCTL: get device data
                INT     21H
                JC      CE_FAIL                 ; Failure -> no EMM
                TEST    DL, 80H                 ; Device?
                JZ      CE_FAIL                 ; No -> no EMM
                MOV     AX, 4407H               ; IOCTL: check output status
                INT     21H
                JC      CE_FAIL                 ; Failure -> no EMM
                CMP     AL, 0FFH                ; Ready?
                JNE     CE_FAIL                 ; No  -> no EMM
                MOV     AH, 3EH                 ; Close
                INT     21H
                CLC
CE_RET:         RET

CE_FAIL:        MOV     AH, 3EH                 ; Close
                INT     21H
                STC
                RET
CHECK_EMM       ENDP


;
; Find program name
;
GET_NAME        PROC    NEAR
                MOV     ES, ENV_SEG             ; Scan environment
                XOR     DI, DI                  ; starting at offset 0
                MOV     CX, 8000H               ; Maximum environment size
                XOR     AL, AL                  ; Search for bytes of zeros
                CLD                             ; Incrementing
GET_NAME_1:     REPNE   SCAS BYTE PTR ES:[DI]   ; Skip string
                JNE     GET_NAME_ERR            ; Count exhausted -> error
                SCAS    BYTE PTR ES:[DI]        ; End of environment?
                JE      GET_NAME_9              ; Yes -> program name found
                LOOP    GET_NAME_1              ; Next string, 1st char skipped
GET_NAME_ERR:   LEA     DX, $BAD_ENV            ; Bad environment
                JMP     ABORT

GET_NAME_9:     ADD     DI, 2                   ; Skip count
                MOV     NAME_OFF, DI            ; Offset of program name
                CALL    STRLEN                  ; Compute length
                MOV     NAME_LEN, CX
                RET
GET_NAME        ENDP


;
; Check emxbind patch area
;
BIND_HDR        PROC    NEAR
                MOV     AX, HDR_SEG
                MOV     ES, AX                  ; Access emxbind patch area
                ASSUME  ES:HDR_SEG
                CMP     PATCH.BND_BIND_FLAG, FALSE ; Bound?
                JNE     BIND_HDR_1              ; Yes -> continue
                LEA     DX, $USE_EMXBIND        ; Must be bound to a.out
                JMP     ABORT
BIND_HDR_1:     RET
BIND_HDR        ENDP

                ASSUME  ES:NOTHING

;
; Display error message and abort program
;
; In:   DX      Offset of dollar-delimited error message in SV_DATA
;
ABORT:          SET_DS                          ; This can't hurt
                MOV     AH, 09H                 ; Display string
                INT     21H
                MOV     AX, 4C01H               ; Terminate process, rc=1
                INT     21H

;
; Try to find emx.exe using EMX environment variable
;
TRY_EMX_ENV     PROC    NEAR
                LEA     BX, $EMX
                CALL    GETENV
                JC      TRY_EMX_ENV_RET
                MOV     DX, DI
                MOV_DS_ES
                ASSUME  DS:NOTHING
                CALL    SPAWN
                ASSUME  DS:SV_DATA
TRY_EMX_ENV_RET:RET
TRY_EMX_ENV     ENDP

;
; Try to find emx.exe in current working directory
;
TRY_CWD         PROC    NEAR
                LEA     DX, $EMX_EXE
                CALL    SPAWN
                RET
TRY_CWD         ENDP

;
; Try to find emx.exe in the directories listed in the PATH environment
; variable
;
TRY_PATH        PROC    NEAR
                LEA     BX, $PATH
                CALL    GETENV
                JC      TRY_PATH_RET
                MOV     SI, DI
FIND_2:         LEA     DI, PGM_NAME
FIND_3:         MOV     AL, ES:[SI]
                INC     SI
                OR      AL, AL
                JZ      TRY_PATH_RET
                CMP     AL, " "
                JE      FIND_3
                CMP     AL, TAB
                JE      FIND_3
                CMP     AL, ";"
                JE      FIND_3
FIND_4:         CMP     DI, OFFSET PGM_NAME_END
                JAE     FIND_5
                MOV     [DI], AL
                MOV     AH, AL
                INC     DI
FIND_5:         MOV     AL, ES:[SI]
                INC     SI
                OR      AL, AL
                JZ      FIND_6
                CMP     AL, " "
                JE      FIND_5
                CMP     AL, TAB
                JE      FIND_5
                CMP     AL, ";"
                JNE     FIND_4
FIND_6:         DEC     SI
                CMP     DI, OFFSET PGM_NAME_END - EMX_EXE_LEN
                JAE     FIND_2
;
; We don't support DBCS in emxl.exe: PATH should not contain directories
; whose name ends with a DBCS characters whose 2nd byte is 2F, 3AH, or 5CH.
;
                CMP     AH, "\"
                JE      FIND_7
                CMP     AH, "/"
                JE      FIND_7
                CMP     AH, ":"
                JE      FIND_7
                CMP     DI, OFFSET PGM_NAME_END
                JAE     FIND_7
                MOV     BYTE PTR DS:[DI], "\"
                INC     DI
FIND_7:         PUSH    ES
                PUSH    SI
                LEA     SI, $EMX_EXE
                MOV     CX, EMX_EXE_LEN
FIND_8:         LODSB
                MOV     [DI], AL
                INC     DI
                LOOP    FIND_8
                LEA     DX, PGM_NAME
                CALL    SPAWN
                POP     SI
                POP     ES
                JMP     FIND_2

TRY_PATH_RET:   RET

TRY_PATH        ENDP



;
; Find environment entry
;
; In:   BX      Points to zero-terminated name of environment variable
;
; Out:  CY      Not found
;       NC      Found
;       DI      Points to value of environment variable
;

                ASSUME  DS:SV_DATA

GETENV          PROC    NEAR
                MOV     ES, ENV_SEG
                XOR     DI, DI
                CLD
GETENV_NEXT:    CMP     BYTE PTR ES:[DI], 0     ; Empty environment?
                JE      GETENV_FAILURE          ; Yes -> not found
                PUSH    BX                      ; Save pointer to name
GETENV_COMPARE: MOV     AL, [BX]
                CMP     AL, ES:[DI]             ; Compare names
                JNE     GETENV_DIFF             ; Mismatch -> try next one
                OR      AL, AL                  ; Shouldn't happen (`='!)
                JZ      GETENV_DIFF             ; (name matches completely)
                INC     BX
                INC     DI
                JMP     GETENV_COMPARE          ; Compare next character
GETENV_DIFF:    POP     BX                      ; Restore pointer to name
                OR      AL, AL                  ; End of name reached?
                JE      GETENV_EQUAL            ; Yes -> candidate found
GETENV_SKIP:    XOR     AL, AL
                MOV     CX, 32767               ; Search for next entry
                REPNE   SCAS BYTE PTR ES:[DI]
                JMP     GETENV_NEXT             ; Check that entry

GETENV_EQUAL:   CMP     BYTE PTR ES:[DI], "="   ; Exact match?
                JNE     GETENV_SKIP             ; No  -> go to next entry
                INC     DI                      ; Skip `='
                CLC
                JMP     GETENV_RET              ; Return pointer to value

GETENV_FAILURE: STC                             ; Not found
GETENV_RET:     RET

GETENV          ENDP

;
; Compute the length of a zero-terminated string
;
; In:   ES:SI   Points to string
;
; Out:  CX      Length
;
STRLEN          PROC    NEAR
                XOR     CX, CX
                XOR     AL, AL
STRLEN_1:       SCASB
                JE      STRLEN_2
                INC     CX
                JMP     STRLEN_1
STRLEN_2:       RET
STRLEN          ENDP

;
; Try to run a program. Exit if successful, return if failed.
;
; In:   DS:DX   Points to path name of program
;
; Out:  DS      SV_DATA
;

                ASSUME  DS:NOTHING

SPAWN           PROC    NEAR
                MOV     AX, SV_DATA
                MOV     ES, AX
                ASSUME  ES:SV_DATA
                LEA     BX, PAR_BLOCK           ; ES:BX -> parameter block
                MOV     AX, 4B00H               ; Load and execute program
                INT     21H
                SET_DS                          ; Restore data segment
                JNC     DONE                    ; Success -> terminate
                RET

DONE:           MOV     AH, 4DH                 ; Get return code of child
                INT     21H                     ; process
                MOV     AH, 4CH                 ; terminate process
                INT     21H

SPAWN           ENDP

                ASSUME  ES:NOTHING


INIT_CODE       ENDS


RM_STACK        SEGMENT
                WORD    256 DUP (?)
RM_STACK        ENDS

                END     ENTRY
