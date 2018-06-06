;
; PMIO.ASM -- File I/O for the kernel
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

                PUBLIC  DEBUG_FLAG
                PUBLIC  OPEN, SEEK, READ, WRITE, CREATE_TMP, CLOSE
                PUBLIC  FORCE_STDOUT, FIND_FILE, ACCESS


SV_DATA         SEGMENT

DEBUG_FLAG      DB      FALSE
STDOUT_NAME     DB      "CON", 0

SV_DATA         ENDS



PMIO_BEGIN      MACRO
                PUSH    PROCESS_PTR
                MOV     PROCESS_PTR, NO_PROCESS
                ENDM

PMIO_END        MACRO
                POP     PROCESS_PTR
                ENDM

SV_CODE         SEGMENT

                ASSUME  CS:SV_CODE, DS:NOTHING, ES:SV_DATA


;
; Open file
;
; In:   AX:EDX  Pointer to name
;       CL      Access code
;
; Out:  CY      Error
;       EAX     File handle or error code
;
                ASSUME  DS:SV_DATA
OPEN            PROC    NEAR
                PMIO_BEGIN
                PUSH    DS
                MOV     DS, AX
                ASSUME  DS:NOTHING
                MOV     AH, 3DH
                MOV     AL, CL
                INT     21H
                POP     DS
                ASSUME  DS:SV_DATA
                PMIO_END
                RET
OPEN            ENDP

;
; Close file
;
; In:   BX      File handle
;
                ASSUME  DS:SV_DATA
CLOSE           PROC    NEAR
                PMIO_BEGIN
                MOVZX   EBX, BX
                MOV     AH, 3EH
                INT     21H
                PMIO_END
                RET
CLOSE           ENDP

;
; Create temporary file
;
; In:   AX:EDX  Pointer to name
;
                ASSUME  DS:SV_DATA
CREATE_TMP      PROC    NEAR
                PMIO_BEGIN
                PUSH    DS
                PUSH    ECX
                MOV     CX, 0000H
                MOV     DS, AX
                ASSUME  DS:NOTHING
                MOV     AH, 5AH
                INT     21H
                POP     ECX
                POP     DS
                ASSUME  DS:SV_DATA
                PMIO_END
                RET
CREATE_TMP      ENDP


;
; In:   BX      File handle
;       EDX     Position
;
                ASSUME  DS:SV_DATA
SEEK            PROC    NEAR
                PMIO_BEGIN
                PUSH    EAX
                MOVZX   EBX, BX
                MOV     AL, 0
                MOV     AH, 42H
                INT     21H
                POP     EAX
                PMIO_END
                RET
SEEK            ENDP


;
; In:   BX      File handle
;       ECX     Number of bytes
;       AX:EDX  Pointer to buffer
;
; Out:  CY      Error
;       NC NZ   EOF
;       NC ZR   OK
;
                ASSUME  DS:SV_DATA
READ            PROC    NEAR
                PMIO_BEGIN
                PUSH    EAX
                PUSH    DS
                MOV     DS, AX
                ASSUME  DS:NOTHING
                MOVZX   EBX, BX
                MOV     AH, 3FH
                INT     21H
                JC      SHORT READ_RET
                CMP     ECX, EAX
                CLC
READ_RET:       POP     DS
                ASSUME  DS:SV_DATA
                POP     EAX
                PMIO_END
                RET
READ            ENDP


;
; In:   BX      File handle
;       ECX     Number of bytes
;       AX:EDX  Pointer to buffer
;
; Out:  CY      Error
;       NC NZ   Disk full
;       NC ZR   OK
;
                ASSUME  DS:SV_DATA
WRITE           PROC    NEAR
                PMIO_BEGIN
                PUSH    EAX
                PUSH    DS
                MOV     DS, AX
                ASSUME  DS:NOTHING
                MOVZX   EBX, BX
                MOV     AH, 40H
                INT     21H
                JC      SHORT WRITE_RET
                CMP     ECX, EAX
                CLC
WRITE_RET:      POP     DS
                ASSUME  DS:SV_DATA
                POP     EAX
                PMIO_END
                RET
WRITE           ENDP


;
; Force stdout to refer to the CON device (put error message on screen)
;
                ASSUME  DS:SV_DATA
FORCE_STDOUT    PROC    NEAR
                PMIO_BEGIN
                XOR     EDX, EDX
                LEA     DX, STDOUT_NAME
                MOV     AH, 3DH
                MOV     AL, 41H                 ; Deny none; write
                INT     21H
                JC      SHORT FS_RET
                MOV     EBX, EAX
                MOV     ECX, 1                  ; stdout
                MOV     AH, 46H                 ; Force duplicate file handle
                INT     21H
FS_RET:         PMIO_END
                RET
FORCE_STDOUT    ENDP


;
; Find file in directories listed by an environment variable
;
; In:   DS:SI   File name
;       DS:BX   Buffer (filled by FIND_FILE), must be large enough
;       ES:EDI  Search path (separated by ';', 0 terminated)
;
; Out:  CY      Not found
;
; Note: ES:EDI must point to the value of the environment variable
;       The buffer pointed to by DS:BX will contain the path name on success
;       Blanks are not removed
;
FIND_FILE       PROC    NEAR
                PUSH    EDI
                PUSH    CX
FF_NEXT_DIR:    
;
; Now ES:DI points to the next directory
; Build path name at DS:BX
;
                PUSH    BX
                PUSH    SI
                XOR     CX, CX
FF_DIR_COPY:    MOV     AL, ES:[EDI]
                OR      AL, AL
                JZ      SHORT FF_DIR_END
                CMP     AL, ";"
                JE      SHORT FF_DIR_END
                MOV     [BX], AL
                INC     EDI
                INC     BX
                INC     CX
                JMP     SHORT FF_DIR_COPY
FF_DIR_END:     JCXZ    FF_EMPTY
                MOV     AL, [BX-1]
                CMP     AL, "\"
                JE      SHORT FF_FNAME_COPY
                CMP     AL, "/"
                JE      SHORT FF_FNAME_COPY
                CMP     AL, ":"
                JE      SHORT FF_FNAME_COPY
                MOV     BYTE PTR [BX], "\"
                INC     BX
FF_FNAME_COPY:  MOV     AL, [SI]
                MOV     [BX], AL
                INC     SI
                INC     BX
                OR      AL, AL
                JNZ     SHORT FF_FNAME_COPY
FF_EMPTY:       POP     SI
                POP     BX
                JCXZ    FF_SKIP_DIR
;
; Look for file
;
                PUSH    EDX
                MOVZX   EDX, BX
                CALL    ACCESS
                POP     EDX
                JNC     SHORT FF_RET
;
; Try next directory
;
FF_SKIP_DIR:    CMP     BYTE PTR ES:[EDI], 0
                JE      SHORT FF_FAILURE
                INC     EDI
                JMP     SHORT FF_NEXT_DIR

FF_FAILURE:     MOV     BYTE PTR [BX], 0
                STC
FF_RET:         POP     CX
                POP     EDI
                RET
FIND_FILE       ENDP



;
; In:   DS:EDX  Name
;
; Out:  CY      Not found or not a file
;
ACCESS          PROC    NEAR
                PUSH    AX
                PUSH    CX
                MOV     AH, 43H                 ; Get/set file attributes
                MOV     AL, 00H                 ; Get file attributes
                INT     21H
                JC      SHORT ACCESS_RET
                TEST    CX, 18H                 ; Volume label, directory?
                JZ      SHORT ACCESS_RET
                STC
ACCESS_RET:     POP     CX
                POP     AX
                RET
ACCESS          ENDP


SV_CODE         ENDS

                END
