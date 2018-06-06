;
; PTRACE.ASM -- Implement ptrace()
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
                INCLUDE SIGNAL.INC
                INCLUDE PROCESS.INC
                INCLUDE PMINT.INC
                INCLUDE DEBUG.INC
                INCLUDE HEADERS.INC
                INCLUDE ERRORS.INC

SV_DATA         SEGMENT

                TALIGN  2
POKE_VALUE      DD      ?

REG_TABLE       STRUCT
RT_ADDR         DD      ?
RT_PROC         DW      ?
RT_LEN          DB      ?
REG_TABLE       ENDS

PTRACE_REGS     REG_TABLE       <USER_AREA.U_GS,   PROCESS.P_GS,     2>
                REG_TABLE       <USER_AREA.U_FS,   PROCESS.P_FS,     2>
                REG_TABLE       <USER_AREA.U_ES,   PROCESS.P_ES,     2>
                REG_TABLE       <USER_AREA.U_DS,   PROCESS.P_DS,     2>
                REG_TABLE       <USER_AREA.U_EDI,  PROCESS.P_EDI,    4>
                REG_TABLE       <USER_AREA.U_ESI,  PROCESS.P_ESI,    4>
                REG_TABLE       <USER_AREA.U_EBP,  PROCESS.P_EBP,    4>
                REG_TABLE       <USER_AREA.U_ESP,  PROCESS.P_ESP,    4>
                REG_TABLE       <USER_AREA.U_EBX,  PROCESS.P_EBX,    4>
                REG_TABLE       <USER_AREA.U_EDX,  PROCESS.P_EDX,    4>
                REG_TABLE       <USER_AREA.U_ECX,  PROCESS.P_ECX,    4>
                REG_TABLE       <USER_AREA.U_EAX,  PROCESS.P_EAX,    4>
                REG_TABLE       <USER_AREA.U_EIP,  PROCESS.P_EIP,    4>
                REG_TABLE       <USER_AREA.U_CS,   PROCESS.P_CS,     2>
                REG_TABLE       <USER_AREA.U_EFL,  PROCESS.P_EFLAGS, 4>
                REG_TABLE       <USER_AREA.U_UESP, PROCESS.P_ESP,    4>
                REG_TABLE       <USER_AREA.U_SS,   PROCESS.P_SS,     2>
                REG_TABLE       <-1, 0, 0>

SV_DATA         ENDS


SV_CODE         SEGMENT

                ASSUME  CS:SV_CODE, DS:NOTHING

                PUBLIC  PTRACE

                TALIGN  2
PTRACE_JMP      DW      PTRACE00                ; PTRACE_TRACEME
                DW      PTRACE01                ; PTRACE_PEEKTEXT
                DW      PTRACE02                ; PTRACE_PEEKDATA
                DW      PTRACE03                ; PTRACE_PEEKUSER
                DW      PTRACE04                ; PTRACE_POKETEXT
                DW      PTRACE05                ; PTRACE_POKEDATA
                DW      PTRACE06                ; PTRACE_POKEUSER
                DW      PTRACE07                ; PTRACE_RESUME
                DW      PTRACE08                ; PTRACE_EXIT
                DW      PTRACE09                ; PTRACE_STEP
                DW      PTRACE0A                ; PTRACE_SESSION
PTRACE_LAST     =       ($-PTRACE_JMP)/2-1

;
; Bits in EFLAGS that may be changed by ptrace()
;
EFLAGS_USER     =       0000110011010101B       ; OF, DF, SF, ZF, AF, PF, CF

; ----------------------------------------------------------------------------
; AX=7F08H: ptrace()
;
; Debugging support
;
; In:   EBX     request code (see /emx/include/sys/ptrace.h)
;       EDI     pid
;       EDX     address
;       ECX     data
;
; Out:  EAX     result
;       ECX     errno, if non-zero
;
; ----------------------------------------------------------------------------
                ASSUME  DS:SV_DATA
                ASSUME  BP:PTR ISTACKFRAME

                TALIGN  4
PTRACE:         MOV     ECX, I_ECX              ; ECX := data
                MOV     I_ECX, 0                ; No error
                MOV     EDI, I_EBX
                CMP     EDI, 0                  ; PTRACE_TRACEME?
                JE      SHORT SYS08_JMP         ; Yes -> ignore pid
                CMP     EDI, PTRACE_LAST        ; Valid request code?
                JA      SHORT SYS08_IO_ERR      ; No  -> I/O error
                MOV     EAX, I_EDI              ; Get process ID
                CALL    FIND_PROCESS            ; Lookup process table entry
                CMP     BX, NO_PROCESS          ; Found?
                JE      SHORT SYS08_ESRCH       ; No -> no such process
                ASSUME  BX:PTR PROCESS
                TEST    [BX].P_FLAGS, PF_DEBUG  ; Process in debugging mode?
                JZ      SHORT SYS08_IO_ERR      ; No -> I/O error
SYS08_JMP:      MOV     ESI, I_EDX              ; ESI := Address
                JMP     PTRACE_JMP[EDI*2]       ; Dispatch

SYS08_ESRCH:    MOV     I_ECX, ESRCH            ; No such process
                JMP     SHORT SYS08_ERROR       ; Failure

SYS08_EINVAL:   MOV     I_ECX, EINVAL           ; Invalid argument
                JMP     SHORT SYS08_ERROR       ; Failure

SYS08_IO_ERR:   MOV     I_ECX, EIO              ; I/O error
SYS08_ERROR:    MOV     I_EAX, -1               ; Failure
                RET


;
; Request code 0 is used (by Unix) to switch the current process to debugging
; mode. This is not used by emx. Ignored.
;
PTRACE00:       MOV     I_EAX, 0
                RET

;
; Request code 10 is used to switch to and from the child session. Only
; for OS/2.
;
PTRACE0A:       MOV     I_EAX, 0
                RET


;
; Request codes 1 and 2 are used for reading a word from code/data space.
;
PTRACE01:                                       ; Same as PTRACE02
PTRACE02:       CALL    PTRACE_ADDR             ; Check address
                .386P
                SLDT    DI                      ; Save LDT
                LLDT    [BX].P_LDT              ; Get LDT of child process
                MOV     ES, [BX].P_DS
                MOV     EAX, ES:[ESI]           ; Read the word
                LLDT    DI                      ; Note 2
                MOV     I_EAX, EAX              ; and return it in EAX
                RET
                .386
;
; Note 2: It is essential, that the LDT of the client process is active
;         until after accessing the memory of the client process.
;         Why is this necessary, the 386 uses the LDT only when loading
;         a segment register; the LDT isn't referenced after loading
;         the segment register? Correct. But accessing the memory of
;         the child could cause a page fault (or an interrupt may occur
;         between restoring the LDTR and accessing memory). The exception
;         (or interrupt) handler saves all the segment registers (and the
;         LDTR). When returning to the interrupted code, the segment registers
;         are restored using the LDTR saved on entry to the interrupt routine.
;         Therefore, one must be very cautious when switching LDTs!
;

;
; Request code 3 is used for reading from the process table (registers).
;
                TALIGN  2
PTRACE03:       MOV     EAX, ESI
                CMP     EAX, USER_AREA.U_AR0    ; Pointer to registers?
                JE      SHORT PEEKU_AR0         ; Yes -> special handling
                CMP     EAX, USER_AREA.U_FPVALID  ; Floating point flag?
                JE      SHORT PEEKU_FPVALID     ; Yes -> special handling
                CMP     EAX, USER_AREA.U_FPSTATUS ; Floating point status?
                JE      SHORT PEEKU_FPSTATUS      ; Yes -> special handling
                CMP     EAX, USER_AREA.U_FPSTATE  ; Floating point?
                JB      SHORT PEEKU_10
                CMP     EAX, USER_AREA.U_FPSTATE + 108 - 4
                JBE     SHORT PEEKU_FLOAT       ; Yes -> special handling
PEEKU_10:       CALL    USER_ADDR               ; Convert address
                JNZ     SYS08_IO_ERR
                ASSUME  SI:PTR REG_TABLE
                MOVZX   CX, [SI].RT_LEN         ; Get size of register (bytes)
                MOV     SI, [SI].RT_PROC        ; Pointer in process structure
PEEKU_11:       ADD     SI, BX
                MOV     I_EAX, 0                ; Clear return value
                LEA     DI, I_EAX
PEEKU_12:       LODS    BYTE PTR DS:[SI]
                MOV     SS:[DI], AL
                INC     DI
                LOOP    PEEKU_12
                ASSUME  SI:NOTHING
                RET

;
; A MASM bug strikes here if DWORD PTR isn't used: MASM omits the operand
; size prefix and inserts 4 bytes of immediate data.
;
PEEKU_AR0:      MOV     I_EAX, DWORD PTR (USER_AREA.U_REGS + KERNEL_U_ADDR)
                RET

PEEKU_FPSTATUS: MOV     I_EAX, 0
                RET

PEEKU_FPVALID:
              IF FLOATING_POINT
                MOV     I_EAX, 0FFH
              ELSE
                MOV     I_EAX, 0
              ENDIF
                RET

PEEKU_FLOAT:
              IF FLOATING_POINT
                FNOP                            ; Save 387 registers
                ADD     ESI, PROCESS.P_CW - USER_AREA.U_FPSTATE
                MOV     CX, 4
                JMP     SHORT PEEKU_11
              ELSE
                JMP     SYS08_IO_ERR
              ENDIF

;
; Request code 6 is used for writing to the process table (registers).
;
; Note: Only some bits of EFLAG can be written. Segment registers and
;       387 registers cannot be written.
;
                TALIGN  2
PTRACE06:       MOV     POKE_VALUE, ECX
                MOV     EAX, ESI
                CALL    USER_ADDR               ; Convert address
                JNZ     SYS08_IO_ERR
                ASSUME  SI:PTR REG_TABLE
                MOVZX   CX, [SI].RT_LEN
                MOV     DI, [SI].RT_PROC
                CMP     DI, PROCESS.P_EFLAGS
                JNE     SHORT POKEU_10
                ADD     DI, BX
                MOV     EAX, [DI]
                MOV     ECX, POKE_VALUE
                AND     ECX, EFLAGS_USER        ; Keep only changable bits
                AND     EAX, NOT EFLAGS_USER    ; Keep only unchangabe bits
                OR      EAX, ECX                ; Insert new bits
                MOV     [DI], EAX
                JMP     SHORT POKEU_ZERO

                ASSUME  SI:NOTHING

POKEU_10:       ADD     DI, BX
                LEA     SI, POKE_VALUE
                MOV_ES_DS
                REP     MOVSB
POKEU_ZERO:     MOV     I_EAX, 0                ; Return 0
                RET

;
; Request codes 4 and 5 are used for writing a word to code/data space.
;
PTRACE04:                                       ; Same as PTRACE05
PTRACE05:
                CALL    PTRACE_ADDR             ; Check address
                .386P
                SLDT    DI                      ; Save LDT
                LLDT    [BX].P_LDT              ; Get LDT of child process
                MOV     ES, [BX].P_DS
                MOV     ES:[ESI], ECX           ; Write the word
                LLDT    DI                      ; Note 2
                MOV     I_EAX, ECX              ; and return it
                RET
                .386

;
; Request code 8 is used for terminating a process.
;
PTRACE08:       CALL    REMOVE_PROCESS          ; Kill child process
                RET

;
; Request code 9 is used for single stepping a process.
;
; Note: ECX (data) is passed to PTRACE_SWITCH
;
PTRACE09:       CALL    PTRACE_SWITCH           ; Switch to child process
                JC      SYS08_EINVAL            ; Error ->
                CALL    DEBUG_STEP              ; Set EFLAGS for stepping
                RET


;
; Request code 7 is used for resuming a process.
;
; Note: ECX (data) is passed to PTRACE_SWITCH
;
PTRACE07:       CALL    PTRACE_SWITCH           ; Switch to child process
                JC      SYS08_EINVAL            ; Error ->
                CALL    DEBUG_RESUME            ; Set EFLAGS for resuming
                CALL    BREAK_AFTER_IRET        ; For debugging debuggers
                RET

                ASSUME  BX:NOTHING

;
; Switch to the program being debugged.
;
; In:   BX      Pointer to process table entry of child process
;       ECX     4th argument (DATA) of ptrace(): signal number
;       SS:BP   Interrupt stack frame
;
; Out:  CY      Error (invalid signal number)
;
                ASSUME  BX:PTR PROCESS
PTRACE_SWITCH   PROC    NEAR
                MOV     [BX].P_SIG_PENDING, 0   ; Cancel pending signals
                JECXZ   NOSIG                   ; No signal -> skip
                CMP     ECX, SIGNALS            ; Valid signal number?
                JAE     SHORT FAIL              ; No  -> failure
                CMP     SIG_VALID[EAX], FALSE
                JE      SHORT FAIL              ; No  -> failure
                BTS     [BX].P_SIG_PENDING, ECX ; Generate signal
NOSIG:          PUSH    BX                      ; Child process table entry
                MOV     BX, PROCESS_PTR
                MOV     (PROCESS PTR [BX]).P_STATUS, PS_WAIT_PTRACE
                CALL    SAVE_PROCESS            ; Save current process
                POP     BX
                MOV     PROCESS_PTR, BX         ; Switch to child process
                MOV     PROCESS_SIG, BX
                CALL    REST_PROCESS
                CLC                             ; No error
                RET

FAIL:           STC                             ; Error
                RET
PTRACE_SWITCH   ENDP
                ASSUME  BX:NOTHING

;
; Check address
;
; In:   DS:BX   Pointer to process table entry
;       ESI     Address
;
                ASSUME  BX:PTR PROCESS
PTRACE_ADDR     PROC    NEAR
                PUSH    EAX
                PUSH    EDX
                MOV     EAX, ESI
                ADD     EAX, 3                  ; Last byte of word
                JC      SHORT PTRACE_ADDR_ERR   ; Beyond end -> error
;
; Try code area: [BX].P_CODE_OFF ... [BX].P_CODE_OFF + [BX].P_CODE_SIZE - 1
;
                MOV     EDX, [BX].P_TEXT_OFF
                CMP     ESI, EDX
                JB      SHORT PTRACE_ADDR_1     ; Not in code area ->
                ADD     EDX, [BX].P_TEXT_SIZE
                CMP     EAX, EDX
                JBE     SHORT PTRACE_ADDR_OK    ; In code area ->
;
; Try data area: [BX].P_DATA_OFF ... [BX].P_BRK - 1
;
PTRACE_ADDR_1:  CMP     ESI, [BX].P_DATA_OFF
                JB      SHORT PTRACE_ADDR_2     ; Not in data area ->
                CMP     EAX, [BX].P_BRK
                JB      SHORT PTRACE_ADDR_OK    ; In data area ->
;
; Try stack: [BX].P_STACK_ADDR - [BX].P_STACK_SIZE ... [BX].P_STACK_ADDR - 1
;
PTRACE_ADDR_2:  MOV     EDX, [BX].P_STACK_ADDR
                CMP     EAX, EDX
                JAE     SHORT PTRACE_ADDR_3     ; Not in stack ->
                SUB     EDX, [BX].P_STACK_SIZE
                CMP     ESI, EDX
                JAE     SHORT PTRACE_ADDR_OK
PTRACE_ADDR_3:
PTRACE_ADDR_ERR:POP     EDX                     ; Restore registers
                POP     EAX
                ADD     SP, 2                   ; Remove return address
                JMP     SYS08_IO_ERR

PTRACE_ADDR_OK: POP     EDX
                POP     EAX
                RET
                ASSUME  BX:NOTHING
PTRACE_ADDR     ENDP

;
;
;
                TALIGN  2
USER_ADDR       PROC    NEAR
                LEA     SI, PTRACE_REGS
                ASSUME  SI:PTR REG_TABLE
USER_ADDR_1:    CMP     [SI].RT_ADDR, -1        ; End of table?
                JE      USER_ADDR_ERR
                CMP     EAX, [SI].RT_ADDR
                JE      SHORT USER_ADDR_OK
                ADD     SI, SIZE REG_TABLE
                JMP     SHORT USER_ADDR_1
USER_ADDR_ERR:  CMP     [SI].RT_ADDR, 0         ; Set NZ
USER_ADDR_OK:   RET
                ASSUME  SI:NOTHING
USER_ADDR       ENDP


SV_CODE         ENDS

                END
