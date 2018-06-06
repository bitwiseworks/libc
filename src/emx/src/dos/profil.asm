;
; PROFIL.ASM -- Implement profil()
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

__PROFIL        =       1
                INCLUDE EMX.INC
                INCLUDE SIGNAL.INC
                INCLUDE PROCESS.INC
                INCLUDE PMINT.INC
                INCLUDE PROFIL.INC
                INCLUDE ERRORS.INC

                PUBLIC  DO_PROFIL, PROFIL_SUSPEND, PROFIL_RESUME, PROFIL_TICK
                PUBLIC  PROFIL_COUNT

;
; I/O port addresses of the MC146818 RTC chip
;
RTC_ADDR        =       70H
RTC_DATA        =       71H


SV_DATA         SEGMENT
;
; Number of processes being profiled.  We don't need the RTC interrupt
; if the number is zero.
;
PROFIL_COUNT    DW      0

;
; The following two variables attempt to avoid lossage when using
; the kernel debugge on the profiling code.
;
PROFIL_SUSPENDED DB     FALSE
PROFIL_RESUMED  DB      FALSE

;
; The original values of registers 0A and 0B of the RTC chip.
;
OLD_RTC_REG0A   DB      ?
OLD_RTC_REG0B   DB      ?

SV_DATA         ENDS


SV_CODE         SEGMENT

                ASSUME  CS:SV_CODE, DS:NOTHING

;
; profil()
;
; In:  ES:ESI   Pointer to PROFIL structure
;      DI       Pointer to process table entry
;
; Out: EAX      errno
;
                TALIGN  4
                ASSUME  ESI:NEAR32 PTR PROFIL
                ASSUME  DI:PTR PROCESS
                ASSUME  DS:SV_DATA
DO_PROFIL       PROC    NEAR
                CMP     ES:[ESI].PRF_CB, SIZE PROFIL
                JNE     SHORT PROFIL_EINVAL
;
; Turn off profiling if scale <= 2 or bufsiz = 0
;
                CMP     ES:[ESI].PRF_SCALE, 1
                JBE     SHORT PROFIL_OFF
                CMP     ES:[ESI].PRF_BUFSIZ, 0
                JE      SHORT PROFIL_OFF
;
; Copy the values into the process table
;
                MOV     EAX, ES:[ESI].PRF_BUFF
                MOV     [DI].P_PRF_BUFF, EAX
                MOV     EAX, ES:[ESI].PRF_BUFSIZ
                AND     EAX, NOT 3
                MOV     [DI].P_PRF_BUFSIZ, EAX
                MOV     EAX, ES:[ESI].PRF_OFFSET
                MOV     [DI].P_PRF_OFFSET, EAX
                MOV     EAX, ES:[ESI].PRF_SCALE
                MOV     [DI].P_PRF_SCALE, EAX
;
; Start profiling
;
                CALL    PROFIL_START
                JC      SHORT PROFIL_EINVAL
                XOR     EAX, EAX
                RET

;
; Stop profiling
;
                TALIGN  4
PROFIL_OFF:     XOR     EAX, EAX
                XCHG    EAX, [DI].P_PRF_SCALE
                TEST    EAX, EAX
                JZ      SHORT PROFIL_OK
                CALL    PROFIL_STOP
PROFIL_OK:      XOR     EAX, EAX
                RET

PROFIL_EINVAL:  MOV     EAX, EINVAL
                RET
DO_PROFIL       ENDP

                ASSUME  ESI:NOTHING
                ASSUME  DI:NOTHING

;
; Start profiling one process
;
; Out:  CY      Error
;
                ASSUME  DS:SV_DATA
PROFIL_START    PROC    NEAR
                INC     PROFIL_COUNT
                CMP     PROFIL_COUNT, 1
                JNE     SHORT DONE
                CLI
                CALL    PROFIL_RESUME
                JNC     SHORT OK
                MOV     PROFIL_COUNT, 0
OK:             STI
DONE:           RET
PROFIL_START    ENDP

;
; Stop profiling one process
;
                ASSUME  DS:SV_DATA
PROFIL_STOP     PROC    NEAR
                CMP     PROFIL_COUNT, 0
                JE      SHORT DONE
                DEC     PROFIL_COUNT
                JNZ     SHORT DONE
                CLI
                CALL    PROFIL_SUSPEND
                STI
DONE:           RET
PROFIL_STOP     ENDP

;
; Timer tick for profiler, called by PMINT for IRQ8 if PROFIL_COUNT
; is non-zero
;
                TALIGN  4
                ASSUME  DS:SV_DATA
                ASSUME  BP:PTR ISTACKFRAME
PROFIL_TICK     PROC    NEAR
                MOV     AL, 0CH                 ; Read status register
                CALL    RTC_READ
                TEST    AL, 40H                 ; Periodic interrupt pending?
                JZ      SHORT DONE              ; No  -> done
                CMP     I_CS, L_CODE_SEL        ; Profile user code only
                JNE     SHORT DONE
                MOV     BX, PROCESS_PTR
                CMP     BX, NO_PROCESS          ; User process active?
                JE      SHORT DONE              ; No  -> done
                ASSUME  BX:PTR PROCESS
                CMP     [BX].P_PRF_SCALE, 0     ; Profiling that process?
                JE      SHORT DONE              ; No  -> done
                MOV     EAX, I_EIP              ; Compute counter offset
                SUB     EAX, [BX].P_PRF_OFFSET
                JC      SHORT DONE
                MUL     [BX].P_PRF_SCALE
                SHRD    EAX, EDX, 16
                AND     EAX, NOT 3
                CMP     EAX, [BX].P_PRF_BUFSIZ  ; Within buffer?
                JAE     SHORT DONE              ; No  -> done
                MOV     EDX, [BX].P_PRF_BUFF
                MOV     CX, L_DATA_SEL
                MOV     ES, CX
                INC     DWORD PTR ES:[EDX+EAX]  ; Increment counter
DONE:           MOV     AL, 20H                 ; EOI
                OUT     0A0H, AL                ; Slave PIC
                OUT     20H, AL                 ; Master PIC
                RET
PROFIL_TICK     ENDP
                ASSUME  BP:NOTHING
                ASSUME  BX:NOTHING


;
; Suspend generating profiler interrupts.
;
; This should be done before executing real-mode-code.  Call with
; interrupts disabled!
;
; Important: PROFIL_RESUME must have been called at least once!
;
                ASSUME  DS:SV_DATA
                TALIGN  4
PROFIL_SUSPEND  PROC    NEAR
                MOV     PROFIL_RESUMED, FALSE
                CMP     PROFIL_SUSPENDED, FALSE
                JNE     SHORT DONE
                MOV     PROFIL_SUSPENDED, NOT FALSE
;
; Disable the RTC interrupt (IRQ8) in the slave interrupt controller
;
                IN      AL, 0A1H
                OR      AL, 01H
                CALL    IO_DELAY
                OUT     0A1H, AL
;
; Stop the RTC from generating interrupts
;
                MOV     AL, 0BH
                MOV     AH, OLD_RTC_REG0B
                CALL    RTC_WRITE
;
; Restore the original interrupt rate
;
                MOV     AL, 0AH
                MOV     AH, OLD_RTC_REG0A
                CALL    RTC_WRITE
                CALL    IO_DELAY
                CALL    RTC_CLEAN
DONE:           RET
PROFIL_SUSPEND  ENDP


;
; Resume generating profiler interrupts.
;
; This should be done after executing real-mode-code.  Call with
; interrupts disabled!
;
; Out:  CY      Cannot enable profiler interrupts
;
                ASSUME  DS:SV_DATA
                TALIGN  4
PROFIL_RESUME   PROC    NEAR
                MOV     PROFIL_SUSPENDED, FALSE
                CMP     PROFIL_RESUMED, FALSE
                JNE     SHORT DONE
                MOV     PROFIL_RESUMED, NOT FALSE
;
; Read and save the current values of RTC registers 0A and 0B
;
                MOV     AL, 0AH
                CALL    RTC_READ
                MOV     OLD_RTC_REG0A, AL

                MOV     AL, 0BH
                CALL    RTC_READ
                MOV     OLD_RTC_REG0B, AL
;
; Don't profile if RTC interrupts are used by someone else
; or if square-wave output is enabled
;
                TEST    AL, 78H
                JNZ     SHORT FAIL
;
; Set the interrupt rate to 1024Hz
;
                MOV     AH, OLD_RTC_REG0A
                AND     AH, 0F0H
                OR      AH, 06H
                MOV     AL, 0AH
                CALL    RTC_WRITE
;
; Enable the RTC's periodic interrupt
;
                MOV     AH, OLD_RTC_REG0B
                OR      AH, 40H                 ; Enable periodic interrupt
                AND     AH, NOT 30H             ; Disable alarm & update-ended
                MOV     AL, 0BH                 ; interrupts
                CALL    RTC_WRITE
;
; Clear pending interrupts
;
                MOV     AL, 0CH
                CALL    RTC_READ
;
; Enable the RTC interrupt (IRQ8) in the slave interrupt controller
;
                IN      AL, 0A1H
                AND     AL, NOT 01H
                CALL    IO_DELAY
                OUT     0A1H, AL
DONE:           CLC
                RET

FAIL:           CALL    RTC_CLEAN
                STC
                RET
PROFIL_RESUME   ENDP


;
; Read an RTC register
;
; In:   AL      Register number
;
; Out:  AL      Register contents
;
; Note: This routine enables NMIs
;       Call with interrupts disabled!
;
        TALIGN  4
RTC_READ PROC   NEAR
        OUT     RTC_ADDR, AL
        CALL    IO_DELAY
        IN      AL, RTC_DATA
        RET
RTC_READ ENDP

;
; Write an RTC register
;
; In:   AL      Register number
;       AH      Value
;
; Note: This routine enables NMIs
;       Call with interrupts disabled!
;
        TALIGN  4
RTC_WRITE PROC  NEAR
        OUT     RTC_ADDR, AL
        CALL    IO_DELAY
        MOV     AL, AH
        OUT     RTC_DATA, AL
        RET
RTC_WRITE ENDP

;
; Set the RTC address register to 0CH
;
; (BIOS does this after reading or writing a RTC register.)
;
; Note: This routine enables NMIs
;
        TALIGN  4
RTC_CLEAN PROC  NEAR
        MOV     AL, 0CH
        OUT     RTC_ADDR, AL
        RET
RTC_CLEAN ENDP

;
; Delay between two accesses to I/O ports of the same chip
;
                TALIGN  4
IO_DELAY        PROC    NEAR
                MOV     CX, 4
                TALIGN  4
LOOP1:          DEC     CX
                JNZ     LOOP1
                RET
IO_DELAY        ENDP

SV_CODE         ENDS

                END
