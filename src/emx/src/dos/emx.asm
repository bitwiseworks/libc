;
; EMX.ASM -- Main module for emx.exe
;
; Copyright (c) 1991-1999 by Eberhard Mattes
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
; As a special exception, if you bind emx.exe to an executable file
; (using emxbind), this does not cause the resulting executable file
; to be covered by the GNU General Public License.  This exception
; does not however invalidate any other reasons why the executable
; file might be covered by the GNU General Public License.  However,
; if you bind a modified copy of emx.exe to an executable file, you
; have to include source code for emx.exe.
;
; If you modify this file, you have to remove the reference to the
; special exception from all the source files and replace the text
; between the outer <START_OF_EXCEPTION> and <END_OF_EXCEPTION> markers
; with the following paragraph (which currently does not apply as you
; have not modified this file):
;
; As a special exception, if you bind emx.exe to an executable file
; (using emxbind), this does not cause the resulting executable file
; to be covered by the GNU General Public License.  The source code
; for emx.exe must be distributed with the executable file.  This
; exception does not however invalidate any other reasons why the
; executable file might be covered by the GNU General Public License.
; <END_OF_EXCEPTION>
;

                INCLUDE EMX.INC
                INCLUDE TABLES.INC
                INCLUDE SEGMENTS.INC
                INCLUDE PAGING.INC
                INCLUDE OPRINT.INC
                INCLUDE VPRINT.INC
                INCLUDE MEMORY.INC
                INCLUDE PMIO.INC
                INCLUDE SIGNAL.INC
                INCLUDE PROCESS.INC
                INCLUDE SWAPPER.INC
                INCLUDE EXCEPT.INC
                INCLUDE RMINT.INC
                INCLUDE PMINT.INC
                INCLUDE VCPI.INC
                INCLUDE DPMI.INC
                INCLUDE RPRINT.INC
                INCLUDE DEBUG.INC
                INCLUDE LOADER.INC
                INCLUDE XMS.INC
                INCLUDE A20.INC
                INCLUDE TERMIO.INC
                INCLUDE VERSION.INC
                INCLUDE HEADERS.INC
                INCLUDE PMINT.INC
                INCLUDE OPTIONS.INC
                INCLUDE FILEIO.INC

                PUBLIC  SV_TOS, RM_TOS
                PUBLIC  FP_FLAG, FP_TMP, DOS_MAJOR, DOS_MINOR, ENV_EMXOPT
                PUBLIC  CMDL_OPTIONS, ENV_PATH, ENV_EMXPATH
                PUBLIC  PROT1
                PUBLIC  CLEANUP, EXIT, RM_OUT_OF_MEM

SV_DATA         SEGMENT

;
; Names of environment variables
;
$PATH           BYTE    "PATH", 0
$EMXPATH        BYTE    "EMXPATH", 0
$TMP            BYTE    "TMP", 0
$EMXTMP         BYTE    "EMXTMP", 0
$EMXOPT         BYTE    "EMXOPT", 0

;
; Program segment prefix and environment
;
                DALIGN  2

EMXL_SEG        WORD    0                       ; PSP segment of emxl
EMX_PSP         WORD    ?                       ; Our PSP segment
ENV_SEG         WORD    ?                       ; Environment segment
ENV_PATH        WORD    ?                       ; Pointer to PATH value
ENV_EMXPATH     WORD    ?                       ; Pointer to EMXPATH value
ENV_EMXOPT      WORD    ?                       ; Pointer to EMXOPT value
ENV_TOTAL_SIZE  WORD    ?                       ; Size of environment, w/ exe

;
; The arguments of the initial process
;
ARG_STRINGS     BYTE    256 DUP (?)             ; Arguments

;
; Floating point
;
FP_TMP          WORD    ?                       ; 287/387 CW or SW
FP_FLAG         BYTE    FP_NO                   ; No coprocessor

;
; This field is copied from HDR_SEG to SV_DATA
;
BIND_FLAG       BYTE    ?                       ; Bound application

;
; DOS version
;
DOS_MAJOR       BYTE    ?
DOS_MINOR       BYTE    ?

;
; Options copied from the command line.  Non-option arguments don't
; appear hear.  After starting the first process, CMDL_OPTIONS is
; cleared by zeroing the first byte.
;
CMDL_OPTIONS    BYTE    64 DUP (0)

;
; Messages
;
$WRONG_CPU      BYTE    "This program requires an 80386 CPU", CR, LF, 0
$WRONG_OPSYS    BYTE    "This program does not run in DOS mode of OS/2"
                BYTE    CR, LF, 0
$DOS_VERSION    BYTE    "This program requires DOS 3.0 or later", CR, LF, 0
$NO_A20         BYTE    "Cannot enable A20", CR, LF, 0
$RM_OUT_OF_MEM  BYTE    "Out of memory (RM)", CR, LF, 0

SV_DATA         ENDS


;
; This area will be patched by emxbind when using emx.exe as DOS stub.
;
HDR_SEG         SEGMENT
PATCH           LABEL   BIND_HEADER
                BYTE    "emx ", VERSION, 0
                BYTE    (SIZE BIND_HEADER - HDR_VERSION_LEN) DUP (0)
HDR_SEG         ENDS


                .386P

SV_CODE         SEGMENT

                ASSUME  CS:SV_CODE, DS:NOTHING

;
; This is the proteced mode entry point
;
PROT1           LABEL   FAR
                MOV     AX, G_SV_DATA_SEL
                MOV     DS, AX
                ASSUME  DS:SV_DATA
                MOV     ES, AX
                MOV     FS, AX
                MOV     GS, AX
                MOV     AX, G_SV_STACK_SEL
                MOV     SS, AX
                LEA     ESP, SV_TOS
                AND     TSS_BUSY, NOT 2
                MOV     AX, G_TSS_SEL
                LTR     AX
;
; This must be done before any interrupt can be handled
; (SLDT, LLDT in PMINT.ASM)
;
                MOV     AX, G_LDT_SEL           ; Minimal LDT
                LLDT    AX                      ; Load LDTR
;
; Protected mode established
;
                CALL    DEBUG_INIT
                XOR     EAX, EAX
                MOV     DR6, EAX                ; Clear debug status register
                CALL    INIT_FP
;
; From now onwards, we can switch back to real mode
;
; Enable paging
;
                MOV     PAGING, 1               ; Paging enabled
                CMP     VCPI_FLAG, FALSE        ; VCPI?
                JNE     SHORT PROT2             ; Yes -> paging already enabled
                MOV     EAX, PAGE_DIR_PHYS
                MOV     CR3, EAX                ; Load CR3
                MOV     EAX, CR0
                OR      EAX, CR0_PG             ; Enable paging
                MOV     CR0, EAX
;
; Further initializations
;
PROT2:          STI                             ; Enable interrupts
                CALL    INIT_LIN_MAP            ; Enable segment creation
                CALL    INIT_PAGE_BMAP          ; Initialize freed page bitmap
                CALL    INIT_SWAP               ; Enable swapping
                CALL    INIT_TSS                ; Initialize new TSS
                CALL    INIT_DBCS               ; Get DBCS lead bytes
;
; Create GDT entry for environment segment
;
                MOVZX   EBX, ENV_SEG
                SHL     EBX, 4
                MOVZX   ECX, ENV_TOTAL_SIZE
                LEA     SI, G_ENV_DESC
                MOV     AX, A_DATA32 OR DPL_0
                CALL    CREATE_SEG
;
; Initialize termio structure for stdin (global data for all processes)
;
                LEA     BX, STDIN_TERMIO
                CALL    TERMIO_INIT
                MOV     STDIN_FL, 0
;
; Load program
;
                LEA     SI, ARG_STRINGS + 1     ; argv[0]
                CALL    FIND_EXEC
                MOV     NP1.NP_FNAME_OFF, EDX
                MOV     NP1.NP_FNAME_SEL, DS
                LEA     AX, PROC0               ; Dummy parent process
                MOV     NP1.NP_PARENT, AX
                MOV     NP1.NP_FPROC, AX
                MOV     NP1.NP_MODE1, NP_SPAWN_SYNC
                MOV     NP1.NP_MODE2, 0
                LEA     SI, NP1                 ; Arguments
                CALL    NEW_PROCESS             ; Load the program
                OR      AX, AX                  ; Ok?
                JZ      SHORT LOAD3
                CALL    OTEXT                   ; Display error message
                MOV     AX, 4C01H               ; Quit
                INT     21H
                JMP     SHORT $                 ; Never reached

                ASSUME  DI:PTR PROCESS
LOAD3:          MOV     CMDL_OPTIONS[0], 0      ; Ignore options from now on
                MOV     [DI].P_STATUS, PS_RUN   ; Process is running!
                MOV     PROCESS_PTR, DI         ; Current process (I/O etc.)
                MOV     PROCESS_SIG, DI         ; Current process (Ctrl-Break)
;
; Now DS:DI points to the process table entry
;
; Switch to 32 bit code/data
;
                CMP     STEP_FLAG, FALSE        ; Debugging program?
                JE      SHORT DISPATCH_1        ; No -> skip
                MOV     DX, L_CODE_SEL          ; Set breakpoint
                MOV     EAX, [DI].P_ENTRY_POINT ; on entry point
                CALL    SET_BREAKPOINT
                CALL    INS_BREAKPOINTS
DISPATCH_1:     MOV     EAX, 0002H              ; This bit is always 1
                PUSH    EAX
                POPFD                           ; Clear all flag bits
                LLDT    [DI].P_LDT              ; Use LDT
; Stack empty!
                MOV     EAX, L_DATA_SEL
                PUSH    EAX                     ; SS
                MOV     EBX, [DI].P_ESP
                PUSH    EBX                     ; ESP
                MOV     EAX, 0202H              ; IOPL=0, IF=1
                CMP     STEP_FLAG, FALSE        ; Single stepping?
                JE      SHORT DISPATCH_2        ; No -> skip
                OR      EAX, FLAG_TF            ; Set trap flag
DISPATCH_2:     PUSH    EAX                     ; EFLAGS
                MOV     EAX, L_CODE_SEL
                PUSH    EAX                     ; CS
                MOV     EBX, [DI].P_ENTRY_POINT
                PUSH    EBX                     ; EIP
                MOV     AX, L_DATA_SEL
                MOV     DS, AX
                ASSUME  DS:NOTHING
                MOV     ES, AX
                MOV     FS, AX
                MOV     GS, AX
                IRETD

                ASSUME  DI:NOTHING

;
; We must not use FWAIT if there is no coprocessor. We use this loop
; instead.
;
DELAY_FP        MACRO
                MOV     ECX, 50
                LOOP    $
                ENDM

;
; Initialize CR0 for floating point math (287, 387, or emulator)
;
; Note: ET set by reset?
;
                ASSUME  DS:SV_DATA
INIT_FP         PROC    NEAR
                MOV     EAX, CR0                ; Get current CR0
                AND     EAX, CR0_FPU            ; Keep only FPU related bits
                MOV     RM_CR0, EAX             ; Save it
                MOV     EAX, CR0                ; Get current CR0
                AND     EAX, NOT (CR0_TS OR CR0_EM)
                OR      EAX, CR0_MP             ; Assume FPU present
                MOV     CR0, EAX
;
; Now check for coprocessor
;
                CMP     FP_IGNORE, FALSE        ; Check for coprocessor?
                JNE     SHORT INIT_FP1          ; No  -> skip check
                FNINIT
                DELAY_FP                        ; Delay
                MOV     FP_TMP, 0               ; 387 FNINIT sets CW to 037FH
                FNSTCW  FP_TMP                  ; Copy CW to memory
                DELAY_FP                        ; Delay
                MOV     AX,  FP_TMP             ; Now check CW
                AND     AX, 0F3FH               ; Keep PC and exception masks
                CMP     AX, 033FH               ; Initialized?
                JNE     SHORT INIT_FP1          ; No  -> no x87 coprocessor
                MOV     FP_TMP, 0FFFFH          ; 387 FNINIT: SW &= 0x4700
                FNSTSW  FP_TMP                  ; Copy SW to memory
                DELAY_FP                        ; Delay
                TEST    FP_TMP, NOT 47C0H       ; Initialized?
                JNZ     SHORT INIT_FP1          ; No  -> no x87 coprocessor
;
; We have found *some* coprocessor (87, 287, 387, ...)
;
; If 1/0 equals -(1/0), we are dealing with a 287: FNINIT sets the 287
; to projective mode, ie, -inf = +inf, whereas the 387 always uses
; affine mode (-inf < +inf).
;
                MOV     FP_FLAG, FP_287         ; ST   ST(1)
                FLD1                            ; 1.0
                FLDZ                            ; 0.0  1.0
                FDIV                            ; +inf
                FLD     ST                      ; +inf +inf
                FCHS                            ; -inf +inf  (287: +inf +inf)
                FCOMPP                          ; (stack empty)
                FSTSW   AX                      ; Status to AX
                SAHF                            ; 387: NZ, 287: ZR
                JE      SHORT INIT_FP1          ; 287 found -> skip
                MOV     FP_FLAG, FP_387         ; 387 found
INIT_FP1:
;
; Now set CR0
;
                MOV     EAX, CR0
                AND     EAX, NOT CR0_FPU
                CMP     FP_FLAG, FP_387         ; 387 coprocessor?
                JNE     SHORT IFP_EMU           ; No  -> setup for emulation
                OR      EAX, CR0_TS OR CR0_MP   ; Task switched, math present
                JMP     SHORT IFP_SET
IFP_EMU:        OR      EAX, CR0_TS OR CR0_EM   ; Task switched, emulate
IFP_SET:        MOV     CR0, EAX                ; Set CR0
                AND     EAX, CR0_FPU            ; Keep only FPU bits
                MOV     PM_CR0, EAX             ; Save for switching to PM
                RET                             ; Done
INIT_FP         ENDP

;
; Initialize DBCS lead byte table.  Note that we can't call GET_DBCS_LEAD
; directly.
;
INIT_DBCS       PROC    NEAR
                PUSH    DS
                MOV     EDX, 0
                MOV     DS, DX
                MOV     AX, 7F5AH
                INT     21H
                POP     DS
                RET
INIT_DBCS       ENDP

SV_CODE         ENDS


SV_STACK        SEGMENT
                WORD    2048 DUP (?)
SV_TOS          LABEL   WORD
SV_STACK        ENDS



RM_STACK        SEGMENT
                WORD    1024 DUP (?)
RM_TOS          LABEL   WORD
RM_STACK        ENDS


INIT_CODE       SEGMENT

                ASSUME  CS:INIT_CODE



;
; Use backward jump to this code to make sure the prefetch queue
; will be flushed
;
START_4:        JMPF16  G_SV_CODE_SEL, PROT1

;
; Entrypoint
;

                ASSUME  DS:NOTHING
                .8086
START:          MOV     AX, SV_DATA
                MOV     DS, AX
                ASSUME  DS:SV_DATA
                MOV     EMX_PSP, ES             ; Save PSP segment

                CALL    EMXBIND
                CALL    CHECK_CPU               ; 386 CPU required
                CALL    CHECK_OPSYS             ; Check operating system

                .386P

                CALL    CHECK_EMXL              ; Run by emxl?
                CALL    ENVIRONMENT             ; Read environment
                CALL    ARGS                    ; Read command line arguments
                CALL    GET_PATH                ; Get PATH/EMXPATH variables
                CALL    GET_TMP                 ; Get TMP/EMXTMP variables
                CALL    CHECK_DV                ; Check for DESQview
                CALL    PROCESS_INIT            ; Initialize emx processes
                CALL    HOOK_INT                ; Hook interrupts
                CALL    CHECK_VCPI              ; Check for VCPI
                CALL    CHECK_DPMI              ; Check for DPMI
                                                ; (After CHECK_VCPI)
                CALL    CHECK_VM
                CALL    CHECK_XMS               ; Check for XMS
                                                ; (After CHECK_VCPI)
                CALL    INIT_TERMIO             ; Get keyboard type
                CALL    INIT_A20                ; Find out which machine
                                                ; (after CHECK_XMS)
                CALL    INIT_BUFFER             ; Allocate buffers
                CALL    INIT_MEMORY             ; Initialize memory allocation
                CALL    INIT_PAGING             ; Initialize page tables
                                                ; (after CHECK_VCPI, INIT_MEMORY)
                CALL    INIT_ADDR               ; Initialize physical/linear
                                                ; addresses (after INIT_MEMORY,
                                                ; INIT_PAGING)
                CALL    INIT_MEM_PHYS
                CALL    INIT_VIDEO              ; Init video segments
                CMP     XMS_FLAG, FALSE
                JE      SHORT START_2
                CALL    INIT_XMS                ; After CHECK_VCPI, INIT_MEMORY
START_2:        CALL    INIT_BASES              ; After INIT_PAGING
                CMP     HIMEM_COUNT, 0          ; Extended memory used?
                JE      SHORT START_3           ; No  -> skip
                CALL    INIT_HIMEM              ; Update high memory table
                CALL    A20_ON                  ; Enable address line A20
                CALL    CHECK_A20               ; Check if A20 is working
                STI                             ; CHECK_A20 disables interrupts
                OR      AX, AX                  ; Bad news?
                JNE     SHORT START_3           ; No - > skip
                LEA     DX, $NO_A20             ; Cannot enable A20
                CALL    RTEXT
                MOV     AL, 0FFH
                JMP     SHORT EXIT
START_3:        CALL    INIT_FILEIO             ; Initialize file handles
                CALL    INIT_INT                ; Initialize interrupts
                CALL    INIT_TIMER              ; Initialize timer interrupt

                CMP     VCPI_FLAG, FALSE
                JNE     START_VCPI
                LGDT    GDT_PTR
                LIDT    IDT_PTR
                MOV     EAX, CR0
                OR      AL, CR0_PE              ; Enable protected mode
                MOV     CR0, EAX
                JMP     START_4                 ; Flush prefetch queue


;
; Cleanup and exit to DOS
;
                ASSUME  DS:NOTHING
EXIT:           PUSH    AX
                CALL    CLEANUP
                ASSUME  DS:SV_DATA
                POP     AX
                STI
                MOV     AH, 4CH
                INT     21H
                JMP     SHORT $                 ; Never reached


;
; Convert some real-mode addresses to physical or linear addresses
;
                ASSUME  DS:SV_DATA
INIT_ADDR       PROC    NEAR
                LEA     DI, GDT_LIN
                CALL    INIT_LIN_1
                LEA     DI, IDT_LIN
                CALL    INIT_LIN_1
                LEA     DI, V2P_GDTR
                CALL    INIT_LIN_1
                LEA     DI, V2P_IDTR
                CALL    INIT_LIN_1
                LEA     DI, V2P_LIN
                CALL    INIT_LIN_1
                LEA     DI, LOMEM_HEAD_PHYS
                CALL    INIT_PHYS_1
                LEA     DI, HIMEM_HEAD_PHYS
                CALL    INIT_PHYS_1
                RET
INIT_ADDR       ENDP

;
; Convert one real-mode address to a physical address
;
; Note: the object pointed to by the address must be contained completely
;       in *one* physical page.
;
; In:   DI      Pointer to a DWORD which contains in the lower word the
;               real-mode offset
;
INIT_PHYS_1     PROC    NEAR
                MOV     DX, [DI]                ; Get real-mode offset
                MOV     AX, SV_DATA             ; Use this segment
                CALL    RM_TO_PHYS              ; Convert to physical address
                MOV     [DI], EAX               ; Store physical address
                RET
INIT_PHYS_1     ENDP

;
; Convert one real-mode address to a linear address
;
; Note: the object pointed to by the address must not be accessed with
;       G_PHYS_SEL. (Why?)
;
; In:   DI      Pointer to a DWORD which contains in the lower word the
;               real-mode offset
;
INIT_LIN_1      PROC    NEAR
                XOR     EAX, EAX
                MOV     AX, SV_DATA             ; Use this segment
                SHL     EAX, 4
                MOVZX   EDX, WORD PTR [DI]      ; Get real-mode offset
                ADD     EAX, EDX
                MOV     [DI], EAX               ; Store linear address
                RET
INIT_LIN_1      ENDP


;
; Put the base addresses of some segments into the GDT. This is done
; by using the BASE_TABLE table. Each table entry consists of two (16-bit)
; words: The first word is the real-mode segment, the second word points
; to the GDT entry, which already contains the offset in the base 0..15 field.
;

                ASSUME  DS:SV_DATA
INIT_BASES      PROC    NEAR
                LEA     SI, BASE_TABLE          ; Table of segment/offset pairs
IB_LOOP:        MOV     AX, [SI+0]              ; Get real-mode segment
                OR      AX, AX                  ; End of table?
                JZ      SHORT IB_END            ; Yes -> done
                MOV     DI, [SI+2]              ; Offset of GDT entry (SV_DATA)
                CALL    INIT_DESC               ; Set base field
                ADD     SI, 4                   ; Next table entry
                JMP     SHORT IB_LOOP           ; Repeat for all entries
IB_END:         RET                             ; Done
INIT_BASES      ENDP


;
; If called by emxl.exe, the command line is formatted this way:
;
;   -/xxxx/
;
; where xxxx is the (hexadecimal) PSP segment of emxl.exe.  Note that there
; is no blank before the dash.  If called by emxl.exe, we used the (patched)
; options and the command line arguments of emxl.exe -- under DOS we can
; access memory of other `processes'.
;

                ASSUME  DS:SV_DATA
CHECK_EMXL      PROC    NEAR
                MOV     ES, EMX_PSP             ; Access PSP (command line)
                MOV     DI, 80H                 ; Point to length field
                MOV     AL, 7                   ; Length must be 7
                SCAS    BYTE PTR ES:[DI]
                JNE     SHORT CE_RET
                MOV     AL, "-"                 ; Check for "-/"
                SCAS    BYTE PTR ES:[DI]
                JNE     SHORT CE_RET
                MOV     AL, "/"
                SCAS    BYTE PTR ES:[DI]
                JNE     SHORT CE_RET
                MOV     DX, 0
                MOV     CX, 0404H
CE_1:           MOV     AL, ES:[DI]
                INC     DI
                SUB     AL, "0"
                JB      SHORT CE_RET
                CMP     AL, 10
                JB      SHORT CE_2
                SUB     AL, "A" - ("0" + 10)
                CMP     AL, 10
                JB      SHORT CE_RET
                CMP     AL, 15
                JA      SHORT CE_RET
CE_2:           SHL     DX, CL
                OR      DL, AL
                DEC     CH
                JNZ     SHORT CE_1
                MOV     AL, "/"                 ; Check for "/", CR
                SCAS    BYTE PTR ES:[DI]
                JNE     SHORT CE_RET
                MOV     AL, CR
                SCAS    BYTE PTR ES:[DI]
                JNE     SHORT CE_RET
;
; Check for emx signature
;
                MOV     DI, 100H                ; Data segment of emxl
                PUSH    DS                      ; Save DS
                MOV     AX, HDR_SEG
                MOV     DS, AX
                ASSUME  DS:NOTHING
                LEA     SI, PATCH.BND_SIGNATURE
                MOV     CX, HDR_EMX_LEN
                CLD
                REPE    CMPS BYTE PTR DS:[SI], BYTE PTR ES:[DI]
                POP     DS                      ; Restore DS
                ASSUME  DS:SV_DATA
                JNE     SHORT CE_RET
                MOV     EMXL_SEG, DX            ; It looks right
CE_RET:         RET
CHECK_EMXL      ENDP

;
; Parse EMXOPT and command line
;
; When run manually (or by an old version of emxl), process the following
; items (note that we don't know about the emxbind patch area here):
;
;   1/ EMXOPT environment variable
;   2/ command line

; When run by emxl.exe, process the following items:
;
;   1/ emxbind patch area of emxl.exe
;   2/ EMXOPT environment variable
;   3/ command line of emxl.exe
;
; When bound with an a.out file (without emxl.exe), process the following
; items:
;
;   1/ emxbind patch area of this program
;   2/ EMXOPT environment variable
;
                ASSUME  DS:SV_DATA

ARGS            PROC    NEAR
                MOV     NP1.NP_ARG_COUNT, 0     ; No arguments
                MOV     AX, EMXL_SEG
                OR      AX, AX
                JNZ     SHORT ARGS_01
                CMP     BIND_FLAG, FALSE        ; Bound?
                JE      SHORT ARGS_10           ; No  -> skip
                MOV     AX, HDR_SEG
                LEA     SI, PATCH.BND_OPTIONS
                JMP     SHORT ARGS_02
ARGS_01:        LEA     SI, (BIND_HEADER PTR ES:[100H]).BND_OPTIONS
ARGS_02:        MOV     ES, AX
                CALL    RM_OPTIONS              ; Parse options in patch area
                CALL    RM_SKIP_BLANKS
                OR      AL, AL                  ; End of string?
                JNZ     USAGE                   ; No  -> error
;
; Process options in the EMXOPT environment variable
;
ARGS_10:        LEA     BX, $EMXOPT
                CALL    GETENV                  ; Get EMXOPT environment var.
                OR      DI, DI                  ; Found?
                JZ      SHORT ARGS_11           ; No  -> skip
                MOV     SI, DI
                CALL    RM_OPTIONS              ; Parse options in EMXOPT
                CALL    RM_SKIP_BLANKS
                OR      AL, AL                  ; End of string?
                JNZ     USAGE                   ; No  -> error
;
; Process command line
;
ARGS_11:        MOV     AX, EMXL_SEG            ; Try emxl's PSP
                OR      AX, AX                  ; Run by emxl.exe?
                JNZ     SHORT ARGS_12           ; Yes -> use emxl's cmd line
                MOV     AX, EMX_PSP             ; Use our PSP
ARGS_12:        MOV     ES, AX                  ; Access PSP (command line)
                MOV     SI, 81H                 ; Start of command line
                MOV     BL, ES:[SI-1]           ; Fetch length of command line
                MOV     BH, 0
                MOV     BYTE PTR ES:[SI+BX], 0  ; Terminate string with 0
                CALL    RM_SKIP_BLANKS          ; Skip leading blanks
                CMP     BIND_FLAG, FALSE        ; Bound?
                JNE     SHORT ARGS_13           ; Yes -> don't read options
                CMP     EMXL_SEG, 0             ; Run by new emxl?
                JNE     SHORT ARGS_13           ; Yes -> don't read options
                PUSH    SI
                CALL    RM_OPTIONS              ; Parse options
                POP     AX
                PUSH    SI                      ; Save pointer
                MOV     CX, SI                  ; Copy options
                SUB     CX, AX
                CMP     CX, SIZE CMDL_OPTIONS
                JAE     USAGE
                XCHG_DS_ES
                ASSUME  DS:NOTHING
                MOV     SI, AX
                LEA     DI, CMDL_OPTIONS
                CLD
                REP     MOVSB
                XOR     AL, AL
                STOSB
                XCHG_DS_ES
                ASSUME  DS:SV_DATA
                POP     SI                      ; Restore pointer
ARGS_13:        CALL    NAMES                   ; Parse arguments
                RET
ARGS            ENDP


;
; Parse command line for user program
;
; In:  ES:SI    Pointer to command line (null terminated)
;
NAMES           PROC    NEAR
                XOR     EDI, EDI
                LEA     DI, ARG_STRINGS         ; Put arguments here
                MOV     NP1.NP_ARG_OFF, EDI
                MOV     NP1.NP_ARG_SEL, G_SV_DATA_SEL
                CMP     EMXL_SEG, 0             ; Run by emxl?
                JNE     SHORT NAMES0            ; Yes -> copy argv[0]
                CMP     BIND_FLAG, FALSE        ; Bound?
                JE      SHORT NAMES1            ; No  -> skip
;
; Copy argv[0]
;
NAMES0:         MOV     FS, ENV_SEG
                MOV     BX, NP1.NP_ENV_SIZE     ; End of environment
                ADD     BX, 2                   ; Skip word count
                MOV     BYTE PTR [DI], 80H      ; No flags set
                INC     DI                      ; Skip flags byte
COPY_ARGV0:     MOV     AL, FS:[BX]             ; Copy program name
                MOV     DS:[DI], AL
                INC     BX
                INC     DI
                OR      AL, AL                  ; End of program name?
                JNZ     COPY_ARGV0              ; No  -> carry on copying
                INC     NP1.NP_ARG_COUNT        ; One argument
;
; Command line
;
NAMES1:         CALL    RM_SKIP_BLANKS          ; Skip blanks
                OR      AL, AL                  ; End of command line?
                JZ      SHORT NAMES9            ; Yes -> done
;
; Parse one argument
;
; "..."                         Quoted string, may contain blanks
; \"                            Odd number 2n+1 of \ followed by " is read as
;                               n backslashes followed by "
; \\"                           Even number 2n of \ followed by " is read as
;                               n backslashes followed by start/end of string
;
NAMES2:         MOV     CX, 0                   ; Number of backslashes := 0
                MOV     DL, 0                   ; Not within quoted string
                MOV     DH, 00H                 ; No quotes used
                INC     NP1.NP_ARG_COUNT        ; We've found an argument
                MOV     BX, DI                  ; Save pointer to flags
                INC     DI                      ; Space for flags
NAMES3:         MOV     AL, ES:[SI]             ; Fetch character
;
; First test for characters which don't automatically insert backslashes
;
                CMP     AL, "\"                 ; Backslash?
                JE      SHORT NAMES_BACKSLASH   ; Yes -> special treatment
                CMP     AL, '"'                 ; Quote?
                JE      SHORT NAMES_QUOTE       ; Yes -> special treatment
                JCXZ    NAMES5
NAMES4:         MOV     BYTE PTR DS:[DI], "\"
                INC     DI
                LOOP    NAMES4
NAMES5:         OR      AL, AL                  ; End of command line?
                JZ      SHORT NAMES_END         ; Yes -> special treatment
                CMP     AL, " "                 ; Blank?
                JE      SHORT NAMES_WHITE       ; Yes -> special treatment
                CMP     AL, TAB                 ; Tab?
                JE      SHORT NAMES_WHITE       ; Yes -> special treatment
NAMES_PUT:      MOV     DS:[DI], AL             ; Store character
                INC     DI                      ; Next destination location
                INC     SI                      ; Skip to next character
                JMP     SHORT NAMES3            ; Repeat

NAMES_BACKSLASH:INC     CX                      ; Count backslashes
                INC     SI                      ; Skip to next character
                JMP     SHORT NAMES3            ; Repeat

NAMES_WHITE:    OR      DL, DL                  ; Within string?
                JNZ     SHORT NAMES_PUT         ; Yes -> store character
NAMES_END:      OR      DH, 80H                 ; Store flags (non-zero,
                MOV     DS:[BX], DH             ; otherwise end of string!)
                MOV     BYTE PTR DS:[DI], 0     ; End of argument
                INC     DI                      ; Next destination location
                JMP     SHORT NAMES1            ; Skip blanks, check for end

NAMES_QUOTE:    SUB     CX, 2                   ; Insert CX/2 backslashes
                JB      SHORT NAMES_Q1
                MOV     BYTE PTR DS:[DI], "\"
                INC     DI
                JMP     SHORT NAMES_QUOTE

NAMES_Q1:       TEST    CX, 1                   ; Odd CX?
                MOV     CX, 0
                JNZ     SHORT NAMES_PUT         ; Yes -> store quote
                OR      DH, 01H                 ; Quotes used
                NOT     DL                      ; Toggle quote flag
                INC     SI                      ; Skip to next character
                JMP     SHORT NAMES3            ; Repeat

NAMES9:         SUB     DI, OFFSET SV_DATA:ARG_STRINGS
                MOV     NP1.NP_ARG_SIZE, DI
                CMP     NP1.NP_ARG_COUNT, 1     ; Program name given?
                JB      USAGE                   ; No  -> complain
                RET

NAMES           ENDP

;
; Read the environment, compute the size and number of variables
;
; We use emxl's environment if we're run by emxl.exe.  This way, we
; get the executable path name of the program bound with emxl.
;
;
ENVIRONMENT     PROC    NEAR
                MOV     AX, EMXL_SEG            ; Get emxl's PSP segment
                OR      AX, AX                  ; Run by emxl.exe?
                JNZ     SHORT ENV_1             ; Yes -> use emxl's environment
                MOV     AX, EMX_PSP             ; No: use our environment
ENV_1:          MOV     ES, AX
                MOV     BX, ES:[2CH]            ; Get segment of environment
                MOV     ENV_SEG, BX
                MOV     ES, BX
                MOV     NP1.NP_ENV_OFF, 0
                MOV     NP1.NP_ENV_SEL, G_ENV_SEL
                MOV     NP1.NP_ENV_SIZE, 0
                MOV     NP1.NP_ENV_COUNT, 0
                MOV     ENV_TOTAL_SIZE, 0
                CALL    SCAN_ENV
                MOV     NP1.NP_ENV_SIZE, BX
                MOV     NP1.NP_ENV_COUNT, DX
                MOV     ENV_TOTAL_SIZE, DI
;
; Build EMX_DIR which is the path name of the directory from which
; emx.exe was run or loaded.  If emx.exe was bound to the program,
; EMX_DIR will be empty.
;
                CMP     BIND_FLAG, FALSE        ; Bound?
                JE      SHORT EMXDIR_1          ; No -> scan environment
                CMP     EMXL_SEG, 0             ; Run by emxl.exe?
                JZ      SHORT EMXDIR_FAIL       ; No -> EMX_DIR empty
EMXDIR_1:       MOV     ES, EMX_PSP
                MOV     ES, ES:[2CH]            ; Get segment of environment
                CALL    SCAN_ENV
                TEST    BX, BX
                JZ      SHORT EMXDIR_FAIL
;
; Find the length of the directory name
;
                ADD     BX, 2                   ; Skip word count
                MOV     SI, BX                  ; Save start address
                MOV     CX, BX                  ; Points to last delim + 1
EMXDIR_LOOP:    MOV     AL, ES:[BX]             ; Fetch next character
                TEST    AL, AL                  ; End of string?
                JZ      SHORT EMXDIR_EOS        ; Yes -> done
                CMP     AL, ":"                 ; Delimiter?
                JE      SHORT EMXDIR_DELIM      ; Yes -> remember position
                CMP     AL, "\"                 ; Delimiter?
                JE      SHORT EMXDIR_DELIM      ; Yes -> remember position
                CMP     AL, "/"                 ; Delimiter? (Can this happen?)
                JE      SHORT EMXDIR_DELIM      ; Yes -> remember position
                INC     BX                      ; Skip the character
                JMP     SHORT EMXDIR_LOOP       ; Loop again

EMXDIR_DELIM:   INC     BX                      ; Skip the character
                MOV     CX, BX                  ; Remember position
                JMP     SHORT EMXDIR_LOOP       ; Loop again

EMXDIR_EOS:     SUB     CX, SI                  ; Is there a directory?
                JZ      EMXDIR_CWD              ; No  -> use current directory
;
; The program name includes a directory.  For simplicity, assume
; it's an absolute path name.
;
                LEA     DI, EMX_DIR
EMXDIR_COPY:    MOV     AL, ES:[SI]
                MOV     [DI], AL
                INC     SI
                INC     DI
                LOOP    EMXDIR_COPY
                MOV     BYTE PTR [DI], 0
                JMP     SHORT EMXDIR_DONE

;
; The program name does not include a directory.  Use the current
; directory.
;
EMXDIR_CWD:     MOV     AH, 19H                 ; Get current disk
                INT     21H
                ADD     AL, "A"
                MOV     EMX_DIR[0], AL
                MOV     EMX_DIR[1], ":"
                MOV     EMX_DIR[2], "\"
                LEA     SI, EMX_DIR + 3
                MOV     DL, 0                   ; Current drive
                MOV     AH, 47H                 ; Get current directory
                INT     21H
                JC      SHORT EMXDIR_FAIL
;
; Add a backslash to the end if required
;
                LEA     SI, EMX_DIR + 2
EMXDIR_BS:      LODS    BYTE PTR DS:[SI]
                TEST    AL, AL
                JNZ     SHORT EMXDIR_BS
                CMP     BYTE PTR [SI-2], "\"
                JE      SHORT EMXDIR_DONE
                MOV     BYTE PTR [SI-1], "\"
                MOV     BYTE PTR [SI-0], 0
                JMP     SHORT EMXDIR_DONE

EMXDIR_FAIL:    MOV     EMX_DIR[0], 0
EMXDIR_DONE:    RET
ENVIRONMENT     ENDP


;
; In:  ES       Segment of environment
;
; Out: DX       Number of environment variables
;      BX       Pointer to end of environment
;      DI       Pointer to end of program name
;
SCAN_ENV        PROC    NEAR
                MOV     CX, 8000H
                XOR     DX, DX
                XOR     DI, DI
                XOR     AL, AL
                CLD
                CMP     WORD PTR ES:[DI], 0
                JE      SHORT SE_2
SE_1:           INC     DX
                REPNE   SCAS BYTE PTR ES:[DI]
                JNE     SHORT SE_9
                SCAS    BYTE PTR ES:[DI]
                JE      SHORT SE_3
                LOOP    SE_1
                JMP     SHORT SE_9
SE_2:           INC     DI
SE_3:           MOV     BX, DI
                ADD     DI, 2                   ; Skip word count
                REPNE   SCAS BYTE PTR ES:[DI]
                JNE     SHORT SE_9
                RET
;
; Bad environment
;
SE_9:           XOR     DX, DX
                XOR     BX, BX
                XOR     DI, DI
                RET
SCAN_ENV        ENDP


;
; Get PATH and EMXPATH environment variables
;
GET_PATH        PROC    NEAR
                LEA     BX, $PATH
                CALL    GETENV
                MOV     ENV_PATH, DI
                LEA     BX, $EMXPATH
                CALL    GETENV
                MOV     ENV_EMXPATH, DI
                LEA     BX, $EMXOPT
                CALL    GETENV
                MOV     ENV_EMXOPT, DI
                RET
GET_PATH        ENDP

;
; Get EMXTMP environment variable. If EMXTMP is not set, TMP will
; be used instead.
;
GET_TMP         PROC    NEAR
                LEA     BX, $EMXTMP
                CALL    GETENV
                OR      DI, DI
                JNZ     SHORT GET_TMP1
                LEA     BX, $TMP
                CALL    GETENV
GET_TMP1:       CALL    SET_TMP_DIR
                RET
GET_TMP         ENDP


;
; Search environment
;
; In:   DS:BX   Pointer to name of environment variable
;
; Out:  DI=0    Variable not found
;       ES:DI   Pointer to value of environment variable
;
GETENV          PROC    NEAR
                MOV     ES, ENV_SEG
                XOR     DI, DI                  ; ES:DI points to environment
                CLD
GETENV_NEXT:    CMP     BYTE PTR ES:[DI], 0     ; Empty environment?
                JE      SHORT GETENV_FAILURE    ; Yes -> not found
                PUSH    BX                      ; Save pointer to name
GETENV_COMPARE: MOV     AL, [BX]
                CMP     AL, ES:[DI]             ; Compare names
                JNE     SHORT GETENV_DIFF       ; Mismatch -> try next one
                OR      AL, AL                  ; Shouldn't happen (`='!)
                JZ      SHORT GETENV_DIFF       ; (name matches completely)
                INC     BX
                INC     DI
                JMP     SHORT GETENV_COMPARE    ; Compare next character
GETENV_DIFF:    POP     BX                      ; Restore pointer to name
                OR      AL, AL                  ; End of name reached?
                JE      SHORT GETENV_EQUAL      ; Yes -> candidate found
GETENV_SKIP:    XOR     AL, AL
                MOV     ECX, 32767              ; Search for next entry
                REPNE   SCAS BYTE PTR ES:[DI]
                JMP     SHORT GETENV_NEXT       ; Check that entry

GETENV_EQUAL:   CMP     BYTE PTR ES:[DI], "="   ; Exact match?
                JNE     SHORT GETENV_SKIP       ; No  -> go to next entry
                INC     DI                      ; Skip `='
                JMP     SHORT GETENV_RET        ; Return pointer to value

GETENV_FAILURE: XOR     DI, DI                  ; Not found
GETENV_RET:     RET
GETENV          ENDP




;
; Read emxbind patch area
;
; Note: This code must use only 8086/88 instrutions
;
                ASSUME  DS:SV_DATA
                .8086
EMXBIND         PROC    NEAR
                MOV     AX, HDR_SEG
                MOV     ES, AX                  ; Access emxbind patch area
                ASSUME  ES:HDR_SEG
                MOV     AL, PATCH.BND_BIND_FLAG
                MOV     BIND_FLAG, AL           ; Copy to data segment
EMXBIND_RET:    RET
                ASSUME  ES:NOTHING
EMXBIND         ENDP

                .386P

;
; Abort if not running on a 386 or later
;
; Note: This code must use only 8086/88 instrutions
;
                ASSUME  DS:SV_DATA
                .8086
CHECK_CPU       PROC    NEAR
                PUSHF                           ; Save flags
                XOR     AX, AX                  ; Clear all bits
                PUSH    AX                      ; Clear all flag bits
                POPF
                STI                             ; Enable interrupts
                PUSHF                           ; Copy flags to AX
                POP     AX
                POPF                            ; Restore flags
                STI                             ; Enable interrupts
                NOT     AX                      ; Complement all bits
                AND     AX, 0F000H              ; Flags!=F000 -> 286/386/...
                JZ      WRONG_CPU               ; 86/186 -> error
                PUSHF                           ; Save flags
                MOV     AX,7000H                ; Set NT:=1, IOPL:=3
                PUSH    AX
                POPF
                STI                             ; Enable interrupts
                PUSHF                           ; Copy flags to AX
                POP     AX
                POPF                            ; Restore flags
                STI                             ; Enable interrupts
                AND     AX, 7000H               ; NT and IOPL sticky?
                JZ      WRONG_CPU               ; No  -> 286, error
                RET                             ; Ok, it's an 80386

WRONG_CPU:      LEA     DX, $WRONG_CPU          ; Display message: 386 required
                JMP     SHORT ABORT             ; and abort

CHECK_CPU       ENDP

                .386P

;
; Abort if unsupported operating system
;
                ASSUME  DS:SV_DATA
CHECK_OPSYS     PROC    NEAR
                MOV     AH, 30H
                INT     21H                     ; Get operating system version
                MOV     DOS_MAJOR, AL
                MOV     DOS_MINOR, AH
                CMP     AL, 10                  ; OS/2 1.x?
                JE      WRONG_OPSYS             ; Yes -> won't work
                CMP     AL, 3                   ; DOS 3.0 or later?
                JAE     OPSYS_OK                ; Yes -> ok
                CMP     BIND_FLAG, FALSE        ; Bound?
                JE      SHORT OPSYS_OK          ; No  -> ok
                LEA     DX, $DOS_VERSION        ; Display error message
                JMP     SHORT ABORT             ; and abort

OPSYS_OK:       RET

WRONG_OPSYS:    LEA     DX, $WRONG_OPSYS        ; Display error message
ABORT::         CALL    RTEXT
                MOV     AX, 4CFFH               ; Abort
                INT     21H
                JMP     SHORT $                 ; Never reached

CHECK_OPSYS     ENDP


;
; Check for DESQview
;
                ASSUME  DS:SV_DATA
CHECK_DV        PROC    NEAR
                MOV     AH, 2BH                 ; `Set current date'
                MOV     AL, 01H                 ; Subfunction: get version
                MOV     CX, 4445H               ; 'DE'
                MOV     DX, 5351H               ; 'SQ'
                INT     21H
                CMP     AL, 0FFH                ; DESQview installed?
                JE      SHORT CDV_RET           ; No -> return
                MOV     DV_FLAG, NOT FALSE
CDV_RET:        RET
CHECK_DV        ENDP


;
; Determine the linear address of video memory and set the
; base address of G_VIDEO_SEL
;
; Must not be called before INIT_PAGING as G_PHYS_DESC is used!
;
                ASSUME  DS:SV_DATA
INIT_VIDEO      PROC    NEAR
                MOV     EAX, 0B8000H            ; ???
                CMP     MACHINE, MACH_PC98      ; NEC PC-98?
                JE      SHORT VIOMEM1           ; Yes ->
                CMP     MACHINE, MACH_FMR70     ; FMR70?
                JE      SHORT VIOMEM1           ; Yes ->
                INT     11H                     ; Equipment determination
                AND     AL, 30H                 ; Initial video mode
                CMP     AL, 30H                 ; Monochrome adapter?
                MOV     EAX, 0B0000H
                JE      SHORT VIOMEM1           ; Yes ->
                MOV     EAX, 0B8000H
VIOMEM1:        ADD     EAX, G_PHYS_BASE        ; Compute linear address
                MOV     VIDEO_LIN, EAX          ; Store linear address
                LEA     DI, G_VIDEO_DESC
                CALL    RM_SEG_BASE
                CALL    VINIT                   ; Get the width and height
                RET
INIT_VIDEO      ENDP

;
; Cleanup before returning to DOS
;
                ASSUME  DS:NOTHING
CLEANUP         PROC    NEAR
                MOV     AX, SV_DATA
                MOV     DS, AX
                ASSUME  DS:SV_DATA
                CALL    CLEANUP_TIMER
                CALL    CLEANUP_INT
                CALL    CLEANUP_SIGNAL
                CALL    CLEANUP_SWAP
                CALL    CLEANUP_MEMORY
                CALL    CLEANUP_VCPI
                CALL    CLEANUP_XMS
                CALL    CLEANUP_A20
                RET
CLEANUP         ENDP

;
; Out of memory (real mode)
;
                ASSUME  DS:SV_DATA
RM_OUT_OF_MEM:  LEA     DX, $RM_OUT_OF_MEM
                CALL    RTEXT
                MOV     AL, 0FFH
                JMP     EXIT


INIT_CODE       ENDS

                END     START
