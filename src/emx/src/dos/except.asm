;
; EXCEPT.ASM -- Handle excetions
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
                INCLUDE SWAPPER.INC
                INCLUDE DEBUG.INC
                INCLUDE VPRINT.INC
                INCLUDE OPRINT.INC
                INCLUDE PMIO.INC
                INCLUDE PMINT.INC
                INCLUDE SIGNAL.INC
                INCLUDE PROCESS.INC
                INCLUDE CORE.INC
                INCLUDE TABLES.INC
                INCLUDE MISC.INC

                PUBLIC  EXC_TAB, STDOUT_DUMP
                PUBLIC  EXCEPT_FAKE, EXCEPT_RET, EXCEPT_TASK, EXCEPT_NAME


SER_FLAG        =       FALSE                   ; Don't use serial interface
DEBUG_PTRACE    =       FALSE

SV_DATA         SEGMENT

EXCEPT2         EQU     NMI

EXC_TAB         LABEL   WORD
                IRP     X,<0,1,2,3,4,5,6,7,9,11,12,13,14,15,16,17>
                DW      X, EXCEPT&X
                ENDM
                DW      0FFFFH, 0FFFFH

$EXCEPTION      DB      "Exception ",0
$ERRCODE        DB      "ERRCODE=",0
$CS             DB      "CS=", 0
$EIP            DB      "  EIP=", 0
$SS             DB      "SS=", 0
$ESP            DB      "  ESP=", 0
$EFLAGS         DB      "EFLAGS=", 0
$CR2            DB      "CR2=", 0

$DS             DB      "DS=", 0
$ES             DB      "ES=", 0
$FS             DB      "FS=", 0
$GS             DB      "GS=", 0
$EAX            DB      "EAX=", 0
$EBX            DB      "EBX=", 0
$ECX            DB      "ECX=", 0
$EDX            DB      "EDX=", 0
$ESI            DB      "ESI=", 0
$EDI            DB      "EDI=", 0
$EBP            DB      "EBP=", 0

$EXCEPT0        DB      "divide error", 0
$EXCEPT1        DB      "debug exception", 0
$EXCEPT2        DB      "nonmaskable interrupt", 0
$EXCEPT3        DB      "breakpoint", 0
$EXCEPT4        DB      "overflow", 0
$EXCEPT5        DB      "bounds check", 0
$EXCEPT6        DB      "invalid opcode", 0
$EXCEPT7        DB      "coprocessor not available", 0
$EXCEPT8        DB      "double fault", 0
$RESERVED       DB      "reserved", 0
$EXCEPT10       DB      "invalid TSS", 0
$EXCEPT11       DB      "segment not present", 0
$EXCEPT12       DB      "stack exception", 0
$EXCEPT13       DB      "general protection", 0
$EXCEPT14       DB      "page fault", 0
$EXCEPT15       DB      "reserved", 0
$EXCEPT16       DB      "coprocessor error", 0
$EXCEPT17       DB      "alignment check", 0

EXCEPT_TEXTS    DW      $EXCEPT0, $EXCEPT1, $EXCEPT2, $EXCEPT3
                DW      $EXCEPT4, $EXCEPT5, $EXCEPT6, $EXCEPT7
                DW      $EXCEPT8, $RESERVED, $EXCEPT10, $EXCEPT11
                DW      $EXCEPT12, $EXCEPT13, $EXCEPT14, $EXCEPT15
                DW      $EXCEPT16, $EXCEPT17

REG_16          EQU     10000H
REG_NL          EQU     20000H

REG_TAB         LABEL   DWORD
                DD      OFFSET $DS,   6 OR REG_16
                DD      OFFSET $ES,   4 OR REG_16
                DD      OFFSET $FS,   2 OR REG_16
                DD      OFFSET $GS,   0 OR REG_16 OR REG_NL
                DD      OFFSET $EAX, 36
                DD      OFFSET $EBX, 24
                DD      OFFSET $ECX, 32
                DD      OFFSET $EDX, 28 OR REG_NL
                DD      OFFSET $ESI, 12
                DD      OFFSET $EDI,  8
                DD      OFFSET $EBP, 16
                DD      0, 0

;
; Table for converting exception numbers to signal numbers
;
EXC_TO_SIG_TAB  DB      SIGILL          ;  0 Unused
                DB      SIGTRAP         ;  1 Debug
                DB      SIGBUS          ;  2 NMI
                DB      SIGTRAP         ;  3 Breakpoint
                DB      SIGFPE          ;  4 Overflow
                DB      SIGFPE          ;  5 BOUND
                DB      SIGILL          ;  6 Invalid instruction
                DB      SIGFPE          ;  7 Numeric coprocessor not available
                DB      SIGSEGV         ;  8 Double fault
                DB      SIGSEGV         ;  9 Coproc. operand seg. limit viol.
                DB      SIGILL          ; 10 Invalid task state segment
                DB      SIGSEGV         ; 11 Segment not present
                DB      SIGSEGV         ; 12 Stack fault
                DB      SIGSEGV         ; 13 General protection
                DB      SIGSEGV         ; 14 Page fault
                DB      SIGBUS          ; 15 Unused
                DB      SIGFPE          ; 16 Numeric coprocessor error
                DB      SIGSEGV         ; 17 Alignment check (486)

;
; Send register dump of exception to stdout (not to CON) if NOT FALSE
;
STDOUT_DUMP     DB      FALSE

SV_DATA         ENDS


SV_CODE         SEGMENT

                ASSUME  CS:SV_CODE, DS:NOTHING

                .386P

                ASSUME  DS:NOTHING


                IRP     X,<0,1,3,4,5,6,7,9,11,12,13,14,15,16,17>
                TALIGN  2
EXCEPT&X:
              IF &X LE 7 OR &X EQ 15 OR &X EQ 16 ; Assume EX 17 w/errcd
                PUSH   DWORD PTR 0              ; Mock error code
              ENDIF
                PUSHAD                          ; Save the general registers
                MOV     AL, X                   ; Exception number to AL
                JMP     EXCEPT                  ; Common code
                ENDM

;
; Common code for all exceptions
;
; This code is called via an interrupt gate, therefore interrupts are
; disabled.
;


                ASSUME  DS:NOTHING
                ASSUME  BP:PTR ISTACKFRAME
                TALIGN  4
EXCEPT:         PUSH    DS                      ; Save segment registers
                PUSH    ES
                PUSH    FS
                PUSH    GS
              IF SER_FLAG
                PUSH    AX
                SERIAL  "E"
                POP     AX
              ENDIF
                MOV     EBP, ESP                ; Setup stack frame
                SUB     ESP, FRAME_SIZE         ; Local variables
                MOV     BX, G_SV_DATA_SEL
                MOV     DS, BX                  ; Make data segment accessible
                ASSUME  DS:SV_DATA
                MOV     EXCEPT_NO, AL           ; Save exception number
                CLD
;
; Check for debugging
;
              IF DEBUG_PTRACE
                CMP     AL, 1                   ; Debug exception?
                JE      SHORT EXCEPT_4          ; Yes -> built-in debugger
              ENDIF
;
; Check for page faults
;
                CMP     AL, 14                  ; Page fault?
                JNE     SHORT EXCEPT_1
                CALL    PAGE_FAULT              ; Yes -> try to handle this
;
; Check for floating-point exceptions
;
EXCEPT_1:     IF FLOATING_POINT
                CMP     EXCEPT_NO, 7            ; Coprocessor not available?
                JNE     SHORT EXCEPT_2          ; No  -> skip
                CALL    EXCEPT_DNA              ; Task switch / emulator
              ENDIF
;
; Check for return from signal handler
;
EXCEPT_2:       CMP     EXCEPT_NO, 13           ; General protection violation?
                JNE     SHORT EXCEPT_3          ; No  -> skip
                MOV     EAX, I_ERRCD            ; Get error code
                OR      AX, AX                  ; Tried to load a selector?
                JNE     SHORT EXCEPT_3          ; Yes -> skip
                MOVZX   EAX, I_CS               ; Get CS
                LSL     EAX, EAX                ; Get code segment limit
                JNZ     SHORT EXCEPT_3          ; Avoid another exception
                MOV     ESI, I_EIP              ; Put CS:EIP into ES:ESI
                MOV     ES, I_CS                ; NB: All segments are readable
                CMP     ESI, EAX                ; EIP beyond limit?
                JA      SHORT EXCEPT_3          ; Yes -> avoid another exception
                CMP     BYTE PTR ES:[ESI], 0C3H ; Pointing to RETN?
                JNE     SHORT EXCEPT_3          ; No  -> skip
                MOVZX   EAX, I_SS               ; Get SS
                LSL     EAX, EAX                ; Get stack segment limit
                JNZ     SHORT EXCEPT_3          ; Avoid another exception
                SUB     EAX, size SIGFRAME - 1  ; We examine a SIGFRAME
                JC      SHORT EXCEPT_3          ; Cannot happen
                MOV     ESI, I_ESP              ; Put SS:ESP into ES:ESI
                ASSUME  ESI:NEAR32 PTR SIGFRAME
                MOV     ES, I_SS                ; NB: Expand=Up
                CMP     ESI, EAX                ; ESP beyond limit?
                JA      SHORT EXCEPT_3          ; Yes -> avoid another exception
                CMP     ES:[ESI].SF_EIP, SIG_EIP ; Our special EIP?
                JNE     SHORT EXCEPT_3          ; No  -> skip
                CMP     ES:[ESI].SF_SIGNATURE, SIG_SIGNATURE ; Signature?
                JNE     SHORT EXCEPT_3          ; No  -> skip
                CALL    SIG_RETURN              ; Return from signal handler
                ASSUME  ESI:NOTHING
                JMP     EXCEPT_12               ; Signal not handled

;
; This entry point is used for SIGFPE (see PMINT.ASM).
;
EXCEPT_FAKE:    MOV     EXCEPT_NO, AL

;
; Check for ptraced process
;
EXCEPT_3:       TEST    I_CS, 3                 ; In supervisor?
                JZ      SHORT EXCEPT_4          ; Yes -> skip
                MOV     BX, PROCESS_PTR         ; Current process
                CMP     BX, NO_PROCESS          ; No process?
                JE      SHORT EXCEPT_4          ; Yes -> skip (cannot happen)
                TEST    (PROCESS PTR [BX]).P_FLAGS, PF_DEBUG
                JZ      SHORT EXCEPT_4          ; Process not in debug mode ->
                CALL    EXCEPT_PTRACE           ; Return to ptracing process
EXCEPT_4:       CMP     EXCEPT_NO, 1            ; Debug exception?
                JNE     SHORT EXCEPT_5          ; No  -> skip
                CALL    DEBUG_EXCEPTION
                JMP     SHORT EXCEPT_10

;
; Give the debugger a chance
;
EXCEPT_5:       CMP     STEP_FLAG, FALSE        ; -S option?
                JE      SHORT EXCEPT_6
                CALL    DEBUG_EXCEPTION
;
; Check for exceptions to be mapped to signals
;
EXCEPT_6:       TEST    I_CS, 3                 ; Called from user code?
                JZ      SHORT EXCEPT_10         ; No  -> skip
                MOV     BX, PROCESS_PTR         ; Current process
                CMP     BX, NO_PROCESS          ; No process?
                JE      SHORT EXCEPT_10         ; Yes -> skip
                ASSUME  BX:PTR PROCESS
                MOV     AL, EXCEPT_NO           ; Compute signal number
                CALL    EXC_TO_SIG              ; from exception number
                MOV     EDX, EAX                ; EDX := signal number
;
; Deliver the signal if a signal handler is set.  Deliver the
; signal even if it is blocked.
;
                MOV     SI, DX
                SHL     SI, 2
                MOV     EAX, [BX].P_SIG_HANDLERS[SI] ; Get address
                CMP     EAX, SIG_IGN            ; Ignore signal
                JE      SHORT EXCEPT_10         ; Yes -> skip
                CMP     EAX, SIG_DFL            ; Default processing?
                JE      SHORT EXCEPT_10         ; Yes -> skip
                CALL    EXCEPT_SIGNAL           ; Deliver the signal
                ASSUME  BX:NOTHING
;
; Display register dump if appropriate
;
EXCEPT_10:      CMP     I_CS, L_CODE_SEL        ; In user code?
                JE      SHORT EXCEPT_11         ; Yes -> don't show registers
                CALL    EX_DUMP
EXCEPT_11:      CMP     I_CS, L_CODE_SEL        ; In user code?
                JNE     SHORT STOP              ; No  -> loop forever
;
; Display message.  Dump core unless disabled
;
                MOV     AL, EXCEPT_NO
                CALL    EXC_TO_SIG
EXCEPT_12:      CALL    SIG_MSG                 ; Display "Stopped by SIG..."
                CMP     EAX, SIGNALS            ; Valid signal number?
                JAE     SHORT EXCEPT_13         ; No  -> dump core
                CMP     SIG_CORE[EAX], FALSE    ; Dump core for this signal?
                JE      SHORT NOCORE            ; No  -> skip
EXCEPT_13:      MOV     BX, PROCESS_PTR         ; Current process
                CMP     BX, NO_PROCESS          ; No process?
                JE      SHORT NOCORE            ; Yes -> skip
                TEST    (PROCESS PTR [BX]).P_FLAGS, PF_NO_CORE
                JNZ     SHORT NOCORE            ; Core dumps disabled -> skip
                CALL    CORE_REGS_I
                CALL    CORE_DUMP
NOCORE:         MOV     AX, 4CFFH
                INT     21H

STOP:           JMP     STOP

;
; Return from exception handler.  This code is either CALLed from the
; exception handler or JuMPed to by a procedure CALLed by the exception
; handler.
;
                TALIGN  2
EXCEPT_RET:     ADD     ESP, 2 + FRAME_SIZE     ; Remove return address and
                                                ; local data
;
; Deliver pending signals.  This is required for delivering signals
; on return from a signal handler.
;
                CALL    SIG_DELIVER_PENDING
                POP     GS
                POP     FS
                POP     ES
                POP     DS
                POPAD
                NOP                             ; Avoid 386 bug
                ADD     ESP, 4                  ; Skip error code
                IRETD


;
; Switch back to the process that issued ptrace(P_STEP)
;
; In:   BX      PROCESS_PTR
;
                ASSUME  DS:SV_DATA
                ASSUME  BX:PTR PROCESS
EXCEPT_PTRACE:  MOV     [BX].P_STATUS, PS_STOP  ; Process stopped
                OR      [BX].P_FLAGS, PF_WAIT_WAIT
                MOV     AL, EXCEPT_NO           ; Get exception number
                CALL    EXC_TO_SIG              ; Convert to signal number
                MOV     [BX].P_SIG_NO, AL       ; and store it
                PUSH    EAX                     ; Save it for ptrace result
                CALL    SAVE_PROCESS            ; Save outgoing process
                MOV     EAX, [BX].P_PPID        ; Parent process ID
                CALL    FIND_PROCESS            ; Find table entry
                POP     EAX                     ; Signal number
                CMP     BX, NO_PROCESS          ; Not found?
                JE      SHORT EXCEPT_PTRACE_1   ; Should not happen
                MOV     PROCESS_PTR, BX         ; Switch to parent process
                MOV     PROCESS_SIG, BX
                MOV     CX, [BX].P_STATUS       ; Get process status
                CALL    REST_PROCESS            ; Restore incoming process
                CMP     CX, PS_WAIT_PTRACE      ; Has been waiting in ptrace?
                JNZ     SHORT EXCEPT_PTRACE_1   ; No -> should not happen
                MOV     I_EAX, EAX              ; Return signal number
                MOV     I_ECX, 0                ; No error
                CALL    BREAK_AFTER_IRET        ; For built-in debugger
                JMP     EXCEPT_RET              ; Return to ptrace
EXCEPT_PTRACE_1:RET                             ; Continue exception handler
                ASSUME  BX:NOTHING
                ASSUME  BP:NOTHING


                ASSUME  DS:SV_DATA
                ASSUME  BP:PTR ISTACKFRAME
EX_DUMP         PROC    NEAR
                CMP     I_CS, L_CODE_SEL
                JE      SHORT EX_DUMP3
                MOV     VOUTPUT, NOT FALSE
                MOV     ETEXT, OFFSET SV_CODE:VTEXT
                MOV     EBYTE, OFFSET SV_CODE:VBYTE
                MOV     EWORD, OFFSET SV_CODE:VWORD
                MOV     EDWORD, OFFSET SV_CODE:VDWORD
                MOV     ECRLF, OFFSET SV_CODE:VCRLF
                MOV     ECHAR, OFFSET SV_CODE:VCHAR
                MOV     AX, G_VIDEO_SEL
                MOV     ES, AX
                CALL    VCLS
                JMP     SHORT EX_DUMP5

EX_DUMP3:       MOV     VOUTPUT, FALSE
                MOV     ETEXT, OFFSET SV_CODE:OTEXT
                MOV     EBYTE, OFFSET SV_CODE:OBYTE
                MOV     EWORD, OFFSET SV_CODE:OWORD
                MOV     EDWORD, OFFSET SV_CODE:ODWORD
                MOV     ECRLF, OFFSET SV_CODE:OCRLF
                MOV     ECHAR, OFFSET SV_CODE:OCHAR
                CMP     STDOUT_DUMP, FALSE
                JNE     SHORT EX_DUMP4
                CALL    FORCE_STDOUT
EX_DUMP4:       CALL    OCRLF
EX_DUMP5:       LEA     EDX, $EXCEPTION
                CALL    ETEXT
                MOV     AL, EXCEPT_NO
                CALL    EBYTE
                CMP     AL, 17
                JA      SHORT EX_DUMP6
                MOV     AL, ":"
                CALL    ECHAR
                MOV     AL, " "
                CALL    ECHAR
                MOV     AL, EXCEPT_NO
                CALL    EXCEPT_NAME
                CALL    ETEXT
EX_DUMP6:       CALL    ECRLF
                MOV     AL, EXCEPT_NO
                CMP     AL, 8
                JB      SHORT NO_ERRCODE
                CMP     AL, 14
                JA      SHORT NO_ERRCODE
                LEA     EDX, $ERRCODE
                CALL    ETEXT
                MOV     EAX, I_ERRCD
                CALL    EDWORD
                CALL    ECRLF
NO_ERRCODE:     LEA     EDX, $CS
                CALL    ETEXT
                MOV     AX, I_CS
                CALL    EWORD
                LEA     EDX, $EIP
                CALL    ETEXT
                MOV     EAX, I_EIP
                CALL    EDWORD
                CALL    ECRLF
;
; Display SS:ESP
;
                MOV     SI, I_SS
                MOV     EDI, I_ESP
                TEST    I_CS, 3                 ; RPL = 0?
                JNZ     SHORT EXD_SS_ESP_1      ; No -> use values from stack
;
; For RPL=0 the stack has not been switched
;
                MOV     SI, SS
                LEA     EDI, [EBP+56]
EXD_SS_ESP_1:   LEA     EDX, $SS
                CALL    ETEXT
                MOV     AX, SI
                CALL    EWORD
                LEA     EDX, $ESP
                CALL    ETEXT
                MOV     EAX, EDI
                CALL    EDWORD
                CALL    ECRLF

                LEA     EDX, $EFLAGS
                CALL    ETEXT
                MOV     EAX, I_EFLAGS
                CALL    EDWORD
                CALL    ECRLF
                CMP     EXCEPT_NO, 14
                JNE     SHORT EX_NO_CR2
                LEA     EDX, $CR2
                CALL    ETEXT
                MOV     EAX, CR2
                CALL    EDWORD
                CALL    ECRLF
EX_NO_CR2:      LEA     EDI, REG_TAB
REG_DUMP_1:     MOV     EDX, [EDI+0]
                OR      EDX, EDX
                JZ      SHORT REG_DUMP_END
                CALL    ETEXT
                MOV     EAX, [EDI+4]
                TEST    EAX, REG_16
                JNZ     SHORT REG_DUMP_16
                AND     EAX, 0FFFFH
                MOV     EAX, SS:[EBP+EAX]
                CALL    EDWORD
                JMP     SHORT REG_DUMP_2
REG_DUMP_16:    AND     EAX, 0FFFFH
                MOV     AX, SS:[EBP+EAX]
                CALL    EWORD
REG_DUMP_2:     TEST    DWORD PTR [EDI+4], REG_NL
                JZ      SHORT REG_DUMP_3
                CALL    ECRLF
                JMP     SHORT REG_DUMP_9
REG_DUMP_3:     MOV     AL, TAB
                CALL    ECHAR
REG_DUMP_9:     ADD     EDI, 8
                JMP     SHORT REG_DUMP_1

REG_DUMP_END:   CALL    ECRLF
                LEA     ESI, I_EFLAGS + 4
                TEST    I_CS, 3                 ; RPL of return CS
                JZ      SHORT SAME_LEVEL_2      ; Called from supervisor
                ADD     ESI, 8                  ; Skip ESP and SS
SAME_LEVEL_2:   XOR     EAX, EAX
                MOV     AX, SS
                MOV     FS, AX
                LSL     ECX, EAX
                INC     ECX
                SUB     ECX, ESI
                JBE     SHORT NOSDUMP
                SHR     ECX, 1
                CMP     ECX, 128
                JBE     SHORT SDUMP1
                MOV     ECX, 128
SDUMP1:         CALL    MEM_DUMP                ; (FS, ESI, ECX)
NOSDUMP:        RET
                ASSUME  BP:NOTHING
EX_DUMP         ENDP


;
; Raise a signal from exception handler
;
; Note that this must be a procedure to get the stack right at EXCEPT_RET.
;
; In:  EAX      Address of the signal handler
;      EDX      Signal number
;      BX       Pointer to process table entry
;
EXCEPT_SIGNAL   PROC    NEAR
                MOV     ECX, NOT FALSE          ; Trap
                CALL    SIG_USER_CALL           ; Call user signal handler
                JMP     EXCEPT_RET              ; (This enables interrupts)
EXCEPT_SIGNAL   ENDP

;
; Numeric coprocessor not available (DNA=device not available)
;
; Handle task switching and emulation
;
              IF FLOATING_POINT

                ASSUME  DS:SV_DATA
                ASSUME  BP:PTR ISTACKFRAME
EXCEPT_DNA      PROC    NEAR
                CMP     FP_FLAG, FP_387         ; 387 coprocessor present?
                JNE     SHORT EDNA_EMU          ; No -> emulate
                CLTS                            ; Clear TS bit of CR0
                MOV     BX, PROCESS_FP          ; Last process using FPU
                CMP     BX, PROCESS_PTR         ; Same process?
                JE      SHORT EDNA_NO_REST      ; Yes -> do nothing
                CMP     BX, NO_PROCESS          ; Save status?
                JE      SHORT EDNA_NO_SAVE      ; No  -> skip
                ASSUME  BX:PTR PROCESS
                DB      66H
                FNSAVE  [BX].P_CW               ; Save FPU context, initialize
                FWAIT
EDNA_NO_SAVE:   MOV     BX, PROCESS_PTR         ; Current process
                MOV     PROCESS_FP, BX          ; FPU switched to new process
                CMP     BX, NO_PROCESS          ; Any process?
                JE      SHORT EDNA_SV           ; No  -> in supervisor
                TEST    [BX].P_FLAGS, PF_387_USED ; Initialize?
                JNZ     SHORT EDNA_REST         ; No  -> restore
                CALL    SETUP_FP                ; Initialize 387
                OR      [BX].P_FLAGS, PF_387_USED ; 387 initialized
                JMP     SHORT EDNA_NO_REST      ; Don't restore

EDNA_SV:        CALL    SETUP_FP                ; Initialize 387
                JMP     SHORT EDNA_NO_REST      ; Don't restore
;
; Note: 32-bit mode? 287?
;
EDNA_REST:
                DB      66H
                FRSTOR  [BX].P_CW
EDNA_NO_REST:   JMP     EXCEPT_RET              ; Return to fp instruction

;
; Call coprocessor emulator.
;
; If there is no coprocess emulator, the untyped call and return
; constructs of GCC 2.6.0 use the FNSAVE and FRSTOR instructions
; even if no floating point code is used by the application.
; Therefore, we silently ignore FNSAVE and FRSTOR.
;
; However, GCC 2.6.1 generates more complicated code which I don't
; want to handle here.  That doesn't matter as we have a real FPU
; emulator now.
;
                TALIGN  4
EDNA_EMU:       TEST    I_CS, 3                 ; In supervisor?
                JZ      SHORT EDNA_EMU_RET      ; Yes -> skip
                CMP     FPUEMU_STATE, FES_ON
                JE      SHORT EDNA_EMU_CALL
                CMP     FPUEMU_STATE, FES_OFF
                JNE     SHORT EDNA_EMU_NO
                CALL    FPUEMU_LOAD             ; Try to load a FPU emulator
                CMP     FPUEMU_STATE, FES_LOADING
                JNE     SHORT EDNA_EMU_NO
                JMP     EXCEPT_RET              ; Run the emulator

; TODO: Abort if FP exception in emulator
                TALIGN  4
EDNA_EMU_CALL:  CALL    FPUEMU_CALL             ; Switch to the FPU emulator
                JMP     EXCEPT_RET

                TALIGN  4
EDNA_EMU_NO:    MOV     ESI, I_EIP              ; Put CS:EIP into ES:ESI
                MOV     ES, I_CS                ; NB: All segments are readable
                CMP     BYTE PTR ES:[ESI], 0DDH ; 1st byte of FNSAVE, FRSTOR
                JNE     SHORT EDNA_EMU_RET      ; Other -> default processing
                INC     ESI
                MOV     AH, ES:[ESI]            ; Get MOD, XXX, R/M
                INC     ESI
                MOV     AL, AH
                AND     AL, 11000000B           ; Extract MOD
                CMP     AL, 11000000B           ; MOD=11?
                JE      SHORT EDNA_EMU_RET      ; Yes -> not FSAVE/FRSTOR
                MOV     AL, AH
                AND     AL, 00111000B           ; Extract XXX (opcode)
                CMP     AL, 00100000B           ; FRSTOR?
                JE      SHORT FPEMU_1           ; Yes ->
                CMP     AL, 00110000B           ; FSAVE?
                JE      SHORT FPEMU_1           ; Yes ->
EDNA_EMU_RET:   RET                             ; Default exception processing
                ASSUME  BX:NOTHING

;
; FSAVE and FRSTOR
;
FPEMU_1:        MOV     AL, AH
                AND     AL, 00000111B           ; Extract R/M
                CMP     AL, 00000100B           ; Use SIB?
                JE      SHORT FPEMU_SIB         ; Yes -> look at SIB
                MOV     AL, AH
                AND     AL, 11000000B           ; Extract MOD
                JZ      SHORT FPEMU_MOD00       ; MOD=00 ->
                CMP     AL, 01000000B           ; MOD=01?
                JE      SHORT FPEMU_MOD01       ; Yes ->
;
; MOD=10: [REG+DISP32]
;
FPEMU_MOD10:    ADD     ESI, 4                  ; Skip DISP32
                JMP     SHORT FPEMU_IGNORE

;
; MOD=01: [REG+DISP8]
;
FPEMU_MOD01:    INC     ESI                     ; Skip DISP8
                JMP     SHORT FPEMU_IGNORE

;
; MOD=00: [REG] or [DISP32]
;
FPEMU_MOD00:    MOV     AL, AH
                AND     AL, 00000111B           ; Extract R/M
                CMP     AL, 00000101B           ; EBP (meaning DISP32)?
                JNE     SHORT FPEMU_IGNORE      ; No -> no displacement
                ADD     ESI, 4                  ; Skip DISP32
                JMP     SHORT FPEMU_IGNORE

;
; R/M=100: SIB
;
FPEMU_SIB:      MOV     DL, ES:[ESI]            ; Fetch SIB
                INC     ESI
                MOV     AL, AH
                AND     AL, 11000000B           ; Extract MOD
                JZ      SHORT FPEMU_SIB_MOD00   ; MOD=00 ->
                CMP     AL, 01000000B           ; MOD=01?
                JE      SHORT FPEMU_SIB_MOD01   ; Yes ->
;
; MOD=10: [REG+SCALE*INDEX+DISP32]
;
FPEMU_SIB_MOD10:ADD     ESI, 4                  ; Skip DISP32
                JMP     SHORT FPEMU_IGNORE

;
; MOD=01: [REG+SCALE*INDEX+DISP8]
;
FPEMU_SIB_MOD01:INC     ESI                     ; Skip DISP8
                JMP     SHORT FPEMU_IGNORE

;
; MOD=00: [REG+SCALE*INDEX] or [DISP32+SCALE*INDEX]
;
FPEMU_SIB_MOD00:MOV     AL, DL
                AND     AL, 00000111B           ; Extract BASE
                CMP     AL, 00000100B           ; EBP (meaning DISP32)?
                JNE     SHORT FPEMU_IGNORE      ; No -> no displacement
                ADD     ESI, 4                  ; Skip DISP32
                JMP     SHORT FPEMU_IGNORE

;
; Continue the application at EIP:=ESI
;
FPEMU_IGNORE:   MOV     I_EIP, ESI
                JMP     EXCEPT_RET


;
; Initialize 387 for new process
;
SETUP_FP        PROC    NEAR
                FNINIT
                FSTCW   FP_TMP
                FWAIT
                OR      FP_TMP, 3FH             ; Use default error handlers
                FLDCW   FP_TMP
                RET
SETUP_FP        ENDP

                ASSUME  BP:NOTHING
EXCEPT_DNA      ENDP

              ENDIF

;
; Convert exception number to signal number
;
; In:   AL      Exception number (0..17, not checked!)
;
; Out:  EAX     Signal number
;
                ASSUME  DS:SV_DATA
EXC_TO_SIG      PROC    NEAR
                PUSH    BX
                LEA     BX, EXC_TO_SIG_TAB
                XLAT    EXC_TO_SIG_TAB
                MOVZX   EAX, AL
                POP     BX
                RET
EXC_TO_SIG      ENDP

;
; Memory (stack) dump
;
; In:   FS:ESI  Source
;       CX      Word count
;       ES:BX   Destination
;
                ASSUME  DS:NOTHING
MEM_DUMP        PROC    NEAR
                PUSH    EAX
                PUSH    ESI
                JCXZ    MDUMP9
MDUMP1:         CMP     VOUTPUT, FALSE
                JE      SHORT MDUMP2
                MOV     AX, 5
                CALL    VWRAP
                CALL    VGETCOL
MDUMP2:         MOV     AX, FS:[ESI]
                CALL    EWORD
                DEC     CX
                JZ      SHORT MDUMP9
                MOV     AL, " "
                CALL    ECHAR
                ADD     ESI, 2
                JMP     MDUMP1
MDUMP9:         POP     ESI
                POP     EAX
                RET
MEM_DUMP        ENDP


;
; Task for exceptions 8 (double fault) and 10 (invalid TSS)
;
                ASSUME  DS:SV_DATA              ; Loaded from TSS
EXCEPT_TASK:    CLI                             ; Not required
              IF SER_FLAG
                SERIAL  "T"
              ENDIF
                PUSH    AX                      ; Exception number
                MOV     AX, G_VIDEO_SEL
                MOV     ES, AX
                CALL    VCLS
                LEA     EDX, $EXCEPTION
                CALL    VTEXT
                POP     AX                      ; Loaded from TSS
                CALL    VBYTE
                PUSH    AX
                MOV     AL, ":"
                CALL    VCHAR
                MOV     AL, " "
                CALL    VCHAR
                POP     AX
                CALL    EXCEPT_NAME
                CALL    VTEXT
                CALL    VCRLF
                LEA     EDX, $CS
                CALL    VTEXT
                MOV     AX, G_TSS_MEM_SEL
                MOV     FS, AX
                MOV     AX, (TSS_STRUC PTR FS:[0]).TSS_CS
                CALL    VWORD
                LEA     EDX, $EIP
                CALL    VTEXT
                MOV     EAX, (TSS_STRUC PTR FS:[0]).TSS_EIP
                CALL    VDWORD
                CALL    VCRLF
                JMP     $
                ASSUME  ESI:NOTHING

;
; Return name of exception
;
; In:   AL      Exception number
;
; Out:  EDX     Text
;
EXCEPT_NAME     PROC    NEAR
                LEA     EDX, $RESERVED
                CMP     AL, 17
                JA      SHORT ENAME1
                PUSH    BX
                MOV     BL, AL
                XOR     BH, BH
                SHL     BX, 1
                MOV     DX, EXCEPT_TEXTS[BX]    ; Keep upper word of EDX
                POP     BX
ENAME1:         RET
EXCEPT_NAME     ENDP

SV_CODE         ENDS

                END
