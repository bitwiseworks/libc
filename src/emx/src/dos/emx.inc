;
; EMX.INC -- Global definitions for emx
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
; See emx.asm for a special exception.
;

FALSE           =       0

;
; Assembly-time options (for debugging)
;
DEBUG_MEM       =       FALSE
DEBUG_MEM_WAIT  =       FALSE
DEBUG_MAP       =       FALSE
STOP_SWAP       =       FALSE
DEBUG_SWAP      =       FALSE

;
; Floating point support
;
FLOATING_POINT  =       NOT FALSE

;
; GDT Selectors
;
G_NULL_SEL      =       00H             ; NULL
G_GDT_MEM_SEL   =       08H             ; The GDT as memory segment
G_IDT_MEM_SEL   =       10H             ; The IDT as memory segment
G_PHYS_SEL      =       18H             ; Physical memory
G_SV_CODE_SEL   =       20H             ; Supervisor code segment
G_REAL_SEL      =       28H             ; Used for switching to real mode
G_INIT_SEL      =       30H             ; Mixed mode init/exit code
G_SV_DATA_SEL   =       38H             ; Supervisor data
G_SV_STACK_SEL  =       40H             ; Supervisor stack
G_VIDEO_SEL     =       48H             ; Video memory
G_TSS_SEL       =       50H             ; TSS
G_SERVER_SEL    =       58H             ; VCPI Server (3 selectors)
G_ENV_SEL       =       70H             ; Environment
G_HDR_SEL       =       78H             ; emxbind patch area
G_TMP1_SEL      =       80H             ; For copying arguments/environment
G_TMP2_SEL      =       88H             ; For copying arguments/environment
G_ZERO_SEL      =       90H             ; For VCPI (prot mode -> real mode)
G_BUF1_SEL      =       98H             ; 1st communication buffer
G_BUF2_SEL      =      0A0H             ; 2nd communication buffer
G_PAGEDIR_SEL   =      0A8H             ; The page directory (memory segment)
G_SWAP_BMP_SEL  =      0B0H             ; The swapper file bitmap
G_LIN_MAP_SEL   =      0B8H             ; Linear address map
G_PAGE_BMP_SEL  =      0C0H             ; Free page bitmap
G_TSS_EX8_SEL   =      0C8H             ; TSS for exception 8 (double fault)
G_TSS_EX10_SEL  =      0D0H             ; TSS for exception 10 (invalid TSS)
G_EX8_STACK_SEL =      0D8H             ; Stack for exception 8 task
G_EX10_STACK_SEL =     0E0H             ; Stack for exception 10 task
G_TSS_MEM_SEL   =      0E8H             ; The TSS as memory segment
G_LOWMEM_SEL    =      0F0H             ; The lower 1MB (+64KB)
G_LDT_SEL       =      0F8H             ; LDT (MAX_PROCESSES entries)

;
; LDT selectors
;
L_NULL_SEL      =       07H             ; NULL
L_CODE_SEL      =       0FH             ; User code segment
L_DATA_SEL      =       17H             ; User data segment
L_VIDEO_SEL     =       1FH             ; Video memory
L_SYM_SEL       =       27H             ; Symbols
L_CLIENT_SEL    =       2FH             ; Client process' data (for FPU emu)

;
; Swapper info table entry
;
SRC_MASK        =      300H             ; ...in swapper info table entry

SRC_NONE        =      000H             ; No source (inaccessible, stack)
SRC_ZERO        =      100H             ; Zero
SRC_EXEC        =      200H             ; Executable file
SRC_SWAP        =      300H             ; Swapper file

SWAP_ALLOC      =      400H             ; Swap space allocated

;
; Flags
;
FLAG_C          =        0001H          ; Carry flag
FLAG_TF         =        0100H          ; Trap flag
FLAG_IF         =        0200H          ; Interrupt enable

;
; Control characters
;
TAB             =       09H             ; Tab
LF              =       0AH             ; Linefeed
CR              =       0DH             ; Carriage return

;
; Other constants
;
NO_FILE_HANDLE  =       0FFFFH          ; Invalid file handle (file not open)

;
; Machines
;
MACH_PC         =       0               ; COMPAQ or IBM or 100% compatible
MACH_FMR70      =       1               ; Fujitsu FMR70
MACH_PC98       =       2               ; NEC PC-98
MACH_INBOARD    =       3               ; Inboard 386/PC
MACH_MAX        =       3               ; (Last machine code)

FMR70_MEM_MAX   =       6144            ; 6 MB

;
; Values of FP_FLAG
;
FP_NO           =       0               ; No coprocessor
FP_287          =       2               ; 287
FP_387          =       3               ; 387


                .386
                .387

;
; More flags (386)
;
FLAG_RF         =       10000H          ; Resume flag
FLAG_VM         =       20000H          ; Virtual mode flag

;
; Control register 0
;
CR0_PE          =       00000001H       ; Protected mode
CR0_MP          =       00000002H       ; Math present
CR0_EM          =       00000004H       ; Emulate
CR0_TS          =       00000008H       ; Task switched
CR0_ET          =       00000010H       ; Extension type
CR0_PG          =       80000000H       ; Paging

;
; All FPU bits but CR0_ET
;
CR0_FPU         =       CR0_TS OR CR0_EM OR CR0_MP

;
; The biggest unsigned 32 bit number
;
MAX_32          =       0FFFFFFFFH

;
; Structures
;
DESCRIPTOR      STRUCT
LIMIT_0_15      DW      ?
BASE_0_15       DW      ?
BASE_16_23      DB      ?
DACCESS         DB      ?
GRAN            DB      ?
BASE_24_31      DB      ?
DESCRIPTOR      ENDS

;
; Segments
;
                .SEQ                    ; Sequential ordering

HDR_SEG         SEGMENT PARA USE16 PUBLIC 'HEADER'
HDR_SEG         ENDS


INIT_CODE       SEGMENT PARA USE16 PUBLIC 'CODE'
INIT_CODE       ENDS


SV_DATA         SEGMENT PARA USE16 PUBLIC 'DATA'
SV_DATA         ENDS


SV_CODE         SEGMENT PARA USE16 PUBLIC 'PROTCODE'
SV_CODE         ENDS

SV_STACK        SEGMENT PARA USE16 PUBLIC 'PROTSTACK'
SV_STACK        ENDS


RM_STACK        SEGMENT PARA USE16 STACK 'STACK'
RM_STACK        ENDS

LSTACK          SEGMENT PARA USE16 PUBLIC 'LSTACK'
LSTACK          ENDS


EX8_STACK       SEGMENT PARA USE16 PUBLIC 'EX8STACK'
EX8_STACK       ENDS


EX10_STACK      SEGMENT PARA USE16 PUBLIC 'EX10STACK'
EX10_STACK      ENDS

;
; ES := DS
;
MOV_ES_DS       MACRO
                PUSH    DS
                POP     ES
                ENDM

; DS := ES
MOV_DS_ES       MACRO
                PUSH    ES
                POP     DS
                ENDM

;
; FS := DS
;
MOV_FS_DS       MACRO
                PUSH    DS
                POP     FS
                ENDM

;
; Exchange DS and ES
;
XCHG_DS_ES      MACRO
                PUSH    DS
                PUSH    ES
                POP     DS
                POP     ES
                ENDM

;
; JMP and CALL instructions with <selector>:<offset16> instead of
; <segment>:<offset16>
;
JMPF16          MACRO   SEL, OFF
                DB      0EAH                    ; JMP FAR
                DW      OFF, SEL
                ENDM


CALLF16         MACRO   SEL, OFF
                DB      9AH                     ; CALL FAR
                DW      OFF, SEL
                ENDM

;
; Acoustic output for debugging
;
BEEP            MACRO
                LOCAL   L1, L2
                IN      AL, 61H
                MOV     AH, AL
                OR      AL, 3
                JMP     SHORT L1
L1:             JMP     SHORT L2
L2:             OUT     61H, AL
                MOV     CX, 0
                LOOP    $
                LOOP    $
                MOV     AL, AH
                OUT     61H, AL
                ENDM



;
; Serial output for debugging
;
; Modifies AL, DX and flags
;

;SER_PORT       =       03F8H                   ; COM1
SER_PORT        =       02F8H                   ; COM2

SERIAL          MACRO   C1
                LOCAL   L1
                MOV     DX, SER_PORT+4
                MOV     AL, 3
                OUT     DX, AL
                INC     DX
L1:             IN      AL, DX                  ; Read line status
                TEST    AL, 20H                 ; Transmitter empty?
                JZ      SHORT L1                ; No  -> repeat
                MOV     DX, SER_PORT            ; Transmitter register
                MOV     AL, C1                  ; Write character
                OUT     DX, AL                  ; to transmitter register
                ENDM


DALIGN          MACRO   VAL:REQ
                ALIGN   VAL
                ENDM

TALIGN          MACRO   VAL:REQ
                ALIGN   VAL
                ENDM
