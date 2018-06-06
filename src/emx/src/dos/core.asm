;
; CORE.ASM -- Dump core
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
                INCLUDE OPRINT.INC
                INCLUDE HEADERS.INC
                INCLUDE PMINT.INC
                INCLUDE SIGNAL.INC
                INCLUDE PROCESS.INC

                PUBLIC  CORE_DUMP, CORE_MAIN, CORE_REGS_I, CORE_REGS_P

SV_DATA         SEGMENT

CORE_SIZE       DD      ?
CORE_HANDLE     DD      ?

;
; User area header
;
U_HDR           USER_AREA <>

FILL_BUF        DB      32 DUP (0)

$CORE           DB      "core", 0

$CORE_DUMPED    DB      "core dumped", CR, LF, 0

SV_DATA         ENDS

SV_CODE         SEGMENT

                ASSUME  CS:SV_CODE, DS:NOTHING

;
; Write "core" image file
;
; In:   BX      Pointer to process table entry
;

                TALIGN  4
                ASSUME  DS:SV_DATA
CORE_DUMP       PROC    NEAR
                PUSH    PROCESS_PTR
                MOV     PROCESS_PTR, NO_PROCESS
                LEA     EDX, $CORE
                MOV     CX, 0
                MOV     AH, 3CH
                INT     21H
                JC      SHORT CORE_DUMP_RET
                PUSH    BX
                CALL    CORE_MAIN
                MOV     EBX, CORE_HANDLE
                MOV     AH, 3EH
                INT     21H
                POP     BX
                LEA     EDX, $CORE_DUMPED
                CALL    OTEXT
CORE_DUMP_RET:  POP     PROCESS_PTR
                RET
CORE_DUMP       ENDP


;
; Write a core image file
;
; In:   EAX     Handle
;       BX      Pointer to process table entry
;
; Out:  CY      Error
;       AX      Error code (CY)
;
; Note: This procedure assumes that the file has been rewound.
;
                TALIGN  4
                ASSUME  DS:SV_DATA
                ASSUME  BX:PTR PROCESS
CORE_MAIN       PROC    NEAR
                MOV     CORE_HANDLE, EAX
                MOV     CORE_SIZE, 0
                MOV     U_HDR.U_MAGIC, UMAGIC
                MOV     U_HDR.U_AR0, USER_AREA.U_REGS + KERNEL_U_ADDR
;
; Set base and end fields
;
                MOV     EAX, [BX].P_DATA_OFF
                MOV     EDX, [BX].P_BSS_OFF
                ADD     EDX, [BX].P_BSS_SIZE
                MOV     U_HDR.U_DATA_BASE, EAX
                MOV     U_HDR.U_DATA_END, EDX
                MOV     EAX, [BX].P_INIT_BRK
                MOV     EDX, [BX].P_BRK
                MOV     U_HDR.U_HEAP_BASE, EAX
                MOV     U_HDR.U_HEAP_END, EDX
                MOV     U_HDR.U_HEAP_BRK, EDX
                MOV     EAX, [BX].P_STACK_ADDR
                MOV     EDX, EAX
                SUB     EAX, [BX].P_STACK_SIZE
                MOV     U_HDR.U_STACK_BASE, EAX
                MOV     U_HDR.U_STACK_END, EDX
                MOV     EAX, U_HDR.U_UESP
                MOV     U_HDR.U_STACK_LOW, EAX
                ASSUME  BX:NOTHING
;
; Set offsets
;
                MOV     U_HDR.U_DATA_OFF, U_OFFSET
                MOV     EAX, U_HDR.U_DATA_END
                SUB     EAX, U_HDR.U_DATA_BASE
                CALL    ROUND_PAGE
                ADD     EAX, U_HDR.U_DATA_OFF
                MOV     U_HDR.U_HEAP_OFF, EAX
                MOV     EAX, U_HDR.U_HEAP_BRK
                SUB     EAX, U_HDR.U_HEAP_BASE
                CALL    ROUND_PAGE
                ADD     EAX, U_HDR.U_HEAP_OFF
                MOV     U_HDR.U_STACK_OFF, EAX
;
; Write header
;
                LEA     ESI, U_HDR
                MOV     ECX, SIZE U_HDR
                MOV     AX, DS
                CALL    CORE_WRITE
                JC      SHORT CORE_MAIN_RET
;
; Write data
;
                MOV     EAX, U_HDR.U_DATA_OFF
                CALL    CORE_FILL
                JNZ     SHORT CORE_MAIN_RET
                MOV     ESI, U_HDR.U_DATA_BASE
                MOV     ECX, U_HDR.U_DATA_END
                SUB     ECX, ESI
                JBE     SHORT CM_10
                MOV     AX, U_HDR.U_DS
                CALL    CORE_WRITE
                JC      SHORT CORE_MAIN_RET
CM_10:
;
; Write heap
;
                MOV     EAX, U_HDR.U_HEAP_OFF
                CALL    CORE_FILL
                JNZ     SHORT CORE_MAIN_RET
                MOV     ESI, U_HDR.U_HEAP_BASE
                MOV     ECX, U_HDR.U_HEAP_BRK
                SUB     ECX, ESI
                JBE     SHORT CM_11
                MOV     AX, U_HDR.U_DS
                CALL    CORE_WRITE
                JC      SHORT CORE_MAIN_RET
CM_11:
;
; Write stack
;
                MOV     EAX, U_HDR.U_STACK_OFF
                CALL    CORE_FILL
                JNZ     SHORT CORE_MAIN_RET
                MOV     ESI, U_HDR.U_STACK_LOW
                MOV     ECX, U_HDR.U_STACK_END
                SUB     ECX, ESI
                JBE     SHORT CM_12
                MOV     AX, U_HDR.U_SS
                CALL    CORE_WRITE
                JC      SHORT CORE_MAIN_RET
CM_12:          XOR     AX, AX
CORE_MAIN_RET:  RET
CORE_MAIN       ENDP



;
; Write memory area to core file
;
; In:   AX:ESI  Pointer to memory area
;       ECX     Size of memory area
;
; Out:  CY      Error
;
                TALIGN  4
                ASSUME  DS:SV_DATA
CORE_WRITE      PROC    NEAR
                PUSH    EBX
                PUSH    EDX
                ADD     CORE_SIZE, ECX
                MOV     EDX, ESI
                MOV     EBX, CORE_HANDLE
                PUSH    DS
                ASSUME  DS:NOTHING
                MOV     DS, AX
                MOV     AH, 40H
                INT     21H
                POP     DS
                ASSUME  DS:SV_DATA
                JC      SHORT CORE_WRITE_1
                CMP     EAX, ECX
                STC
                JNE     SHORT CORE_WRITE_1
                CLC
CORE_WRITE_1:   POP     EDX
                POP     EBX
                RET
CORE_WRITE      ENDP


;
; Fill core file
;
; In:   EAX     Fill file up to this position
;
; Out:  EAX     DOS error code
;       CY      Error
;
                TALIGN  4
                ASSUME  DS:SV_DATA
CORE_FILL       PROC    NEAR
                PUSH    ECX
                SUB     EAX, CORE_SIZE
                JBE     SHORT CORE_FILL_OK
                MOV     ECX, EAX
CORE_FILL_1:    PUSH    ECX
                CMP     ECX, SIZE FILL_BUF
                JBE     SHORT CORE_FILL_2
                MOV     ECX, SIZE FILL_BUF
CORE_FILL_2:    PUSH    ESI
                LEA     ESI, FILL_BUF
                MOV     AX, DS
                CALL    CORE_WRITE
                POP     ESI
                JC      SHORT CORE_FILL_ERR
                MOV     EAX, ECX
                POP     ECX
                SUB     ECX, EAX
                JNBE    SHORT CORE_FILL_1
CORE_FILL_OK:   XOR     EAX, EAX
CORE_FILL_RET:  POP     ECX
                RET

CORE_FILL_ERR:  POP     ECX
                JMP     SHORT CORE_FILL_RET
CORE_FILL       ENDP



;
; Copy registers from stack to U_HDR structure for CORE_MAIN
;
; Must not change BX
;
                TALIGN  4
                ASSUME  DS:SV_DATA
                ASSUME  BP:PTR ISTACKFRAME
CORE_REGS_I     PROC    NEAR
                PUSH    EAX
                PUSH    ECX
                PUSH    ESI
                PUSH    EDI
                LEA     EDI, U_HDR
                MOV_ES_DS
                MOV     ECX, SIZE U_HDR
                XOR     AL, AL
                REP     STOS BYTE PTR ES:[EDI]
                IRP     X, <EAX, EBX, ECX, EDX, ESI, EDI, EBP, ESP, EIP>
                MOV     EAX, I_&X
                MOV     U_HDR.U_&X, EAX
                ENDM
                MOV     EAX, I_EFLAGS
                MOV     U_HDR.U_EFL, EAX
                IRP     X, <CS, DS, ES, FS, GS, SS>
                MOV     AX, I_&X
                MOV     U_HDR.U_&X, AX
                ENDM
                MOV     EAX, U_HDR.U_ESP
                MOV     U_HDR.U_UESP, EAX
                ; TODO: 387

;
; Unwind the stack for syscalls.  Calling sequence for a syscall:
;
;       mov     al, syscall_no
;       mov     ah, 7fh
;       call    __syscall
;       ...
;
; __syscall:                    ; at 1000CH
;       call    __dos_syscall
;       ...

; __dos_syscall:                ; anywhere
;       int     21h
;       ret
;
                MOV     EDI, U_HDR.U_EIP
                MOV     ESI, U_HDR.U_ESP
                MOVZX   EDX, I_CS               ; Get limit of CS
                LSL     EDX, EDX
                CMP     EDI, 10000H + 2         ; Avoid bombing for invalid EIP
                JB      NO_UPDATE
                MOV     EAX, EDI
                INC     EAX
                CMP     EAX, EDX
                JAE     NO_UPDATE
                MOV     ES, I_CS                ; Get segment registers
                MOV     FS, I_SS
                CMP     WORD PTR ES:[EDI-2], 021CDH     ; Check for INT 21H
                JNE     SHORT UPDATE
                CMP     BYTE PTR ES:[EDI], 0C3H         ; Check for RET
                JNE     SHORT UPDATE
                CMP     DWORD PTR FS:[ESI], 1000CH + 5  ; Check return address
                JNE     SHORT UPDATE
                ADD     ESI, 4                  ; Undo "call __dos_syscall"
                MOV     EDI, FS:[ESI]           ; Undo "call __syscall"
                ADD     ESI, 4
;
; Now check for specific syscalls:
;
;        Name          ³  AL   ³ PUSH
;        ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÅÄÄÄÄÄÄÄÅÄÄÄÄÄÄÄÄ
;        __core        ³  11H  ³ 1 (EBX)
;        __raise       ³  0EH  ³ 0
;        __signal      ³  0CH  ³ 0
;        __sigprocmask ³  37H  ³ 1 (EBX)
;
                CMP     EDI, 10000H + 9         ; Avoid bombing for invalid
                JB      SHORT NO_UPDATE         ; return address
                CMP     EDI, EDX
                JAE     SHORT NO_UPDATE
                CMP     BYTE PTR ES:[EDI-9], 0B0H    ; Check for "MOV AL,"
                JNE     SHORT UPDATE
                MOV     AL, BYTE PTR ES:[EDI-8]
                CMP     AL, 0CH                 ; __signal
                JE      SHORT HANDLE_SYSCALL
                CMP     AL, 0EH                 ; __raise
                JE      SHORT HANDLE_SYSCALL
                CMP     AL, 11H                 ; __core
                JE      SHORT HANDLE_SYSCALL
                CMP     AL, 37H                 ; __sigprocmask
                JNE     SHORT UPDATE
HANDLE_SYSCALL: CMP     WORD PTR ES:[EDI-7], 07FB4H  ; Check for "MOV AH, 7FH"
                JNE     SHORT UPDATE
                CMP     BYTE PTR ES:[EDI-5], 0e8H    ; Check for "CALL"
                JNE     SHORT UPDATE
;
; It's one of the above syscalls.  Remove any arguments.
;
                MOV     AL, ES:[EDI-8]
                CMP     AL, 11H                 ; __core
                JE      SHORT ONE_ARG
                CMP     AL, 37H                 ; __sigprocmask
                JNE     SHORT NO_ARGS
ONE_ARG:        ADD     ESI, 4                  ; Remove one argument word
NO_ARGS:        MOV     EDI, FS:[ESI]           ; Undo "CALL __raise" etc.
                ADD     ESI, 4
;
; Update EIP and ESP
;
UPDATE:         MOV     U_HDR.U_EIP, EDI
                MOV     U_HDR.U_ESP, ESI
NO_UPDATE:      POP     EDI
                POP     ESI
                POP     ECX
                POP     EAX
                RET
                ASSUME  BP:NOTHING
CORE_REGS_I     ENDP

;
; Copy registers from process table entry to U_HDR structure for CORE_MAIN
;
; In:   BX      Pointer to process table entry
;
                TALIGN  4
                ASSUME  DS:SV_DATA
                ASSUME  BX:PTR PROCESS
CORE_REGS_P     PROC    NEAR
                PUSH    EAX
                PUSH    ECX
                PUSH    EDI
                LEA     EDI, U_HDR
                MOV_ES_DS
                MOV     ECX, SIZE U_HDR
                XOR     AL, AL
                REP     STOS BYTE PTR ES:[EDI]
                IRP     X, <EAX, EBX, ECX, EDX, ESI, EDI, EBP, ESP, EIP>
                MOV     EAX, [BX].P_&X
                MOV     U_HDR.U_&X, EAX
                ENDM
                MOV     EAX, [BX].P_EFLAGS
                MOV     U_HDR.U_EFL, EAX
                IRP     X, <CS, DS, ES, FS, GS, SS>
                MOV     AX, [BX].P_&X
                MOV     U_HDR.U_&X, AX
                ENDM
                MOV     EAX, U_HDR.U_ESP
                MOV     U_HDR.U_UESP, EAX
        ;...387
                POP     EDI
                POP     ECX
                POP     EAX
                RET
                ASSUME  BX:NOTHING
CORE_REGS_P     ENDP

;
; Round up to multiple of page size
;
; In:   EAX     Value to be rounded
;
; Out:  EAX     Rounded value
;
                TALIGN  4
                ASSUME  DS:NOTHING
ROUND_PAGE      PROC    NEAR
                DEC     EAX
                AND     EAX, NOT 0FFFH
                ADD     EAX, 1000H
                RET
ROUND_PAGE      ENDP



SV_CODE         ENDS

                END
