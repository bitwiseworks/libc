;
; RMINT.ASM -- Initialize interrupts and handle real-mode interrupts
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
                INCLUDE VCPI.INC
                INCLUDE PMINT.INC
                INCLUDE EXCEPT.INC
                INCLUDE RPRINT.INC
                INCLUDE SIGNAL.INC
                INCLUDE PROCESS.INC
                INCLUDE OPTIONS.INC
                INCLUDE MISC.INC

                PUBLIC  INIT_INT, CLEANUP_INT, INIT_TIMER, CLEANUP_TIMER
                PUBLIC  SET_RM_INT, RESTORE_RM_INT

RM_DEBUGGER     =       FALSE

;
; The interrupt redirection machinery has four states:
;
; - IC_NOT
;       This is the initial state: interrupt vectors have not been
;       changed yet.  Do not restore interrupt vectors on exit!
;
; - IC_VCPI_SERVER
;       There is a VCPI server (EMS emulator) which has already reprogrammed
;       the interrupt controller to redirect hardware interrupts. Or a
;       different client of the VCPI server has redirected the hardware
;       interrupts. This is the simple case as we don't have to touch
;       the interrupt controller.
;
; - IC_VCPI_CLIENT
;       There is a VCPI server (EMS emulator) which hasn't reprogrammed
;       the interrupt controller. We have to reprogram the interrupt
;       controller in order to distinguish hardware interrupts from
;       processor exceptions. PC design flaw...
;
; - IC_MYSELF
;       There is no VCPI server (EMS emulator). We have to reprogram the
;       interrupt controller in order to distinguish hardware interrupts from
;       processor exceptions. PC design flaw...
;

IC_NOT          =       0               ; Interrupt vectors not changed
IC_VCPI_SERVER  =       1               ; VCPI server has changed vectors
IC_VCPI_CLIENT  =       2               ; VCPI server hasn't changed vectors
IC_MYSELF       =       3               ; This program has changed vectors



SV_DATA         SEGMENT

              IF RM_DEBUGGER
;
; The original debug interrupt vectors.
;
OLD_RM_INT01_OFF        DW      ?
OLD_RM_INT01_SEG        DW      ?

OLD_RM_INT03_OFF        DW      ?
OLD_RM_INT03_SEG        DW      ?

              ENDIF

;
;
; The original interrupt vectors IRQ0_BASE .. IRQ0_BASE+7.
;
SAVED_IVECS     DD      8 DUP (?)

;
; Check the interrupt vectors in this table in this sequence for
; 8 successive unused interrupts vectors. The first entry should
; be the safest, the last the most dangerous. But all vectors
; are checked, so that nothing evil should happen if a vector
; is already in use. The danger is that some other program may
; change an interrupt vector used by this program for redirecting
; a hardware interrupt. This would cause a system crash.
;
UNUSED_INT_TAB  DB      50H, 88H, 90H, 98H
                DB      0A8H, 0B8H, 0C0H, 0C8H, 0D0H, 0D8H
                DB      80H, 0A0H, 0B0H, 0E0H, 0E8H, 38H, 78H, 58H, 0F0H
                DB      00H

;
; These are the offsets for the new interrupt vectors used for IRQ0..IRQ7.
; The code at that address just issues INT 08H .. INT 0FH.
;
NEW_IVECS       DW      HWINT0, HWINT1, HWINT2, HWINT3
                DW      HWINT4, HWINT5, HWINT6, HWINT7

;
; These are the first interrupt vector used for redirecting IRQ0..IRQ7
; and IRQ8..IRQ15, respectively.
;
IRQ0_BASE       DB      ?
IRQ8_BASE       DB      ?

;
; This is the interrupt redirection status.
;
INT_CHANGED     DB      IC_NOT
;
; This is the future interrupt redirection status.
;
NEW_INT_CHANGED DB      IC_NOT

;
; This message should never be required: The VCPI server (or one of its
; clients) has changed the interrupt vectors. But that mappings aren't
; usable.
;
$BAD_VCPI       DB      "Unusable interrupt vector mappings set by VCPI server", CR, LF, 0

              IF RM_DEBUGGER
;
; There's no real-mode debugger
;
$RM_DEBUG       DB      "Real-mode breakpoint at ", 0

              ENDIF

SV_DATA         ENDS


INIT_CODE       SEGMENT

                ASSUME  CS:INIT_CODE, DS:NOTHING

;
; The original timer interrupt vector. This is in the code segment because
; we need OLD_RM_INT1C for jumping to the next interrupt handler in the
; chain.
;
OLD_RM_INT1C            LABEL   DWORD
OLD_RM_INT1C_OFF        DW      ?
OLD_RM_INT1C_SEG        DW      ?
OLD_RM_INT1C_FLAG       DB      FALSE


;
; Redirect hardware interrupt IRQ X to real-mode interrupt Y
;
                ASSUME  DS:NOTHING
HWINTX          MACRO   X,Y
                TALIGN  2
HWINT&X:        INT     Y                       ; Issue software interrupt
                IRET                            ; Return
                ENDM
;
; Redirect IRQ 0 (-> INT 08H) to IRQ 7 (-> INT 0FH)
;
                IRPC    X, <01234567>
                HWINTX  X, %&X+8
                ENDM


;
; Access rights for 32-bit interrupt gates
;
INT_GATE_0      =       08EH                    ; DPL=0 (hardware interrupt)
INT_GATE_1      =       0AEH                    ; DPL=1
INT_GATE_2      =       0CEH                    ; DPL=2
INT_GATE_3      =       0EEH                    ; DPL=3 (software interrupt)


;
; Initialize IDT, redirect hardware interrupts
;
; This procedure is also called after spawning a DOS program
;

                ASSUME  DS:SV_DATA

                TALIGN  2
INIT_INT        PROC    NEAR
                MOV     IRQ0_BASE, 40H
                MOV     IRQ8_BASE, 48H
                CMP     MACHINE, MACH_FMR70     ; Fujitsu FMR70?
                JE      II_TABLE                ; Yes -> use above values
                MOV     IRQ0_BASE, 40H
                MOV     IRQ8_BASE, 48H
                CMP     MACHINE, MACH_PC98      ; NEC PC-98?
                JE      II_TABLE                ; Yes -> use above values
                MOV     IRQ8_BASE, 70H          ; The standard configuration
                MOV     NEW_INT_CHANGED, IC_MYSELF
                CMP     VCPI_FLAG, FALSE        ; Use VCPI?
                JE      SHORT II_CHECK_INT      ; No -> redirect interrupts
                CALL    GET_INT_MAP             ; Get interrupt vector mappings
                MOV     IRQ0_BASE, BL           ; Controller 1 address
                MOV     IRQ8_BASE, CL           ; Controller 2 address
                CMP     BL, 17                  ; Conflict with exceptions?
                JBE     SHORT II_NEED_REMAP     ; Yes -> remapping required
                CMP     CL, 17                  ; Conflict with exceptions?
                JBE     SHORT II_NEED_REMAP     ; Yes -> remapping required
;
; Interrupt vector mapping is usable, VCPI server has done this.
;
                MOV     NEW_INT_CHANGED, IC_VCPI_SERVER
                JMP     SHORT II_TABLE          ; VCPI server has done the work

II_NEED_REMAP:  CMP     BL, 08H                 ; Standard configuration?
                JNE     II_ERROR                ; No  -> almost impossible
                CMP     CL, 70H                 ; Standard configuration?
                JNE     II_ERROR                ; No  -> almost impossible
                MOV     NEW_INT_CHANGED, IC_VCPI_CLIENT
                JMP     SHORT II_MYSELF

;
; There may be programs which redirect IRQ0..IRQ7. As we're not
; running under a VCPI server, we should find this out by hand.
; Here's how:
; - save all the interrupt vectors
; - setup 256 *different* interrupt handlers
; - set a variable V in the code segment to some number I unequal 0..255
; - each handler stores its vector number to V if V equals I
;   (this must be done in an atomic operation) and then jumps to the
;   original handler for that interrupt vector
; - the main program loops until V != I or a loop counter expires
; - restore interrupt vectors
; - if V == 8, we have the original mapping
; Actually, we aren't doing all this. It isn't worth the trouble.
; There's an explicit check for DESQview in VCPI.ASM, instead.
;
II_CHECK_INT:

;
; We have to redirect hardware interrupts. Find 8 sucessive unused
; interrupt vectors for redirecting IRQ0..IRQ7.
;
II_MYSELF:      LEA     SI, UNUSED_INT_TAB      ; Table of vectors to check
                XOR     AX, AX                  ; AH:=0
                MOV     ES, AX                  ; Access interrupt table
II_FIND_1:      LODS    UNUSED_INT_TAB          ; Get interupt vector
                OR      AL, AL                  ; End of table?
                JZ      SHORT II_FIND_FAIL      ; Yes -> be brutal
                MOV     BX, AX                  ; AH=0
                SHL     BX, 2                   ; Compute address of vector
                MOV     EDX, ES:[BX]            ; Fetch first vector
                MOV     CX, 7                   ; Compare to the following 7
II_FIND_2:      ADD     BX, 4                   ; Next vector
                CMP     EDX, ES:[BX]            ; Same as first vector?
                JNE     II_FIND_1               ; No -> try next table entry
                LOOP    II_FIND_2               ; Repeat for 7 vectors
                MOV     IRQ0_BASE, AL           ; Use this vector
                JMP     SHORT II_TABLE          ; Continue

;
; No unused interrupt vectors found. We don't have mercy and use some
; interrupt vectors reserved for BASIC. Starting BASIC while this
; program is running will cause a system crash.
;
II_FIND_FAIL:   MOV     IRQ0_BASE, 0D0H

;
; Initialize all IDT entries.
;
II_TABLE:       LEA     DI, IDT+0
                MOV     CX, 256
                MOV     AL, INT_GATE_0
                MOV     DX, OFFSET SV_CODE:INTERRUPT
II_UNUSED_1:    CALL    SET_INT_GATE
                ADD     DI, 8
                LOOP    II_UNUSED_1
;
; Put exceptions into IDT.
;
; Use interrupt gate instead of trap gate to disable interrupts
;
                LEA     SI, EXC_TAB             ; Table of handlers
II_EXC_1:       LODS    EXC_TAB                 ; Get exception number
                CMP     AX, 0FFFFH              ; End of table?
                JE      SHORT II_EXC_3
                MOV     BX, AX
                SHL     AX, 3                   ; Compute offset into IDT
                LEA     DI, IDT
                ADD     DI, AX                  ; Compute pointer into IDT
                LODS    EXC_TAB                 ; Get offset of handler
                MOV     DX, AX
                MOV     AL, INT_GATE_0          ; Interrupt gate, DPL=0
                CMP     BL, 3                   ; Breakpoint?
                JNE     SHORT II_EXC_2          ; No -> use DPL=0
                MOV     AL, INT_GATE_3          ; Use DPL=3 (for breakpoints)
II_EXC_2:       CALL    SET_INT_GATE            ; Insert into IDT
                JMP     SHORT II_EXC_1          ; Repeat for all table entries
II_EXC_3:
;
; Put task gates for exceptions 8 & 10 into IDT.
;
                LEA     DI, IDT+8*8
                MOV     AX, G_TSS_EX8_SEL
                CALL    SET_TASK_GATE
                LEA     DI, IDT+10*8
                MOV     AX, G_TSS_EX10_SEL
                CALL    SET_TASK_GATE
;
; Put hardware interrupts into IDT.
;
                LEA     SI, PMINT0_TAB
                MOVZX   BX, IRQ0_BASE
                CALL    PUT_HW_INT
                LEA     SI, PMINT8_TAB
                MOVZX   BX, IRQ8_BASE
                CALL    PUT_HW_INT
;
; Put software interrupts into IDT.
;
                IRP     SWINT, <10H, 11H, 14H, 16H, 17H, 21H, 31H, 33H>
                LEA     DI, IDT + SWINT*8
                MOV     DX, OFFSET SV_CODE:PMINT_&SWINT
                MOV     AL, INT_GATE_3
                CALL    SET_INT_GATE
                ENDM
;
; Setup debug interrupt
;
              IF RM_DEBUGGER
                LEA     DX, RM_DEBUG
                MOV     AL, 01H
                CALL    SET_RM_INT
                MOV     OLD_RM_INT01_OFF, DX
                MOV     OLD_RM_INT01_SEG, AX
                LEA     DX, RM_DEBUG
                MOV     AL, 03H
                CALL    SET_RM_INT
                MOV     OLD_RM_INT03_OFF, DX
                MOV     OLD_RM_INT03_SEG, AX
              ENDIF
;
; Copy IRQ0_BASE and IRQ8_BASE to IRQ0_ADD and IRQ8_ADD (in the SV_CODE
; segment), respectively, to make them accessible in the protected-mode
; interrupt handler without setting any segment register (CS will be set
; automatically to SV_CODE_SEL).
;
                MOV     AX, SV_CODE             ; Used for setting IRQ0_ADD
                MOV     ES, AX                  ; and IRQ8_ADD
                ASSUME  ES:SV_CODE
                MOV     AL, IRQ0_BASE
                MOV     IRQ0_ADD, AL            ; Set IRQ0_ADD
                MOV     AL, IRQ8_BASE
                MOV     IRQ8_ADD, AL            ; Set IRQ8_ADD
                ASSUME  ES:NOTHING              ; Access to SV_CODE finished
;
; Redirect hardware interrupts if required: reprogram interrupt controller 1
; and notify VCPI server (if present).
;
                CLI
                CMP     NEW_INT_CHANGED, IC_MYSELF
                JNE     SHORT II_REDIR_1
                CALL    REDIR_INT_VEC
                MOV     CL, IRQ0_BASE
                CALL    SET_HW_INT_BASE
                JMP     SHORT II_REDIR_END

II_REDIR_1:     CMP     NEW_INT_CHANGED, IC_VCPI_CLIENT
                JNE     SHORT II_REDIR_END
                CALL    REDIR_INT_VEC
                MOV     CL, IRQ0_BASE
                CALL    SET_HW_INT_BASE
                MOVZX   BX, IRQ0_BASE
                MOVZX   CX, IRQ8_BASE           ; This is always 70H
                CALL    SET_INT_MAP             ; Inform VCPI server
;
; Now we can copy NEW_INT_CHANGED to INT_CHANGED.
;
II_REDIR_END:   MOV     AL, NEW_INT_CHANGED
                MOV     INT_CHANGED, AL
                RET

;
; We cannot cooperate with a buggy VCPI server; give up.
;
II_ERROR:       LEA     DX, $BAD_VCPI
                CALL    RTEXT
                MOV     AL, 0FFH
                JMP     EXIT

INIT_INT        ENDP

;
; Put hardware interrupts into IDT
;
; In:   BX      First interrupt number
;       DS:SI   Table of vectors
;
                ASSUME  DS:SV_DATA
PUT_HW_INT      PROC    NEAR
                SHL     BX, 3
                LEA     DI, IDT[BX]
                MOV     CX, 8
PHI_1:          LODS    PMINT0_TAB              ; ...or PMINT8_TAB
                MOV     DX, AX
                MOV     AL, INT_GATE_0          ; Not accessible from ring 3
                CALL    SET_INT_GATE
                ADD     DI, 8
                LOOP    PHI_1
                RET
PUT_HW_INT      ENDP

;
; Put interrupt gate (disables interrupts) into IDT
;
; In:   DS:DI   Pointer to IDT entry
;       DX      Offset of interrupt routine (SV_CODE:DX)
;       AL      INT_GATE_0 ... INT_GATE_3
;
                ASSUME  DS:NOTHING

SET_INT_GATE    PROC    NEAR
                MOV     WORD PTR [DI+0], DX
                MOV     WORD PTR [DI+2], G_SV_CODE_SEL
                MOV     BYTE PTR [DI+4], 0
                MOV     BYTE PTR [DI+5], AL
                MOV     WORD PTR [DI+6], 0
                RET
SET_INT_GATE    ENDP


;
; Put task gate into IDT
;
; In:   DS:DI   Pointer to IDT entry
;       AX      TSS selector
;
                ASSUME  DS:NOTHING
SET_TASK_GATE   PROC    NEAR
                MOV     WORD PTR [DI+0], 0
                MOV     WORD PTR [DI+2], AX
                MOV     BYTE PTR [DI+4], 0
                MOV     BYTE PTR [DI+5], 85H
                MOV     WORD PTR [DI+6], 0
                RET
SET_TASK_GATE   ENDP


;
; Setup real-mode interrupt vectors for redirecting IRQ0..IRQ7.
;
                ASSUME  DS:SV_DATA
REDIR_INT_VEC   PROC    NEAR
                XOR     AX, AX
                MOV     ES, AX
                LEA     SI, NEW_IVECS
                LEA     DI, SAVED_IVECS
                MOV     CX, 8
                MOVZX   BX, IRQ0_BASE
                SHL     BX, 2
II_41:          MOV     AX, ES:[BX+0]
                MOV     [DI+0], AX
                MOV     AX, ES:[BX+2]
                MOV     [DI+2], AX
                MOV     AX, [SI]
                MOV     ES:[BX+0], AX
                MOV     ES:[BX+2], CS
                ADD     BX, 4
                ADD     DI, 4
                ADD     SI, 2
                LOOP    II_41
                RET
REDIR_INT_VEC   ENDP

;
; Restore interrupt vectors
;
                ASSUME  DS:SV_DATA
RESTORE_INT_VEC PROC    NEAR
                XOR     AX, AX
                MOV     ES, AX
                MOVZX   DI, IRQ0_BASE
                SHL     DI, 2
                LEA     SI, SAVED_IVECS
                MOV     CX, 8
                CLD
                REP     MOVSD
                RET
RESTORE_INT_VEC ENDP

;
; Delay for accessing interrupt controller registers
;
IO_DELAY        MACRO
                LOCAL   SKIP
                JMP     SHORT SKIP
SKIP:
                ENDM


;
; Reprogram interrupt controller #1
;
; In:   CL      Interrupt base
;
                ASSUME  DS:SV_DATA
SET_HW_INT_BASE PROC    NEAR
                IN      AL, 21H         ; Read IMR
                MOV     AH, AL          ; Save old IMR
                CMP     MACHINE, MACH_INBOARD   ; Inboard 386/PC
                JE      SHORT SET_HIB_INB
                MOV     AL, 11H         ; ICW1
                OUT     20H, AL
                IO_DELAY
                MOV     AL, CL          ; ICW2
                OUT     21H, AL
                IO_DELAY
                MOV     AL, 04H         ; ICW3
                OUT     21H, AL
                IO_DELAY
                MOV     AL, 01H         ; ICW4
                OUT     21H, AL
                IO_DELAY
SET_HIB_END:    MOV     AL, AH
                OUT     21H, AL         ; Write IMR
                RET

;
; Use the values of the original PC for the Inboard 386/PC
;
SET_HIB_INB:    MOV     AL, 13H         ; ICW1
                OUT     20H,AL
                IO_DELAY
                MOV     AL, CL          ; ICW2
                OUT     21H,AL
                IO_DELAY
                MOV     AL, 09H         ; ICW4
                OUT     21H, AL
                JMP     SHORT SET_HIB_END

SET_HW_INT_BASE ENDP


;
; Install real-mode interrupt handler
;
; In:   AL      Interrupt number
;       DX      Offset of interrupt handler (CS=INIT_CODE)
;
; Out:  AX      Segment of original interrupt handler
;       DX      Offset of original interrupt handler
;
                ASSUME  DS:NOTHING
SET_RM_INT      PROC    NEAR
                PUSH    DS
                PUSH    ES
                PUSH    BX
                MOV     AH, 35H
                INT     21H
                PUSH    CS
                POP     DS
                PUSH    BX
                PUSH    ES
                MOV     AH, 25H
                INT     21H
                POP     AX
                POP     DX
                POP     BX
                POP     ES
                POP     DS
                RET
SET_RM_INT      ENDP

;
; Restore real-mode interrupt vector
;
; This is done only if the interrupt vector still points into INIT_CODE
; (avoid restoring the vector if it has been hooked by somebody else)
;
; In:   AL      Interrupt number
;       DX      Original offset
;       BX      Original segment
;
                ASSUME  DS:NOTHING
RESTORE_RM_INT  PROC    NEAR
                PUSH    ES
                PUSH    BX
                MOV     AH, 35H
                INT     21H
                MOV     BX, ES
                CMP     BX, INIT_CODE
                POP     BX
                POP     ES
                JNE     SHORT RRI_RET
                PUSH    DS
                MOV     DS, BX
                MOV     AH, 25H
                INT     21H
                POP     DS
RRI_RET:        RET
RESTORE_RM_INT  ENDP


;
; Restore interrupts
;
; This procedure is also called before spawning a DOS program
;
                ASSUME  DS:SV_DATA
CLEANUP_INT     PROC    NEAR
                CMP     INT_CHANGED, IC_NOT     ; Interrupt vectors remapped?
                JE      SHORT CI_9              ; No  -> skip
                CLI                             ; Interrupts must be disabled
                CMP     INT_CHANGED, IC_MYSELF  ; Done without VCPI?
                JNE     SHORT CI_1              ; No  -> try next case
                CALL    RESTORE_INT_VEC         ; Restore interrupt vectors
                MOV     CL, 8                   ; Restore interrupt controller
                CALL    SET_HW_INT_BASE
                JMP     SHORT CI_8              ; Done

CI_1:           CMP     INT_CHANGED, IC_VCPI_CLIENT     ; Done with VCPI?
                JNE     SHORT CI_8              ; No  -> skip
                CALL    RESTORE_INT_VEC         ; Restore interrupt vectors
                MOV     CL, 8                   ; Restore interrupt controller
                CALL    SET_HW_INT_BASE
                MOV     BX, 8                   ; Tell VCPI server about change
                MOV     CX, 70H
                CALL    SET_INT_MAP

CI_8:           MOV     INT_CHANGED, IC_NOT     ; Back at original state
                STI                             ; Interrupts allowed again
CI_9:         IF RM_DEBUGGER
                MOV     DX, OLD_RM_INT01_OFF
                MOV     BX, OLD_RM_INT01_SEG
                MOV     AL, 01H
                CALL    RESTORE_RM_INT
                MOV     DX, OLD_RM_INT03_OFF
                MOV     BX, OLD_RM_INT03_SEG
                MOV     AL, 03H
                CALL    RESTORE_RM_INT
              ENDIF
                RET

CLEANUP_INT     ENDP


;
; Initialize timer interrupt
;
; This procedure is not called after spawning a DOS program
;

                ASSUME  DS:SV_DATA
                TALIGN  2
INIT_TIMER      PROC    NEAR
                CMP     MACHINE, MACH_PC98      ; NEC PC-98?
                JE      SHORT IT_DONE           ; Yes -> don't set INT 1CH
                LEA     DX, RM_TIMER
                MOV     AL, 1CH
                CALL    SET_RM_INT
                MOV     OLD_RM_INT1C_FLAG, NOT FALSE
                MOV     OLD_RM_INT1C_OFF, DX
                MOV     OLD_RM_INT1C_SEG, AX
IT_DONE:        RET
INIT_TIMER      ENDP

;
; Restore timer interrupt
;
; This procedure is not called before spawning a DOS program
;
                ASSUME  DS:SV_DATA
                TALIGN  2
CLEANUP_TIMER   PROC    NEAR
                CMP     OLD_RM_INT1C_FLAG, FALSE
                JE      SHORT CT_DONE
                MOV     OLD_RM_INT1C_FLAG, FALSE
                MOV     DX, OLD_RM_INT1C_OFF
                MOV     BX, OLD_RM_INT1C_SEG
                MOV     AL, 1CH
                CALL    RESTORE_RM_INT
CT_DONE:        RET
CLEANUP_TIMER   ENDP

;
; The timer interrupt handler.  Note that interrupt 1CH is not
; available on NEC PC-98.
;
                ASSUME  DS:NOTHING
                TALIGN  4
RM_TIMER        PROC    NEAR
                PUSH    EAX
                PUSH    DS
                MOV     AX, SV_DATA
                MOV     DS, AX
                ASSUME  DS:SV_DATA
;
; Update clock for __clock()
;
                ADD     CLOCK_LO, 1
                ADC     CLOCK_HI, 0
;
; Decrement timer for emergency exit
;
                CMP     EMERGENCY_TIMER, 0
                JE      SHORT RMT_SKIP_EMERGENCY
                DEC     EMERGENCY_TIMER
                TALIGN  4
RMT_SKIP_EMERGENCY:
;
; Update software timers
;
                CMP     TIMER_MIN, 0
                JE      SHORT RMT_DONE
                INC     TIMER_TICKS
                DEC     TIMER_MIN
                JNZ     SHORT RMT_DONE
                CALL    RMT_CHECK
                TALIGN  4
RMT_DONE:       POP     DS
                ASSUME  DS:NOTHING
                POP     EAX
                JMP     OLD_RM_INT1C
RM_TIMER        ENDP

;
; At least one timer expired; examine all timers
;
                TALIGN  4
                ASSUME  DS:SV_DATA
RMT_CHECK       PROC    NEAR
                PUSH    BX
                PUSH    CX
                PUSH    EDX
                PUSH    DI
                MOV     CX, MAX_TIMERS
                LEA     BX, TIMER_TABLE
                ASSUME  BX:PTR TIMER
                XOR     EDX, EDX
                MOV     TIMER_MIN, EDX
                XCHG    EDX, TIMER_TICKS
RMT_LOOP:       CMP     [BX].T_TYPE, TT_RUNNING
                JB      SHORT RMT_NEXT
                SUB     [BX].T_TICKS, EDX
                JA      SHORT RMT_MIN
                MOV     AX, TT_EXPIRED
                XCHG    AX, [BX].T_TYPE
                CMP     AX, TT_TERMIO_TIME
                JE      SHORT RMT_TERMIO
                CMP     AX, TT_SLEEP
                JE      SHORT RMT_SLEEP
                CMP     AX, TT_ALARM
                JNE     SHORT RMT_NEXT
RMT_ALARM:      MOV     DI, [BX].T_PROCESS
                ASSUME  DI:PTR PROCESS
                CMP     [DI].P_SIG_HANDLERS[4*SIGALRM], SIG_IGN
                JE      SHORT RMT_NEXT
                BTS     [DI].P_SIG_PENDING, SIGALRM     ; Generate SIGALRM
                ASSUME  DI:NOTHING
                JMP     SHORT RMT_NEXT

RMT_TERMIO:     MOV     DI, [BX].T_PROCESS
                OR      (PROCESS PTR [DI]).P_FLAGS, PF_TERMIO_TIME
                JMP     SHORT RMT_NEXT

RMT_SLEEP:      MOV     DI, [BX].T_PROCESS
                OR      (PROCESS PTR [DI]).P_FLAGS, PF_SLEEP_FLAG
                JMP     SHORT RMT_NEXT

RMT_MIN:        MOV     EAX, [BX].T_TICKS
                CMP     TIMER_MIN, 0
                JE      SHORT RMT_SET
                CMP     EAX, TIMER_MIN
                JAE     SHORT RMT_NEXT
RMT_SET:        MOV     TIMER_MIN, EAX
RMT_NEXT:       ADD     BX, SIZE TIMER
                LOOP    RMT_LOOP
                ASSUME  BX:NOTHING
                POP     DI
                POP     EDX
                POP     CX
                POP     BX
                RET
RMT_CHECK       ENDP

              IF RM_DEBUGGER
;
; Debug interrupt in real mode
;
                ASSUME  DS:NOTHING
RM_DEBUG        PROC    NEAR
                MOV     AX, SV_DATA
                MOV     DS, AX
                ASSUME  DS:SV_DATA
                LEA     DX, $RM_DEBUG
                CALL    RTEXT
                MOV     BP, SP
                MOV     AX, [BP+2]
                CALL    RWORD
                MOV     AL, ":"
                CALL    RCHAR
                MOV     AX, [BP+0]
                CALL    RWORD
                CALL    RCRLF
                JMP     EXIT
RM_DEBUG        ENDP

              ENDIF

INIT_CODE       ENDS

                END
