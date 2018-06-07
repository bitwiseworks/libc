;
; SIGNAL.ASM -- Signal processing
;
; Copyright (c) 1991-1998 by Eberhard Mattes
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

__SIGNAL        =       1
                INCLUDE EMX.INC
                INCLUDE EXCEPT.INC
                INCLUDE PMINT.INC
                INCLUDE RMINT.INC
                INCLUDE MISC.INC
                INCLUDE SIGNAL.INC
                INCLUDE PROCESS.INC
                INCLUDE CORE.INC
                INCLUDE OPRINT.INC
                INCLUDE ERRORS.INC

                PUBLIC  SIG_DELIVER_PENDING, SIG_USER_CALL, SIG_RETURN, SIG_MSG
                PUBLIC  CRIT_ERR_FLAG, SIG_CORE, SIG_VALID, EMERGENCY_TIMER
                PUBLIC  EMERGENCY_FLAG
                PUBLIC  HOOK_INT
                PUBLIC  CLEANUP_SIGNAL

LSTACKS         =       2                       ; Number of local stacks
LSTACK_SIZE     =       512                     ; Size of a local stack (bytes)

LSTACK          SEGMENT
                DB      LSTACKS DUP (LSTACK_SIZE DUP (?))
LSTACK          ENDS


SV_DATA         SEGMENT

;
; Original interrupt 1BH vector
;
OLD_BREAK       LABEL   DWORD
OLD_BREAK_OFF   DW      0
OLD_BREAK_SEG   DW      0

;
; This flag is set when cleaning up while executing the critical
; error handler
;
CRIT_ERR_FLAG   DB      FALSE

;
; Timer and counter for emergency exit.  EMERGENCY_TIMER set to 18 if
; it is zero when Ctrl-Break is hit.  Each timer tick decrements
; EMERGENCY_TIMER.  If Ctrl-Break is hit four times while EMERGENCY_TIMER
; is non-zero, EMERGENCY_FLAG will be set to make emx abort.
; EMERGENCY_COUNTER is used for counting Ctrl-Break interrupts after
; setting EMERGENCY_TIMER.
;
; This implementation is simple but does not work correctly if Ctrl-Break
; is hit less than five times in a second, followed by more Ctrl-Break
; interrupts in the next second.  A perfect solution involves keeping
; time stamps for the last 4 Ctrl-Break interrupts.
;

EMERGENCY_TIMER   DB    0
EMERGENCY_COUNTER DB    0
EMERGENCY_FLAG    DB    FALSE

;
; Don't use RTEXT, as only function calls 01H..0CH, 59H are allowed.
;
$ABORTED        DB      "Program aborted", CR, LF, "$"
$STACKS         DB      "Out of stacks", CR, LF, "$"

$STOPPED_BY     BYTE    CR, LF, "Process terminated by ", 0
$ABNORMAL       BYTE    CR, LF, "Abnormal program termination", CR, LF, 0

$SIG_UNKNOWN    BYTE    "unknown signal", 0

$SIGHUP         BYTE    "SIGHUP", 0
$SIGINT         BYTE    "SIGINT", 0
$SIGQUIT        BYTE    "SIGQUIT", 0
$SIGILL         BYTE    "SIGILL", 0
$SIGTRAP        BYTE    "SIGTRAP", 0
$SIGABRT        BYTE    "SIGABRT", 0
$SIGEMT         BYTE    "SIGEMT", 0
$SIGFPE         BYTE    "SIGFPE", 0
$SIGKILL        BYTE    "SIGKILL", 0
$SIGBUS         BYTE    "SIGBUS", 0
$SIGSEGV        BYTE    "SIGSEGV", 0
$SIGSYS         BYTE    "SIGSYS", 0
$SIGPIPE        BYTE    "SIGPIPE", 0
$SIGALRM        BYTE    "SIGALRM", 0
$SIGTERM        BYTE    "SIGTERM", 0
$SIGUSR1        BYTE    "SIGUSR1", 0
$SIGUSR2        BYTE    "SIGUSR2", 0
$SIGCLD         BYTE    "SIGCLD", 0
$SIGBREAK       BYTE    "SIGBREAK", 0
$SIGWINCH       BYTE    "SIGWINCH", 0

                TALIGN  2
SIG_NAMES       WORD    $SIG_UNKNOWN, $SIGHUP, $SIGINT, $SIGQUIT    ; 0..3
                WORD    $SIGILL, $SIGTRAP, $SIGABRT, $SIGEMT        ; 4..7
                WORD    $SIGFPE, $SIGKILL, $SIGBUS, $SIGSEGV        ; 8..11
                WORD    $SIGSYS, $SIGPIPE, $SIGALRM, $SIGTERM       ; 12..15
                WORD    $SIGUSR1, $SIGUSR2, $SIGCLD                 ; 16..18
                WORD    $SIG_UNKNOWN, $SIG_UNKNOWN, $SIGBREAK       ; 19..21
                WORD    $SIG_UNKNOWN, $SIG_UNKNOWN, $SIG_UNKNOWN    ; 22..24
                WORD    $SIG_UNKNOWN, $SIG_UNKNOWN, $SIG_UNKNOWN    ; 25..27
                WORD    $SIGWINCH                                   ; 28

;
; Valid signal numbers have a non-FALSE entry in this table.
;
SIG_VALID       BYTE    FALSE,     NOT FALSE, NOT FALSE, NOT FALSE  ; 0..3
                BYTE    NOT FALSE, NOT FALSE, NOT FALSE, NOT FALSE  ; 4..7
                BYTE    NOT FALSE, NOT FALSE, NOT FALSE, NOT FALSE  ; 8..11
                BYTE    NOT FALSE, NOT FALSE, NOT FALSE, NOT FALSE  ; 12..15
                BYTE    NOT FALSE, NOT FALSE, NOT FALSE, FALSE      ; 16..19
                BYTE    FALSE,     NOT FALSE, FALSE,     FALSE      ; 20..23
                BYTE    FALSE,     FALSE,     FALSE,     FALSE      ; 24..27
                BYTE    NOT FALSE                                   ; 28

;
; If SIG_CORE[signo] is FALSE, no core dump file will be written for
; the signal signo.
;

SIG_CORE        BYTE    FALSE,     FALSE,     FALSE,     NOT FALSE  ; 0..3
                BYTE    NOT FALSE, NOT FALSE, NOT FALSE, NOT FALSE  ; 4..7
                BYTE    NOT FALSE, FALSE,     NOT FALSE, NOT FALSE  ; 8..11
                BYTE    NOT FALSE, FALSE,     FALSE,     FALSE      ; 12..15
                BYTE    FALSE,     FALSE,     FALSE,     FALSE      ; 16..19
                BYTE    FALSE,     FALSE,     FALSE,     FALSE      ; 20..23
                BYTE    FALSE,     FALSE,     FALSE,     FALSE      ; 24..27
                BYTE    FALSE                                       ; 28

;
; If SIG_DFL_ACTION[signo] is FALSE, the program will be terminated
; for signal signo if SIG_DFL is set.  If the table entry is not FALSE,
; the signal will be ignored.
;
SIG_DFL_ACTION  BYTE    FALSE,     FALSE,     FALSE,     FALSE      ; 0..3
                BYTE    FALSE,     FALSE,     FALSE,     FALSE      ; 4..7
                BYTE    FALSE,     FALSE,     FALSE,     FALSE      ; 8..11
                BYTE    FALSE,     FALSE,     FALSE,     FALSE      ; 12..15
                BYTE    NOT FALSE, NOT FALSE, NOT FALSE, FALSE      ; 16..19
                BYTE    FALSE,     FALSE,     FALSE,     FALSE      ; 20..23
                BYTE    FALSE,     FALSE,     FALSE,     FALSE      ; 24..27
                BYTE    NOT FALSE                                   ; 28

;
; If SIG_IGN_ACTION[signo] is FALSE, the program will be terminated
; for signal signo if SIG_IGN is set.  If the entry is not FALSE, the
; signal will be ignored.
;
SIG_IGN_ACTION  BYTE    FALSE,     NOT FALSE, NOT FALSE, NOT FALSE  ; 0..3
                BYTE    FALSE,     FALSE,     FALSE,     FALSE      ; 4..7
                BYTE    FALSE,     FALSE,     FALSE,     FALSE      ; 8..11
                BYTE    FALSE,     NOT FALSE, NOT FALSE, FALSE      ; 12..15
                BYTE    NOT FALSE, NOT FALSE, NOT FALSE, FALSE      ; 16..19
                BYTE    FALSE,     NOT FALSE, FALSE,     FALSE      ; 20..23
                BYTE    FALSE,     FALSE,     FALSE,     FALSE      ; 24..27
                BYTE    NOT FALSE                                   ; 28

;
; If SIG_FUN_ACTION[signo] is FALSE, the program will be terminated
; for after calling the signal handler for signal signo.  If the entry
; is not FALSE, the program will be continued.
;
SIG_FUN_ACTION  BYTE    FALSE,     NOT FALSE, NOT FALSE, NOT FALSE  ; 0..3
                BYTE    FALSE,     FALSE,     FALSE,     FALSE      ; 4..7
                BYTE    FALSE,     FALSE,     FALSE,     FALSE      ; 8..11
                BYTE    FALSE,     NOT FALSE, NOT FALSE, NOT FALSE  ; 12..15
                BYTE    NOT FALSE, NOT FALSE, NOT FALSE, FALSE      ; 16..19
                BYTE    FALSE,     NOT FALSE, FALSE,     FALSE      ; 20..23
                BYTE    FALSE,     FALSE,     FALSE,     FALSE      ; 24..27
                BYTE    NOT FALSE                                   ; 28

SV_DATA         ENDS


SV_CODE         SEGMENT

;
; Check for pending signals
;
                ASSUME  DS:SV_DATA
                ASSUME  BP:PTR ISTACKFRAME
SIG_DELIVER_PENDING PROC NEAR
                MOV     BX, PROCESS_PTR         ; Current process
                CMP     BX, NO_PROCESS          ; In user code?
                JE      SHORT SDP_RET           ; No  -> no signals
                ASSUME  BX:PTR PROCESS
;
; Check signals only if called from user code [0.8b]
;
                TEST    I_CS, 3                 ; Called from user code?
                JZ      SHORT SDP_RET           ; No  -> no signals
;
; Start of Critical section
;
                CLI
                MOV     EAX, [BX].P_SIG_BLOCKED ; Get set of pending,
                NOT     EAX                     ; unblocked signals
                AND     EAX, [BX].P_SIG_PENDING
                JZ      SHORT SDP_RET_STI       ; No signals -> just return
;
; Find a pending signal, reset its `signal pending' bit and
; fetch the signal handler
;
                BSF     EDX, EAX                ; Find first bit
                JZ      SHORT SDP_RET_STI       ; Cannot happen
                BTR     [BX].P_SIG_PENDING, EDX ; Reset `signal pending' bit
                MOV     SI, DX
                SHL     SI, 2
                MOV     EAX, [BX].P_SIG_HANDLERS[SI] ; Get address
                CMP     EAX, SIG_IGN            ; Ignore signal?
                JE      SHORT SDP_IGNORE        ; Yes -> see table
                CMP     EAX, SIG_DFL            ; Default handler?
                JNE     SDP_USER                ; No  -> call signal handler
;
; SIG_DFL
;
                STI                             ; End of critical section
                CMP     EDX, SIGTERM            ; SIGTERM?
                JE      SHORT SDP_SIGTERM       ; Yes -> terminate program
                CMP     SIG_DFL_ACTION[EDX], FALSE ; Ingore signal?
                JNE     SHORT SDP_RET           ; Yes -> ignore
                JMP     SHORT SDP_TERM          ; No  -> terminate process

SDP_RET_STI:    STI                             ; End of critical section
SDP_RET:        RET

;
; SIG_IGN
;
SDP_IGNORE:     STI                             ; End of critical section
                CMP     SIG_IGN_ACTION[EDX], FALSE ; Ignore signal?
                JNE     SHORT SDP_RET
;
; Terminate / abort process due to signal EDX
;
                ASSUME  BX:PTR PROCESS
SDP_TERM:       MOV     EAX, EDX
                CALL    SIG_MSG
                TEST    [BX].P_FLAGS, PF_NO_CORE
                JNZ     SHORT SDP_TERM_2
                CMP     EAX, SIGNALS
                JAE     SHORT SDP_TERM_2
                CMP     SIG_CORE[EAX], FALSE
                JE      SHORT SDP_TERM_2
                CALL    CORE_REGS_I
                CALL    CORE_DUMP
SDP_TERM_2:
;
; Default signal handler for SIGTERM
;
SDP_SIGTERM:    MOV     AX, 4C03H               ; exit(3)
                INT     21H

;
; Call user specified signal handler
;
SDP_USER:       MOV     ECX, FALSE              ; Not a trap
                CALL    SIG_USER_CALL
                JMP     SDP_RET

                ASSUME  BX:NOTHING
                ASSUME  BP:NOTHING
SIG_DELIVER_PENDING ENDP

;
; Display "Stopped by" message
;
; In:  EAX      Signal number
;
                ASSUME  DS:SV_DATA
SIG_MSG         PROC    NEAR
                LEA     EDX, $ABNORMAL
                CMP     EAX, SIGABRT
                JE      SHORT SIG_MSG_1
                LEA     EDX, $STOPPED_BY
                CALL    OTEXT
                LEA     EDX, $SIG_UNKNOWN
                CMP     EAX, SIGNALS
                JAE     SHORT SIG_MSG_1
                MOVZX   EDX, SIG_NAMES[EAX*2]
SIG_MSG_1:      CALL    OTEXT
                CALL    OCRLF
                RET
SIG_MSG         ENDP


;
; Call user specified signal handler on return from interrupt or exception.
;
; In:   EAX     Address of signal handler
;       EDX     Signal number
;       ECX     Non-FALSE if signal generated by an exception
;       BX      Pointer to process table entry
;
; We copy the saved registers to the interrupted program's stack
; to save supervisor stack space and to enable the program to reclaim
; the stack space by resetting the stack pointer: setjmp/longjmp.
;
; This function reenables interrupts as soon as possible.  It is
; assumed that interrupts are disabled when entering this function.
;
SUC_TRAP        EQU     (DWORD PTR [BP-1*4])
SUC_SIGNO       EQU     (DWORD PTR [BP-2*4])
SUC_HANDLER     EQU     (DWORD PTR [BP-3*4])
SUC_MASK        EQU     (DWORD PTR [BP-4*4])
SUC_FLAGS       EQU     (DWORD PTR [BP-5*4])

                ASSUME  DS:SV_DATA
                ASSUME  BX:PTR PROCESS
                ASSUME  BP:PTR ISTACKFRAME
SIG_USER_CALL   PROC    NEAR
                MOV     DI, BP
                ASSUME  DI:PTR ISTACKFRAME
                PUSH    BP
                MOV     BP, SP
                ASSUME  BP:NOTHING
                SUB     SP, 5 * 4
                MOV     SUC_HANDLER, EAX
                MOV     SUC_TRAP, ECX
                MOV     SUC_SIGNO, EDX
;
; Compute and set new signal mask
;
                MOV     SI, DX                  ; Signal number
                SHL     SI, 2
                MOV     EAX, [BX].P_SIG_BLOCKED
                MOV     SUC_MASK, EAX           ; Previous signal mask
                TEST    [BX].P_SA_FLAGS[SI], SA_SYSV
                JNZ     SHORT SUC_SYSV
                BTS     EAX, EDX                ; Block the signal
                TEST    [BX].P_SA_FLAGS[SI], SA_ACK
                JNZ     SHORT SUC_SETMASK
                OR      EAX, [BX].P_SA_MASK[SI] ; Block signals in sa_mask
                JMP     SHORT SUC_SETMASK

SUC_SYSV:       MOV     [BX].P_SIG_HANDLERS[SI], SIG_DFL
                JMP     SHORT SUC_1

SUC_SETMASK:    MOV     [BX].P_SIG_BLOCKED, EAX
SUC_1:          MOV     EAX, [BX].P_SA_FLAGS[SI]
                STI                             ; Enable interrupts
;
; Compute sa_flags (set SA_TRAP if signal generated by an exception)
;
                AND     EAX, NOT SA_TRAP
                CMP     SUC_TRAP, FALSE
                JE      SHORT SUC_2
                OR      EAX, SA_TRAP
SUC_2:          MOV     SUC_FLAGS, EAX
;
; Copy registers to the stack
;
                MOV     EAX, SS:[DI].IS_ESP     ; User stack
                SUB     EAX, I_REG_DWORDS * 4   ; Allocate stack frame
                MOV     SS:[DI].IS_ESP, EAX     ; and adjust ESP
                PUSH    DS                      ; Save DS
                PUSH    EDI                     ; Save EDI
                MOV     ECX, I_REG_DWORDS       ; Copy saved registers
                PUSH    SS                      ; Copy from supervisor stack
                POP     DS
                ASSUME  DS:NOTHING
                MOVZX   ESI, DI                 ; Pointer to stack frame
                MOV     ES, SS:[DI].IS_SS       ; Access program's stack
                MOV     EDI, EAX
                CLD
                REP     MOVS DWORD PTR ES:[EDI], DWORD PTR DS:[ESI]
                POP     EDI                     ; Restore EDI
                POP     DS                      ; Restore DS
                ASSUME  DS:SV_DATA
;
; Then, build a stack frame for the signal handler and return.
; Problem: on return from the signal handler, it should restore all
; registers from the stack. Where shall we put that code?
;
; 1. put it into the startup code (at a fixed location or introduce
;    a system function called by startup code for telling the supervisor
;    where that code resides)
;
; 2. put it into the application's stack
;
; 3. put it into the supervisor. The signal handler must return to the
;    supervisor. This is tricky, as the signal handler normally returns
;    with a *near* `RET', which doesn't change privilege levels. Here's
;    the trick: The `RET' instruction must generate an exception. There's
;    little choice: Either a segmentation fault (EIP beyond CS limit)
;    or a page fault (page not present). For the second alternative,
;    we would have to reserve a page *inside* the code segment for
;    that purpose to avoid a segmentation fault and we wouldn't know why
;    and from where we got to that address. Therefore, we'll return
;    to an address outside the code segment. CS:EIP will point to the
;    offending instruction (`RET') and SS:ESP will pointer to the address
;    we chose for that purpose. We assume that all signal handlers return
;    by means of `RET', not by `RET n' or `JMP'. For additional security,
;    we put a signature word onto the stack.
;
; This is the stack frame we need:
;
;       +-------------------------------------------+
;       |                                           |
;       | Saved registers of interrupted program    |
;       | (see PMINT.INC for details)               |
;       |                                           |
;       +-------------------------------------------+
;       |                                           |
;       | SIGFRAME:                                 |
;       | - Old signal mask                         |
;       | - sa_flags                                |
;       | - Signal number                           |
;       | - Signature word                          |
;       | - Signal number (signal handler argument) |
;       | - Our special EIP                         |
;       |                                           |
; ESP-> +-------------------------------------------+
;
; The signal number is stored twice as the signal handler may
; change its argument.
;
                MOV     ESI, SS:[DI].IS_ESP     ; Now points to saved regs
                SUB     ESI, SIZE SIGFRAME      ; Push SIGFRAME
                MOV     SS:[DI].IS_ESP, ESI     ; Adjust ESP
                ASSUME  ESI:NEAR32 PTR SIGFRAME
                MOV     ES:[ESI].SF_EIP, SIG_EIP ; Our special EIP
                MOV     EAX, SUC_SIGNO
                MOV     ES:[ESI].SF_ARG, EAX    ; Signal number (arg)
                MOV     ES:[ESI].SF_SIGNO, EAX  ; Signal number
                MOV     ES:[ESI].SF_SIGNATURE, SIG_SIGNATURE ; Signature
                MOV     EAX, SUC_MASK
                MOV     ES:[ESI].SF_MASK, EAX   ; Old signal mask
                MOV     EAX, SUC_FLAGS
                MOV     ES:[ESI].SF_FLAGS, EAX  ; sa_flags
                ASSUME  ESI:NOTHING
                MOV     EAX, SUC_HANDLER
                MOV     SS:[DI].IS_EIP, EAX     ; Address of signal handler
                CALL    BREAK_AFTER_IRET        ; Allow debugging signals
                MOV     SP, BP
                POP     BP
                RET                             ; Done
                ASSUME  BX:NOTHING
                ASSUME  DI:NOTHING
SIG_USER_CALL   ENDP


;
; Return from signal handler
;
; Remove our special stack frame and restore registers from user program's
; stack
;
; In:  ES:ESI   Pointer to special stack frame
;
                ASSUME  DS:SV_DATA
                ASSUME  BP:PTR ISTACKFRAME
                ASSUME  ESI:NEAR32 PTR SIGFRAME
SIG_RETURN      PROC    NEAR
                MOV     BX, PROCESS_PTR         ; Current process
                CMP     BX, NO_PROCESS          ; No process?
                JE      SIGRET_TERMINATE        ; Yes -> cannot happen
                ASSUME  BX:PTR PROCESS

                MOV     EAX, ES:[ESI].SF_SIGNO  ; Get the signal number
                CMP     EAX, SIGNALS            ; Signal number out of range?
                JAE     SHORT SIGRET_TERMINATE  ; Yes -> terminate process
                MOV     ECX, ES:[ESI].SF_FLAGS  ; Saved sa_flags
                TEST    ECX, SA_TRAP            ; Generated by exception?
                JZ      SHORT SIGRET_1          ; No  -> continue
                CMP     SIG_FUN_ACTION[EAX], FALSE ; Continue program?
                JE      SHORT SIGRET_TERMINATE  ; No  -> terminate process
SIGRET_1:       TEST    ECX, SA_SYSV OR SA_ACK
                JNZ     SHORT SIGRET_REGS       ; Don't restore signal mask
                MOV     EAX, ES:[ESI].SF_MASK   ; Restore signal mask
                MOV     [BX].P_SIG_BLOCKED, EAX

SIGRET_REGS:    PUSH    DS                      ; Save DS
                ADD     ESI, SIZE SIGFRAME      ; Skip special stack frame
                ASSUME  ESI:NOTHING
                MOV     EDI, EBP                ; Copy to supervisor stack
                PUSH    SS
                POP     ES
                MOV     DS, I_SS                ; Copy from user stack segment
                ASSUME  DS:NOTHING
                CLD
                MOV     ECX, I_REG_DWORDS       ; Copy saved registers
                REP     MOVS DWORD PTR ES:[EDI], DWORD PTR DS:[ESI]
                POP     DS
                ASSUME  DS:SV_DATA
                MOV     I_ESP, ESI              ; Adjust stack pointer
                CALL    BREAK_AFTER_IRET        ; Continue debugging
                JMP     EXCEPT_RET              ; Return to interrupted program

SIGRET_TERMINATE:
                RET                             ; Continue exception handler
                ASSUME  BX:NOTHING
                ASSUME  BP:NOTHING
SIG_RETURN      ENDP



;
; Implementation of __signal()
;
; In:   EAX     Signal number
;       EDX     Signal handler
;       DI      Pointer to process table entry
;
; Out:  EAX     Previous handler or SIG_ERR
;
ISA             EQU     (SIGACTION PTR [BP-1*SIZE SIGACTION])
OSA             EQU     (SIGACTION PTR [BP-2*SIZE SIGACTION])

                ASSUME  DS:SV_DATA
                ASSUME  DI:PTR PROCESS
                TALIGN  4
DO_SIGNAL       PROC    NEAR
                PUSH    BP
                MOV     BP, SP
                SUB     SP, 2 * SIZE SIGACTION
                MOV     ISA.SA_MASK, 0
                MOV     ISA.SA_HANDLER, EDX
;
; Set sa_flags according to __uflags()
;
                MOV     EDX, [DI].P_UFLAGS
                AND     EDX, UF_SIG_MODEL
                MOV     ECX, SA_ACK
                CMP     EDX, UF_SIG_EMX
                JE      SHORT SIGNAL_1
                MOV     ECX, SA_SYSV
                CMP     EDX, UF_SIG_SYSV
                JE      SHORT SIGNAL_1
                MOV     ECX, 0
SIGNAL_1:       MOV     ISA.SA_FLAGS, ECX
;
; Call __sigaction()
;
                PUSH    SS
                POP     ES
                LEA     EDX, ISA
                LEA     EBX, OSA
                CALL    DO_SIGACTION
                OR      EAX, EAX
                MOV     EAX, SIG_ERR
                JNZ     SHORT SIGNAL_RET
                MOV     EAX, OSA.SA_HANDLER
SIGNAL_RET:     MOV     SP, BP
                POP     BP
                RET
                ASSUME  DI:NOTHING
DO_SIGNAL       ENDP                


;
; This function implements __sigaction()
;
; In:   EAX     Signal number
;       ES:EDX  Pointer to sigaction structure (input)
;       ES:EBX  Pointer to sigaction structure (output)
;
; Out:  EAX     errno (0 if no error)
;
SIGA_O_HANDLER  EQU     (DWORD PTR [BP-1*4])
SIGA_O_MASK     EQU     (DWORD PTR [BP-2*4])
SIGA_O_FLAGS    EQU     (DWORD PTR [BP-3*4])
SIGA_OACT       EQU     (DWORD PTR [BP-4*4])
SIGA_SIGNO      EQU     (DWORD PTR [BP-5*4])

                ASSUME  DS:SV_DATA
                TALIGN  4
                ASSUME  DI:PTR PROCESS
DO_SIGACTION    PROC    NEAR
                PUSH    BP
                MOV     BP, SP
                SUB     SP, 5 * 4
                MOV     SIGA_SIGNO, EAX
                MOV     SIGA_OACT, EBX
                CMP     EAX, SIGNALS            ; Valid signal number?
                JAE     SIGACT_ERROR            ; No  -> failure
                CMP     SIG_VALID[EAX], FALSE
                JE      SIGACT_ERROR            ; No  -> failure
                MOV     BX, AX
                SHL     BX, 2
;
; Save the current action (in case both pointers point to the same object).
;
                CLI                             ; Start of critical section
                MOV     EAX, [DI].P_SIG_HANDLERS[BX]
                MOV     SIGA_O_HANDLER, EAX
                MOV     EAX, [DI].P_SA_MASK[BX]
                MOV     SIGA_O_MASK, EAX
                MOV     EAX, [DI].P_SA_FLAGS[BX]
                MOV     SIGA_O_FLAGS, EAX
;
; Set the new action if present
;
                TEST    EDX, EDX
                JZ      SHORT SIGACT_NO_INPUT
                CMP     SIGA_SIGNO, SIGKILL     ; Cannot modify SIGKILL
                JE      SIGACT_ERROR_STI
                ASSUME  EDX:NEAR32 PTR SIGACTION
                MOV     EAX, ES:[EDX].SA_MASK
                SHL     EAX, 1
                AND     EAX, SIG_BLOCK_MASK
                MOV     [DI].P_SA_MASK[BX], EAX
                MOV     ECX, ES:[EDX].SA_FLAGS
                MOV     [DI].P_SA_FLAGS[BX], ECX
                MOV     EAX, ES:[EDX].SA_HANDLER
                MOV     [DI].P_SIG_HANDLERS[BX], EAX
                ASSUME  EDX:NOTHING
;
; Discard a pending signal when setting the handler to SIG_IGN or,
; if the default action is ignoring the signal, to SIG_DFL.
;
                CMP     EAX, SIG_DFL
                JE      SHORT SIGACT_DISCARD
                CMP     EAX, SIG_IGN
                JE      SHORT SIGACT_NO_DISCARD
SIGACT_DISCARD: MOV     EAX, SIGA_SIGNO
                BTR     [DI].P_SIG_PENDING, EAX
SIGACT_NO_DISCARD:
;
; Store the previous action if OACT is not NULL
;
SIGACT_NO_INPUT:STI                             ; End of critical section
                MOV     ESI, SIGA_OACT
                TEST    ESI, ESI
                JZ      SHORT SIGACT_NO_OUTPUT
                ASSUME  ESI:NEAR32 PTR SIGACTION
                MOV     EAX, SIGA_O_HANDLER
                MOV     ES:[ESI].SA_HANDLER, EAX
                MOV     EAX, SIGA_O_MASK
                SHR     EAX, 1
                MOV     ES:[ESI].SA_MASK, EAX
                MOV     EAX, SIGA_O_FLAGS
                MOV     ES:[ESI].SA_FLAGS, EAX
                ASSUME  ESI:NOTHING
SIGACT_NO_OUTPUT:
                XOR     EAX, EAX
SIGACT_RET:     MOV     SP, BP
                POP     BP
                RET

SIGACT_ERROR_STI:
                STI
SIGACT_ERROR:   MOV     EAX, EINVAL
                JMP     SIGACT_RET
                ASSUME  DI:NOTHING
DO_SIGACTION    ENDP


SV_CODE         ENDS


INIT_CODE       SEGMENT

                ASSUME  CS:INIT_CODE, DS:NOTHING

;
; Hook BIOS interrupt: 1BH (Ctrl-Break)
; Hook DOS interrupts: 23H (Ctrl-C) and 24H (critical error)
;
                ASSUME  DS:SV_DATA
HOOK_INT        PROC    NEAR
                MOV     AL, 1BH                 ; Hook interrupt 1BH
                LEA     DX, CNTRL_BREAK         ; (Ctrl-Break)
                CALL    SET_RM_INT
                MOV     OLD_BREAK_OFF, DX       ; Save old Ctrl-Break
                MOV     OLD_BREAK_SEG, AX       ; handler vector
                MOV     AL, 23H                 ; Hook interrupt 23H
                LEA     DX, CNTRL_C             ; (Ctrl-C)
                CALL    SET_RM_INT
                MOV     AL, 24H                 ; Hook interrupt 24H
                LEA     DX, CRIT_ERROR          ; (critical error)
                CALL    SET_RM_INT
                MOV     CS:OLD_CRIT_OFF, DX     ; Save old critical error
                MOV     CS:OLD_CRIT_SEG, AX     ; handler vector
                RET
HOOK_INT        ENDP

;
; Remove interrupt hooks
;
                ASSUME  DS:SV_DATA
CLEANUP_SIGNAL  PROC    NEAR
                MOV     DX, OLD_BREAK_OFF
                MOV     BX, OLD_BREAK_SEG
                OR      BX, BX
                JZ      SHORT CS_1
                MOV     AL, 1BH
                CALL    RESTORE_RM_INT
CS_1:           RET
CLEANUP_SIGNAL  ENDP


;
; Ctrl-Break interrupt
;
; This interrupt is issued by BIOS when Ctrl-Break is pressed.
;
; Note: INT 23H also raises SIGINT.  To avoid SIGINT being raised twice,
;       CNTRL_BREAK returns instead of jumping to the original vector.
;       This way, DOS doesn't get informed about Ctrl-Break.
;
                ASSUME  DS:NOTHING
CNTRL_BREAK     PROC    FAR
                CALL    LOCAL_STACK
                PUSHAD
                PUSH    DS                      ; Save all registers
                MOV     AX, SV_DATA
                MOV     DS, AX
                ASSUME  DS:SV_DATA
;
; Check for emergency exit (Ctrl-Break five times in one second)
;
                CMP     EMERGENCY_TIMER, 0      ; Timer running?
                JE      SHORT CB_EMGCY_START    ; No  -> start timer
                INC     EMERGENCY_COUNTER       ; Increment counter
                CMP     EMERGENCY_COUNTER, 4    ; 5 Ctrl-Break in one second?
                JNE     SHORT CB_EMGCY_DONE     ; No  -> skip
                MOV     EMERGENCY_FLAG, NOT FALSE ; Yes -> set flag
                JMP     SHORT CB_EMGCY_DONE

CB_EMGCY_START: MOV     EMERGENCY_COUNTER, 0    ; Initialize counter
                MOV     EMERGENCY_TIMER, 18     ; Set timer to one second
CB_EMGCY_DONE:
;
; Generate SIGINT if appropriate
;
                MOV     BX, PROCESS_SIG         ; Get current process
                CMP     BX, NO_PROCESS          ; In supervisor?
                JE      SHORT CB_DONE           ; Yes -> ignore
                ASSUME  BX:PTR PROCESS
                BTS     [BX].P_SIG_PENDING, SIGINT ; Generate SIGINT
CB_DONE:        POP     DS                      ; Restore registers
                ASSUME  DS:NOTHING
                POPAD
                ASSUME  BX:NOTHING
                CALL    RESTORE_STACK           ; Switch back to original stack
                IRET                            ; Return
CNTRL_BREAK     ENDP

;
; Ctrl-C interrupt
;
                ASSUME  DS:NOTHING
CNTRL_C         PROC    FAR
                CALL    LOCAL_STACK             ; Switch to local stack
                PUSHAD
                PUSH    DS                      ; Save all registers
                PUSH    ES
                PUSH    FS
                PUSH    GS
                MOV     AX, SV_DATA
                MOV     DS, AX
                ASSUME  DS:SV_DATA
                CMP     CRIT_ERR_FLAG, FALSE    ; Critical error?
                JNE     SHORT CC_QUIT           ; Yes -> just quit
                MOV     BX, PROCESS_SIG         ; Get current process
                CMP     BX, NO_PROCESS          ; In supervisor?
                JE      SHORT CC_QUIT           ; Yes -> just quit
                ASSUME  BX:PTR PROCESS
                BTS     [BX].P_SIG_PENDING, SIGINT      ; Generate SIGINT
                ASSUME  BX:NOTHING
                POP     GS
                POP     FS
                POP     ES
                POP     DS                      ; Restore registers
                POPAD
                NOP                             ; Avoid 386 bug
                CALL    RESTORE_STACK           ; Switch back to original stack
                IRET

CC_QUIT:        CALL    CLEANUP                 ; Cleanup for exit
                POP     GS
                POP     FS
                POP     ES
                POP     DS                      ; Restore registers
                POPAD
                NOP                             ; Avoid 386 bug
                CALL    RESTORE_STACK           ; Switch back to original stack
                MOV     AX, 4C03H               ; End program, errorlevel 3
                INT     21H
                JMP     SHORT $                 ; Never reached
CNTRL_C         ENDP



OLD_CRIT        LABEL   DWORD
OLD_CRIT_OFF    DW      ?
OLD_CRIT_SEG    DW      ?

;
; Critical error handler
;
                ASSUME  DS:NOTHING
CRIT_ERROR      PROC    FAR
                PUSHF                           ; Call original handler
                CALL    OLD_CRIT                ; (it's an interrupt routine)
                CMP     AL, 2                   ; Abort the program?
                JNE     SHORT CRIT_RET          ; No -> skip
                CALL    LOCAL_STACK             ; Switch to local stack
                PUSHAD                          ; Save registers
                PUSH    DS
                PUSH    ES
                MOV     AX, SV_DATA
                MOV     DS, AX                  ; Setup DS
                ASSUME  DS:SV_DATA
                MOV     CRIT_ERR_FLAG, NOT FALSE ; Avoid some DOS calls
                CALL    CLEANUP                 ; Cleanup before return to DOS
                LEA     DX, $ABORTED
                MOV     AH, 09H                 ; Display message
                INT     21H
                POP     ES                      ; Restore registers
                POP     DS
                ASSUME  DS:NOTHING
                POPAD
                CALL    RESTORE_STACK           ; Switch to original stack
CRIT_RET:       IRET                            ; Return
CRIT_ERROR      ENDP

;
; This variable is located in the code segment to avoid having to
; load segment registers
;
                TALIGN  2
LSTACK_NO       DW      0                       ; No local stacks in use

;
; Switch to local stack
;
; Only flags altered
;
                ASSUME  DS:NOTHING, ES:NOTHING, SS:NOTHING
LOCAL_STACK     PROC    NEAR
                CMP     LSTACK_NO, LSTACKS      ; Any stacks left?
                JB      SHORT LSTACK1           ; Yes -> continue
                MOV     AX, SV_DATA
                MOV     DS, AX
                ASSUME  DS:SV_DATA
                LEA     DX, $STACKS             ; We're out of stacks
                MOV     AH, 09H                 ; Display message
                INT     21H
                JMP     SHORT $                 ; Stop
                ASSUME  DS:NOTHING

LSTACK1:        PUSH    BX                      ; Save registers on original
                PUSH    AX                      ; stack (will be moved to
                PUSH    DS                      ; new stack)
                PUSH    DX                      ; Save DX
                INC     LSTACK_NO               ; One more used stack
                MOV     AX, LSTACK_NO           ; Get stack number
                MOV     DX, LSTACK_SIZE         ; Size of one stack
                MUL     DX                      ; DX:AX := top of stack
                MOV     BX, AX                  ; BX := top of stack
                MOV     AX, LSTACK
                MOV     DS, AX                  ; DS:BX points to top of stack
                ASSUME  DS:LSTACK
                POP     DX                      ; Restore DX
                SUB     BX, 12                  ; Build stack frame
                POP     WORD PTR [BX+0]         ; Move DS to new stack
                POP     WORD PTR [BX+2]         ; Move AX to new stack
                POP     WORD PTR [BX+4]         ; Move BX to new stack
                POP     WORD PTR [BX+6]         ; Move return address
                MOV     [BX+8], SP              ; Copy original SS:SP
                MOV     [BX+10], SS             ; to new stack
                MOV     SS, AX                  ; (Disables interrupts for:)
                MOV     SP, BX                  ; Switch to new stack
                POP     DS                      ; Restore registers
                ASSUME  DS:NOTHING              ; from new stack
                POP     AX
                POP     BX
                RET                             ; Return address from new stack
LOCAL_STACK     ENDP


;
; Switch back to original stack
;
; Only flags altered
;

                ASSUME  DS:NOTHING, ES:NOTHING, SS:NOTHING
RESTORE_STACK   PROC    NEAR
                PUSH    BP                      ; Save registers on local
                PUSH    DS                      ; stack (will be moved to
                PUSH    BX                      ; original stack)
                DEC     LSTACK_NO               ; Adjust number of used stacks
                MOV     BP, SP                  ; SS:SP points to local stack
                LDS     BX, [BP+8]              ; Original stack
                SUB     BX, 8                   ; Build stack frame
                POP     WORD PTR [BX+0]         ; Move BX to original stack
                POP     WORD PTR [BX+2]         ; Move DS to original stack
                POP     WORD PTR [BX+4]         ; Move BP to original stack
                POP     WORD PTR [BX+6]         ; Move return address
                MOV     BP, DS                  ; Copy DS to SS
                MOV     SS, BP                  ; (Disables interrupts for:)
                MOV     SP, BX                  ; Switch to original stack
                POP     BX                      ; Restore registers from
                POP     DS                      ; original stack
                POP     BP
                RET
RESTORE_STACK   ENDP


INIT_CODE       ENDS

                END
