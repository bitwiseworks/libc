;
; DEBUG.ASM -- Built-in debugger
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
; See emx.asm for a special exception.
;

                INCLUDE EMX.INC
                INCLUDE EXCEPT.INC
                INCLUDE OPRINT.INC
                INCLUDE DISASM.INC
                INCLUDE SEGMENTS.INC
                INCLUDE SYMBOLS.INC
                INCLUDE SIGNAL.INC
                INCLUDE PROCESS.INC
                INCLUDE PAGING.INC
                INCLUDE PMINT.INC
                INCLUDE TABLES.INC
                INCLUDE PMIO.INC
                INCLUDE SWAPPER.INC
                INCLUDE FILEIO.INC
                INCLUDE UTILS.INC

                PUBLIC  STEP_FLAG, DEBUG_AVAIL, DEBUG_SER_FLAG, DEBUG_SER_PORT
                PUBLIC  DEBUG_EXCEPTION, SET_BREAKPOINT, INS_BREAKPOINTS
                PUBLIC  DEBUG_RESUME, DEBUG_STEP, DEBUG_QUIT, DEBUG_INIT

BP_INACTIVE     =       0                       ; Entry unused
BP_BREAKPOINT   =       1                       ; Code breakpoint
BP_WATCHPOINT_R =       2                       ; R/W watchpoint
BP_WATCHPOINT_W =       3                       ; W watchpoint
BP_WATCHPOINT_X =       4                       ; X watchpoint

BREAKPOINT      STRUCT
BP_TYPE         DB      ?                       ; Breakpoint type (see above)
BP_LEN          DB      ?                       ; Watchpoint length
BP_SEL          DW      ?                       ; Selector
BP_OFF          DD      ?                       ; Offset
BP_ADDR         DD      ?                       ; Linear address
BP_VAL          DD      ?                       ; Watchpoint: initial value
BREAKPOINT      ENDS

SV_DATA         SEGMENT

DEBUG_SER_PORT  DW      02F8H                   ; COM2
DEBUG_SER_FLAG  DB      FALSE

STEP_FLAG       DB      FALSE
DEBUG_AVAIL     DB      NOT FALSE

DEBUG_SHOW_PC   DB      NOT FALSE
DEBUG_STOP      DB      NOT FALSE
GO_FLAG         DB      FALSE
DEBUG_COUNT     DB      0
FATAL           DB      ?
DA_SYM_FLAG     DB      ?
DA_EMPTY_FLAG   DB      ?
NEXT_CS         DW      ?
NEXT_EIP        DD      ?

B_NEXT          DB      0
B_COLUMN        DB      0

                TALIGN  2
BREAKPOINTS     BREAKPOINT 4 DUP (<BP_INACTIVE>)

                TALIGN  2
GCHAR           DW      B_CHAR
GTEXT           DW      B_TEXT
GDWORD          DW      B_DWORD
GWORD           DW      B_WORD
GBYTE           DW      B_BYTE
GCRLF           DW      B_CRLF
GINCHAR         DW      B_INCHAR
DA_SYM_ADDR     DD      ?
RANGE_SEL       DW      ?
RANGE_OFF       DD      ?
ADDR_LIN        DD      ?
ADDR_ARG        DD      ?
SEARCH_SEL      DW      ?
SEARCH_OFF      DD      ?
SEARCH_LEN      DD      ?
SEARCH_SIZE     DD      ?
ADDR_SEL        DW      ?
ADDR_OFF        DD      ?
ADDR_COUNT      DD      ?
SEL_SEL         DW      ?
SEL_COUNT       DW      ?
U_LAST_SEL      DW      0
U_LAST_OFF      DD      ?
D_LAST_SEL      DW      0
D_LAST_OFF      DD      ?
WP_LEN          DB      ?
WP_TYPE         DB      ?
SKIP_FLAG       DB      FALSE
WATCH_FAILURES  DB      ?
XOFF            DB      FALSE
LOOK_AHEAD      DB      0
LIST_ALL        DB      FALSE

SEARCH_BUF      DB      64 DUP (?)

FLAG_TAB        DW      0001H
                DB      "NCCY"
                DW      0004H
                DB      "PEPO"
                DW      0010H
                DB      "NAAC"
                DW      0040H
                DB      "NZZR"
                DW      0080H
                DB      "PLNG"
                DW      0200H
                DB      "DIEI"
                DW      0400H
                DB      "UPDN"
                DW      0800H
                DB      "NVOV"
                DW      0000H

R8              =       0
R16             =       1
R32             =       2

REGISTER        MACRO   @NAME, @SIZE, @OFFSET
                DB      @NAME, @SIZE, @OFFSET
                ENDM

REG_TAB         LABEL   BYTE
                REGISTER "EFLAGS", R32, 52
                REGISTER "EAX",    R32, 36
                REGISTER "EBX",    R32, 24
                REGISTER "ECX",    R32, 32
                REGISTER "EDX",    R32, 28
                REGISTER "ESI",    R32, 12
                REGISTER "EDI",    R32,  8
                REGISTER "EBP",    R32, 16
                REGISTER "ESP",    R32, 56      ; See GET_REG
                REGISTER "EIP",    R32, 44
                REGISTER "AX",     R16, 36
                REGISTER "BX",     R16, 24
                REGISTER "CX",     R16, 32
                REGISTER "DX",     R16, 28
                REGISTER "SI",     R16, 12
                REGISTER "DI",     R16,  8
                REGISTER "BP",     R16, 16
                REGISTER "SP",     R16, 56      ; See GET_REG
                REGISTER "IP",     R16, 44
                REGISTER "AL",     R8,  36
                REGISTER "BL",     R8,  24
                REGISTER "CL",     R8,  32
                REGISTER "DL",     R8,  28
                REGISTER "AH",     R8,  37
                REGISTER "BH",     R8,  25
                REGISTER "CH",     R8,  33
                REGISTER "DH",     R8,  29
                REGISTER "CS",     R16, 48
                REGISTER "DS",     R16,  6
                REGISTER "ES",     R16,  4
                REGISTER "FS",     R16,  2
                REGISTER "GS",     R16,  0
                REGISTER "SS",     R16, 60      ; See GET_REG
                REGISTER 0FFH,     0FFH, 0FFH

$EXCEPT         DB      "Exception ", 0
$CR2            DB      "  CR2=", 0
$ERRCD          DB      "ERRCD=", 0
$BREAKPOINT     DB      "Breakpoint ", 0
$WATCHPOINT     DB      "Watchpoint ", 0
$INPUT_ERROR    DB      "Input error", CR, LF, 0
$INVALID_SEL    DB      "Invalid selector", CR, LF, 0
$BEYOND_LIMIT   DB      "Offset beyond segment limit", CR, LF, 0
$ALIGNMENT      DB      "Address not correctly aligned", CR, LF, 0
$ADDR_FAILURE   DB      "GET_LIN failed", CR, LF, 0
$PAGE_DIR       DB      "Page table not present", CR, LF, 0
$PHYSICAL       DB      "physical=", 0
$LINEAR         DB      "linear=", 0
$EXTERNAL       DB      "external=", 0
$LINE           DB      " line ", 0
$RPL            DB      ": RPL=", 0
$TI_GDT         DB      " GDT ", 0
$TI_LDT         DB      " LDT ", 0
$SEL_INVALID    DB      "invalid selector", 0
$DPL            DB      " DPL=", 0
$DATA           DB      "data segment", 0
$CODE           DB      "code segment", 0
$BASE           DB      " base=", 0
$LIMIT          DB      " limit=", 0
$SEL_16         DB      "16-bit ", 0
$SEL_32         DB      "32-bit ", 0
$INT            DB      "int gate", 0
$TRAP           DB      "trap gate", 0
$TSS            DB      "TSS", 0
$CALL           DB      "call gate", 0
$TASK           DB      "task gate", 0
$LDT            DB      "LDT", 0
$NULL           DB      "null", 0

$INSBRK         DB      "INS_BREAKPOINTS", CR, LF, 0
$CONTINUE       DB      "CONTINUE", CR, LF, 0
$DO_STEP        DB      "DO_STEP", CR, LF, 0
$DO_CALL        DB      "DO_CALL", CR, LF, 0
$DEBUG_EXCEPTION DB     "DEBUG_EXCEPTION ", 0
$RUN            DB      "RUN", CR, LF, 0

$EMPTY          DB      "empty", 0
$INFINITY       DB      "invalid or infinity", 0
$ZERO           DB      "zero", 0
$CW             DB      "CW=", 0
$SW             DB      " SW=", 0
$C3210          DB      " C3210=", 0

$SWAP_FAULTS    DB      CR, LF
                DB      "Swapper statistics: F=", 0
$SWAP_READS     DB      " R=", 0
$SWAP_WRITES    DB      " W=", 0
$SNATCH_COUNT   DB      " N=", 0
$SWAP_SIZE      DB      " S=", 0

$INFO_TITLE     DB      "idx pid      ppid     file handles", CR, LF, 0
;                        xx* xxxxxxxx xxxxxxxx xx

INPUT_BUF       LABEL   BYTE
DISASM_BUF      DB      80 DUP (?)
SYMBOL_BUF      DB      80 DUP (?)

SV_DATA         ENDS

SV_CODE         SEGMENT

                .386P

                ASSUME  CS:SV_CODE, DS:NOTHING

;
; Set EFLAGS for resuming current process (without stepping)
;
; In:   SS:BP   Interrupt/exception stack frame
;
                ASSUME  BP:PTR ISTACKFRAME
DEBUG_RESUME    PROC    NEAR
                AND     I_EFLAGS, NOT FLAG_TF
                OR      I_EFLAGS, FLAG_RF
                RET
                ASSUME  BP:NOTHING
DEBUG_RESUME    ENDP


;
; Set EFLAGS for single stepping current process
;
; In:   SS:BP   Interrupt/exception stack frame
;
                ASSUME  BP:PTR ISTACKFRAME
DEBUG_STEP      PROC    NEAR
                OR      I_EFLAGS, FLAG_TF
                RET
                ASSUME  BP:NOTHING
DEBUG_STEP      ENDP


;
; Exception 1 (debug)
;
                ASSUME  DS:SV_DATA
                ASSUME  BP:PTR ISTACKFRAME
                TALIGN  2
DEBUG_EXCEPTION PROC    NEAR
                XOR     EAX, EAX
                MOV     DR7, EAX                ; Disable break points
                MOV     EBX, DR6                ; Copy DR6 to EBX
                MOV     DR6, EAX                ; DR6 never cleared by 386
                MOV     ECX, CR2                ; Copy CR2 to ECX
                STI
                CMP     DEBUG_FLAG, FALSE
                JE      SHORT DEX0
                LEA     EDX, $DEBUG_EXCEPTION
                CALL    GTEXT
                MOV     AX, I_CS
                CALL    GWORD
                MOV     AL, ":"
                CALL    GCHAR
                MOV     EAX, I_EIP
                CALL    GDWORD
                CALL    GCRLF
DEX0:           MOV     AL, EXCEPT_NO           ; Get exception number
                CMP     AL, 1                   ; Debugging exception?
                JE      SHORT REASON1           ; Yes -> check for reason
;
; Fatal exception or breakpoint instruction
;
                LEA     EDX, $EXCEPT
                CALL    GTEXT
                CALL    GBYTE
                MOV     AL, ":"
                CALL    GCHAR
                MOV     AL, " "
                CALL    GCHAR
                MOV     AL, EXCEPT_NO
                CALL    EXCEPT_NAME
                CALL    GTEXT
                CALL    GCRLF
                MOV     FATAL, NOT FALSE
                CMP     EXCEPT_NO, 3            ; INT 3
                JNE     SHORT DEX1
                MOV     FATAL, FALSE
DEX1:           CMP     EXCEPT_NO, 8
                JB      SHORT DEX9
                CMP     EXCEPT_NO, 14
                JA      SHORT DEX9
                LEA     EDX, $ERRCD
                CALL    GTEXT
                MOV     EAX, I_ERRCD
                CALL    GDWORD
                CMP     EXCEPT_NO, 14           ; Page fault?
                JNE     SHORT DEX8
                LEA     EDX, $CR2
                CALL    GTEXT
                MOV     EAX, ECX
                CALL    GDWORD
DEX8:           CALL    GCRLF
DEX9:           MOV     AL, 1                   ; New disassembly
                JMP     SHOW

;
; Debugging exception
;
REASON1:        MOV     FATAL, FALSE            ; Not a fatal exception
;
; Check for X type watchpoints
;
                LEA     SI, BREAKPOINTS
                ASSUME  SI:PTR BREAKPOINT
                MOV     CX, 4
                MOV     WATCH_FAILURES, 0
                MOV     EDX, 0001H              ; B0
CHECK_WP:       CMP     [SI].BP_TYPE, BP_WATCHPOINT_X
                JNE     SHORT CHECK_WP9         ; Skip other entries
                CALL    WATCH_GET               ; Read memory
                CMP     EAX, [SI].BP_VAL        ; Changed?
                JNE     SHORT CHECK_WP1         ; Yes -> hit
                INC     WATCH_FAILURES          ; Increment number of non-hits
                JMP     SHORT CHECK_WP9         ; Next entry
CHECK_WP1:      OR      EBX, EDX                ; Set Bx bit of DR6
CHECK_WP9:      SHL     EDX, 1                  ; Next Bx bit
                ADD     SI, SIZE BREAKPOINT
                LOOP    CHECK_WP
                ASSUME  SI:NOTHING
                TEST    EBX, 0FH                ; Any breakpoints or wpx hits?
                JNZ     SHORT SHOW_BP           ; Yes -> display breakpoints
                CMP     WATCH_FAILURES, 0       ; Any X watchpoint?
                JNE     SHORT WATCH1            ; Yes -> continue?
                MOV     AL, FALSE               ; Continue disassembly
                JMP     SHOW                    ; Stop

WATCH1:         CMP     GO_FLAG, FALSE          ; GO command?
                JE      SHOW                    ; No  -> stop
                JMP     CONTINUE                ; Continue program
;
; Display breakpoints
;
SHOW_BP:        LEA     SI, BREAKPOINTS
                ASSUME  SI:PTR BREAKPOINT
                MOV     CX, 4
SHOW_BP1:       SHR     EBX, 1                  ; Test in turn B0, B1, B2, B3
                JNC     SHORT SHOW_BP9          ; Not set -> skip
                CMP     [SI].BP_TYPE, BP_INACTIVE ; B1 seems to be inaccurate
                JE      SHORT SHOW_BP9
                LEA     EDX, $BREAKPOINT        ; Breakpoint
                CMP     [SI].BP_TYPE, BP_BREAKPOINT
                JE      SHORT SHOW_BP2
                LEA     EDX, $WATCHPOINT        ; Watchpoint
SHOW_BP2:       CALL    GTEXT                   ; Display breakpoint type
                MOV     AL, "4"
                SUB     AL, CL
                CALL    GCHAR                   ; Display breakpoint number
                CALL    GCRLF
SHOW_BP9:       ADD     SI, SIZE BREAKPOINT     ; Next breakpoint
                LOOP    SHOW_BP1
                ASSUME  SI:NOTHING
                MOV     AL, 1                   ; New disassembly
;
; Show disassembled instruction
;
SHOW:           CMP     DEBUG_SHOW_PC, FALSE    ; Disassemble?
                JE      SHORT DEBUG_01          ; No  -> skip
                CALL    TRACE_DISASM            ; Show current instruction
DEBUG_01:       CMP     FATAL, FALSE            ; Fatal exception?
                JNE     STOP1                   ; Yes -> stop
                CMP     DEBUG_STOP, FALSE       ; Non-stop mode?
                JE      DO_STEP                 ; Yes -> continue
                CMP     DEBUG_COUNT, 0          ; Multiple steps?
                JE      SHORT STOP1             ; No  -> stop
                DEC     DEBUG_COUNT             ; Another step?
                JNZ     DO_STEP                 ; Yes -> continue
;
; Stop program, wait for command
;
STOP1:          MOV     DEBUG_COUNT, 0          ; Disable multiple steps
                MOV     DEBUG_STOP, NOT FALSE   ; Disable non-stop mode
                MOV     DEBUG_SHOW_PC, NOT FALSE ; Enable disassembly
                MOV     BREAKPOINTS[0].BP_TYPE, BP_INACTIVE ; Delete bp 0
                MOV     GO_FLAG, FALSE          ; No GO command active
;
; Wait for command
;
KEY:            CALL    GINCHAR
                OR      AL, AL                  ; Extended key code?
                JZ      FKEY                    ; Yes ->
;
; Check for digits
;
                MOV     AH, 10
                CMP     AL, "0"                 ; 10 steps?
                JE      CMD_MULT                ; Yes ->
                JB      SHORT KEY1
                MOV     AH, AL
                SUB     AH, "0"
                CMP     AL, "9"                 ; Multiple steps (1-9) ?
                JBE     CMD_MULT                ; Yes ->
;
; Check for other commands
;
KEY1:           CALL    UPPER                   ; Convert to upper case
                CMP     AL, "."                 ; Registers
                JE      CMD_DOT
                CMP     AL, ":"                 ; Silent
                JE      CMD_SILENT
                CMP     AL, "A"                 ; Address
                JE      CMD_ADDR
                CMP     AL, "B"                 ; Breakpoint
                JE      CMD_BREAK
                CMP     AL, "C"                 ; Call
                JE      CMD_CALL
                CMP     AL, "D"                 ; Display
                JE      CMD_DISPLAY
                CMP     AL, "F"                 ; 387 status
                JE      CMD_387
                CMP     AL, "G"                 ; Go
                JE      CMD_GO
                CMP     AL, "I"                 ; Info
                JE      CMD_INFO
                CMP     AL, "K"                 ; Kill
                JE      CMD_KILL
                CMP     AL, "L"                 ; Selector
                JE      CMD_SELECTOR
                CMP     AL, "N"                 ; Non-stop
                JE      SHORT CMD_NONSTOP
                CMP     AL, "Q"                 ; Quit
                JE      CMD_QUIT
                CMP     AL, "R"                 ; Register
                JE      CMD_REG
                CMP     AL, "S"                 ; Search
                JE      CMD_SEARCH
                CMP     AL, "U"                 ; Unassemble
                JE      CMD_UNASSEMBLE
                CMP     AL, "V"                 ; Virtual
                JE      CMD_VIRT
                CMP     AL, "W"                 ; Watchpoint
                JE      CMD_WATCH
                CMP     AL, "X"                 ; Symbol
                JE      CMD_SYM
                CMP     AL, " "                 ; Step
                JE      CMD_STEP
BAD_KEY:        MOV     AL, 07H
                CALL    GCHAR
                JMP     KEY                     ; Wait for another command

FKEY:           CALL    GINCHAR                 ; Get second code
                CMP     AL, 3CH                 ; F2
                JE      CMD_DOT
                CMP     AL, 3FH                 ; F5?
                JE      CONTINUE                ; Yes -> continue program
                CMP     AL, 42H                 ; F8
                JE      DO_STEP
                CMP     AL, 44H                 ; F10
                JE      DO_CALL
                JMP     SHORT BAD_KEY           ; Ring bell

;
; Non-stop single stepping mode
;
CMD_NONSTOP:    MOV     DEBUG_STOP, FALSE       ; Set non-stop mode
                JMP     DO_STEP                 ; Continue program

;
; Display registers
;
CMD_DOT:        CALL    GCRLF                   ; New line
                CALL    REG_DUMP                ; Display registers
                MOV     AL, 1                   ; New disassembly
                JMP     SHOW                    ; Display current instruction

;
; Multiple steps
;
CMD_MULT:       MOV     DEBUG_COUNT, AH         ; Set number of steps
                JMP     DO_STEP                 ; Continue program

;
; Silent command
;
CMD_SILENT:     NOT     DEBUG_SHOW_PC           ; Toggle silent flag
                JMP     KEY                     ; Command loop

;
; Go command
;
CMD_GO:         CALL    INPUT                   ; Input arguments
                JC      INPUT_ERROR
                CALL    END_OF_LINE
                JZ      SHORT CONTINUE
                MOV     DX, I_CS
                CALL    GET_ADDR
                JC      INPUT_ERROR
                CALL    END_OF_LINE
                JNZ     INPUT_ERROR
;
; Breakpoint 0 at DX:EAX and continue program
;
                CALL    SET_BREAKPOINT          ; Set breakpoint 0 at DX:EAX
                JNZ     INPUT_ERROR             ; Error -> display message
;
; Insert breakpoints and continue program
;
CONTINUE:       CMP     DEBUG_FLAG, FALSE
                JE      SHORT CONT_0
                LEA     EDX, $CONTINUE
                CALL    GTEXT
CONT_0:         MOV     GO_FLAG, NOT FALSE      ; GO command executing
                CALL    DEBUG_RESUME            ; Resume program, don't step
                CALL    INS_BREAKPOINTS         ; Insert breakpoints
                JMP     RUN                     ; Continue program


;
; Watchpoint command
;
CMD_WATCH:      CALL    INPUT                   ; Input arguments
                JC      INPUT_ERROR
                CALL    END_OF_LINE             ; Any arguments?
                JZ      CMD_BREAK_SHOW          ; No  -> display breakpoints
                CALL    FETCH                   ; Fetch character (length)
                JZ      INPUT_ERROR             ; End of line -> error
                MOV     AH, 00H
                CMP     AL, "B"
                JE      SHORT CMD_WATCH_1
                MOV     AH, 01H
                CMP     AL, "W"
                JE      SHORT CMD_WATCH_1
                MOV     AH, 03H
                CMP     AL, "D"
                JNE     INPUT_ERROR
CMD_WATCH_1:    MOV     WP_LEN, AH
                CALL    FETCH                   ; Fetch character (type)
                JZ      INPUT_ERROR
                MOV     AH, BP_WATCHPOINT_R
                CMP     AL, "R"
                JE      SHORT CMD_WATCH_2
                MOV     AH, BP_WATCHPOINT_W
                CMP     AL, "W"
                JE      SHORT CMD_WATCH_2
                MOV     AH, BP_WATCHPOINT_X
                CMP     AL, "X"
                JNE     INPUT_ERROR
CMD_WATCH_2:    MOV     WP_TYPE, AH
                CALL    SKIP
                MOV     DX, I_DS
                CALL    GET_ADDR                ; Get address
                JC      INPUT_ERROR
                CALL    END_OF_LINE
                JNZ     INPUT_ERROR
                MOV     BH, WP_LEN
                MOV     BL, WP_TYPE
                TEST    AL, BH
                JNZ     ALIGNMENT_ERROR
                CALL    SET_WATCHPOINT          ; Set watchpoint
                JNZ     INPUT_ERROR
                CALL    SHOW_BREAKPOINT
                JMP     KEY                     ; Command loop

;
; Breakpoint command
;
CMD_BREAK:      CALL    INPUT                   ; Input arguments
                JC      INPUT_ERROR
                CALL    END_OF_LINE             ; Any arguments?
                JZ      CMD_BREAK_SHOW          ; No  -> display breakpoints
                MOV     DX, I_CS
                CALL    GET_ADDR                ; Get address
                JC      INPUT_ERROR
                CALL    END_OF_LINE
                JNZ     INPUT_ERROR
                MOV     BH, 0
                MOV     BL, BP_BREAKPOINT
                CALL    SET_WATCHPOINT          ; Set breakpoint
                JNZ     INPUT_ERROR
                CALL    SHOW_BREAKPOINT
                JMP     KEY                     ; Command loop

CMD_BREAK_SHOW: LEA     SI, BREAKPOINTS
                MOV     CX, 4
CBS_1:          CALL    SHOW_BREAKPOINT
                ADD     SI, SIZE BREAKPOINT
                LOOP    CBS_1
                JMP     KEY

;
; Delete breakpoint
;
CMD_KILL:       CALL    INPUT                   ; Input arguments
                JC      INPUT_ERROR
                CALL    END_OF_LINE             ; Any arguments?
                JZ      CMD_BREAK_SHOW          ; No  -> display breakpoints
                CALL    FETCH                   ; Fetch character (number)
                JZ      INPUT_ERROR             ; End of line -> error
                SUB     AL, "0"
                CMP     AL, 3
                JA      INPUT_ERROR
                MOV     AH, SIZE BREAKPOINT
                MUL     AH
                LEA     SI, BREAKPOINTS
                ADD     SI, AX
                CALL    SKIP
                CALL    END_OF_LINE
                JNZ     INPUT_ERROR
                MOV     (BREAKPOINT PTR [SI]).BP_TYPE, BP_INACTIVE
                JMP     KEY


;
; Display breakpoint entry
;
; In:   DS:SI   Pointer to breakpoint entry
;
                ASSUME  SI:PTR BREAKPOINT
SHOW_BREAKPOINT PROC    NEAR
                CMP     [SI].BP_TYPE, BP_INACTIVE
                JE      SBP_RET
                LEA     AX, BREAKPOINTS
                SUB     AX, SI
                NEG     AX
                MOV     BX, SIZE BREAKPOINT
                XOR     DX, DX
                DIV     BX
                ADD     AL, "0"
                CALL    GCHAR
                MOV     AL, " "
                CALL    GCHAR
                MOV     AL, "B"
                CMP     [SI].BP_TYPE, BP_BREAKPOINT
                JE      SHORT SBP_1
                MOV     AL, "R"
                CMP     [SI].BP_TYPE, BP_WATCHPOINT_R
                JE      SHORT SBP_1
                MOV     AL, "W"
                CMP     [SI].BP_TYPE, BP_WATCHPOINT_W
                JE      SHORT SBP_1
                MOV     AL, "X"
                CMP     [SI].BP_TYPE, BP_WATCHPOINT_X
                JE      SHORT SBP_1
                MOV     AL, "?"
SBP_1:          CALL    GCHAR
                CMP     AL, "B"
                MOV     AL, " "
                JE      SHORT SBP_2
                MOV     AL, "B"
                CMP     [SI].BP_LEN, 00H
                JE      SHORT SBP_2
                MOV     AL, "W"
                CMP     [SI].BP_LEN, 01H
                JE      SHORT SBP_2
                MOV     AL, "D"
                CMP     [SI].BP_LEN, 03H
                JE      SHORT SBP_2
                MOV     AL, "?"
SBP_2:          CALL    GCHAR
                MOV     AL, " "
                CALL    GCHAR
                MOV     AX, [SI].BP_SEL
                CALL    GWORD
                MOV     AL, ":"
                CALL    GCHAR
                MOV     EAX, [SI].BP_OFF
                CALL    GDWORD
                MOV     AL, " "
                CALL    GCHAR
                MOV     EAX, [SI].BP_ADDR
                CALL    GDWORD
                CALL    GCRLF
SBP_RET:        RET
                ASSUME  SI:NOTHING
SHOW_BREAKPOINT ENDP


;
; Unassemble
;
CMD_UNASSEMBLE: CALL    INPUT
                JC      INPUT_ERROR
                MOV     DX, U_LAST_SEL
                OR      DX, DX
                JZ      SHORT UAS01
                CALL    END_OF_LINE
                MOV     EAX, U_LAST_OFF
                JZ      UAS02
UAS01:          MOV     DX, I_CS
                CALL    GET_ADDR
                JC      INPUT_ERROR
                CALL    END_OF_LINE
                JNZ     INPUT_ERROR
UAS02:          VERR    DX
                JNZ     INVALID_SEL
                MOV     FS, DX
                MOV     ESI, EAX
                MOV     CX, 16
                MOV     DA_EMPTY_FLAG, FALSE
UAS1:           CMP     DA_EMPTY_FLAG, FALSE
                JE      SHORT UAS2
                MOV     DA_EMPTY_FLAG, FALSE
                CALL    GCRLF
                JMP     SHORT UAS3
UAS2:           PUSH    CX
                CALL    SHOW_DISASM
                POP     CX
UAS3:           LOOP    UAS1
                MOV     U_LAST_SEL, FS
                MOV     U_LAST_OFF, ESI
                JMP     KEY

CMD_DISPLAY:    CALL    INPUT
                JC      INPUT_ERROR
                MOV     DX, D_LAST_SEL
                OR      DX, DX
                JZ      SHORT DISP00
                CALL    END_OF_LINE
                MOV     EAX, D_LAST_OFF
                MOV     ECX, 16
                JZ      SHORT DISP02
DISP00:         MOV     DX, I_DS
                CALL    GET_ADDR
                JC      INPUT_ERROR
                CALL    END_OF_LINE
                JZ      SHORT DISP01
                CALL    GET_RANGE_MORE
                JC      ERROR
                CALL    END_OF_LINE
                JNZ     INPUT_ERROR
                ADD     ECX, 15
                SHR     ECX, 4
                JMP     SHORT DISP02
DISP01:         VERR    DX
                JNZ     INVALID_SEL
                MOV     ECX, 16
DISP02:         MOV     FS, DX
                MOV     ESI, EAX
DISP10:         CALL    SHOW_DATA
                ADD     ESI, 16
                DEC     ECX
                JNZ     DISP10
                MOV     D_LAST_SEL, FS
                MOV     D_LAST_OFF, ESI
                JMP     KEY

CMD_REG:        CALL    INPUT
                JC      INPUT_ERROR
                CALL    END_OF_LINE
                JZ      CMD_DOT
                PUSH    BX
                CALL    FETCH
                JZ      SHORT REG10
                MOV     AH, AL
                CALL    FETCH
                JZ      SHORT REG10
                CALL    SKIP
                CALL    END_OF_LINE
                JNZ     SHORT REG10
                XCHG    AL, AH
                LEA     BX, FLAG_TAB
REG01:          MOV     CX, [BX+0]
                JCXZ    REG10
                CMP     AX, [BX+2]
                JE      SHORT REG02
                CMP     AX, [BX+4]
                JE      SHORT REG03
                ADD     BX, 6
                JMP     SHORT REG01

REG02:          NOT     CX
                AND     WORD PTR I_EFLAGS, CX
                JMP     SHORT REG04

REG03:          OR      WORD PTR I_EFLAGS, CX
REG04:          POP     BX
                JMP     KEY

REG10:          POP     BX
                PUSH    BX
                CALL    GET_REG
                JC      SHORT REG20
                MOV     CL, AL
                MOV     DI, DX
                CMP     BYTE PTR [BX], " "
                JNE     SHORT REG20
                CALL    SKIP
                CALL    GET_NUMBER
                JC      SHORT REG20
                CALL    SKIP
                CALL    END_OF_LINE
                JNZ     SHORT REG20
;
; CL  = R8, R16, R32
; DI  = pointer to register in stack
; EAX = value
;
                CMP     CL, R8
                JE      SHORT REG10_8
                CMP     CL, R16
                JE      SHORT REG10_16
                CMP     CL, R32
                JNE     SHORT REG20
REG10_32:       MOV     SS:[DI], EAX
                JMP     SHORT REG11
REG10_16:       TEST    EAX, NOT 0FFFFH
                JNZ     SHORT REG10_ERROR
                MOV     SS:[DI], AX
                JMP     SHORT REG11
REG10_8:        TEST    EAX, NOT 0FFH
                JNZ     SHORT REG10_ERROR
                MOV     SS:[DI], AL
REG11:          POP     BX
                JMP     KEY

REG10_ERROR:    POP     BX
                JMP     SHORT INPUT_ERROR

REG20:          POP     BX
                JMP     SHORT INPUT_ERROR

ALIGNMENT_ERROR:
                LEA     EDX, $ALIGNMENT
                JMP     SHORT ERROR

BEYOND_LIMIT:   LEA     EDX, $BEYOND_LIMIT
                JMP     SHORT ERROR

INVALID_SEL:    LEA     EDX, $INVALID_SEL
                JMP     SHORT ERROR

INPUT_ERROR:    LEA     EDX, $INPUT_ERROR
ERROR:          CALL    GTEXT
                JMP     KEY


              IF FLOATING_POINT
CMD_387:        MOV     SI, PROCESS_PTR
                CMP     SI, NO_PROCESS
                JE      BAD_KEY
                ASSUME  SI:PTR PROCESS
                DB      66H
                FNSAVE  DWORD PTR [SI].P_CW    ; Save FPU context, initialize
                FWAIT
                MOV     DX, [SI].P_TW
                MOV     CX, [SI].P_SW
                SHR     CX, 11
                AND     CX, 7
                ROR     DX, CL
                ROR     DX, CL
                LEA     DI, [SI].P_FST
                MOV     CX, 8
FPU_1:          PUSH    DX
                MOV     AL, "S"
                CALL    GCHAR
                MOV     AL, "T"
                CALL    GCHAR
                MOV     AL, "("
                CALL    GCHAR
                MOV     AL, "8"
                SUB     AL, CL
                CALL    GCHAR
                MOV     AL, ")"
                CALL    GCHAR
                MOV     AL, "="
                CALL    GCHAR
                MOV     AX, DX
                AND     AX, 3
                LEA     EDX, $ZERO
                DEC     AX
                JZ      SHORT FPU_2
                LEA     EDX, $INFINITY
                DEC     AX
                JZ      SHORT FPU_2
                LEA     EDX, $EMPTY
                DEC     AX
                JZ      SHORT FPU_2
                PUSH    CX
                PUSH    DI
                MOV     CX, 10
                ADD     DI, CX
FPU_VALID_1:    DEC     DI
                MOV     AL, [DI]
                CALL    GBYTE
                LOOP    FPU_VALID_1
                POP     DI
                POP     CX
                JMP     SHORT FPU_3
FPU_2:          CALL    GTEXT
FPU_3:          CALL    GCRLF
                POP     DX
                ROR     DX, 2
                ADD     DI, 10
                LOOP    FPU_1
                LEA     EDX, $CW
                CALL    GTEXT
                MOV     AX, [SI].P_CW
                CALL    GWORD
                LEA     EDX, $SW
                CALL    GTEXT
                MOV     AX, [SI].P_SW
                CALL    GWORD
                LEA     EDX, $C3210
                CALL    GTEXT
                MOV     DX, 4000H
                CALL    FPU_C
                MOV     DX, 0400H
                CALL    FPU_C
                MOV     DX, 0200H
                CALL    FPU_C
                MOV     DX, 0100H
                CALL    FPU_C
                MOV     AX, [SI].P_SW
                AND     AX, 4500H
                MOV     DL, ">"
                JZ      SHORT FPU_4
                CMP     AX, 0100H
                MOV     DL, "<"
                JE      SHORT FPU_4
                CMP     AX, 4000H
                JNE     SHORT FPU_5
                MOV     DL, "="
FPU_4:          MOV     AL, " "
                CALL    GCHAR
                MOV     AL, DL
                CALL    GCHAR
FPU_5:          CALL    GCRLF
                DB      66H
                FRSTOR  DWORD PTR [SI].P_CW
                JMP     KEY

FPU_C           PROC    NEAR
                MOV     AL, "0"
                TEST    [SI].P_SW, DX
                JZ      SHORT FPU_C_1
                INC     AL
FPU_C_1:        CALL    GCHAR
                RET
FPU_C           ENDP

                ASSUME  SI:NOTHING

              ELSE

CMD_387:        JMP     BAD_KEY

              ENDIF
                

CMD_STEP:       MOV     FS, I_CS
                MOV     ESI, I_EIP
                CALL    GET_INST
                CMP     AL, 0CCH                ; INT 3
                JNE     SHORT CMD_STEP_1
                MOV     I_EIP, ESI              ; Skip INT 3
                CALL    GET_INST
CMD_STEP_1:     CMP     AL, 0CDH                ; INT n
                JE      SHORT DO_CALL
;
; Single-step program
;
DO_STEP:        CMP     DEBUG_FLAG, FALSE
                JE      SHORT DO_STEP_0
                LEA     EDX, $DO_STEP
                CALL    GTEXT
DO_STEP_0:      CALL    DEBUG_STEP              ; Set single step mode
                MOV     SKIP_FLAG, NOT FALSE    ; Ignore breakpoint at CS:EIP
                CALL    INS_BREAKPOINTS         ; Insert breakpoints
                MOV     SKIP_FLAG, FALSE        ; Restore original value
                JMP     RUN                     ; Continue program

;
; Set breakpoint after current instruction, continue program
;
CMD_CALL:       MOV     FS, I_CS
                MOV     ESI, I_EIP
                CALL    GET_INST
                CMP     AL, 0CCH                ; INT 3
                JNE     SHORT CMD_CALL_1
                MOV     I_EIP, ESI              ; Skip INT 3
                CALL    GET_INST
CMD_CALL_1:     CMP     AL, 0C2H                ; RETN immed
                JE      SHORT DO_STEP
                CMP     AL, 0C3H                ; RETN
                JE      SHORT DO_STEP
                CMP     AL, 0CAH                ; RETF immed
                JE      SHORT DO_STEP
                CMP     AL, 0CBH                ; RETF
                JE      SHORT DO_STEP
                CMP     AL, 0CFH                ; IRET/IRETD
                JE      SHORT DO_STEP
                CMP     AL, 0E9H                ; JMP NEAR
                JE      SHORT DO_STEP
                CMP     AL, 0EAH                ; JMP FAR
                JE      SHORT DO_STEP
                CMP     AL, 0EBH                ; JMP SHORT
                JE      SHORT DO_STEP
                CMP     AL, 0FFH
                JE      SHORT CMD_CALL_FF       ; JMP reg/mem ?
DO_CALL:        CMP     DEBUG_FLAG, FALSE
                JE      SHORT DO_CALL_0
                LEA     EDX, $DO_CALL
                CALL    GTEXT
DO_CALL_0:      MOV     FS, I_CS
                MOV     ESI, I_EIP
                MOV     ECX, ESI                ; No relocation
                LEA     DI, DISASM_BUF
                CALL    GET_MODE
                CALL    DISASM
                MOV     DX, I_CS
                MOV     EAX, ESI
                CALL    SET_BREAKPOINT
                JMP     CONTINUE

CMD_CALL_FF:    MOV     AL, FS:[ESI]            ; Get mod reg r/m byte
                AND     AL, 7 SHL 3
                CMP     AL, 4 SHL 3             ; JMP DWORD PTR reg/mem
                JE      DO_STEP
                CMP     AL, 5 SHL 3             ; JMP FWORD PTR reg/mem
                JE      DO_STEP
                JMP     SHORT DO_CALL

CMD_SEARCH:     CALL    INPUT
                JC      INPUT_ERROR
                CALL    END_OF_LINE
                JZ      INPUT_ERROR             ; Future expansion: cont.
                MOV     DX, I_DS
                CALL    GET_RANGE
                JC      ERROR
                MOV     SEARCH_SEL, DX
                MOV     SEARCH_OFF, EAX
                MOV     SEARCH_LEN, ECX
                MOV     SEARCH_SIZE, 0
                LEA     DI, SEARCH_BUF
CMD_SEARCH_01:  CALL    END_OF_LINE
                JZ      SHORT CMD_SEARCH_09
                CMP     SEARCH_SIZE, LENGTH SEARCH_BUF
                JAE     INPUT_ERROR
                CMP     BYTE PTR [BX], "'"
                JE      SHORT CMD_SEARCH_02
                CALL    GET_NUMBER
                JC      INPUT_ERROR
                TEST    EAX, NOT 0FFH
                JNZ     INPUT_ERROR
                MOV     [DI], AL
                INC     DI
                INC     SEARCH_SIZE
                CALL    SKIP
                JMP     SHORT CMD_SEARCH_01

CMD_SEARCH_02:  INC     BX
                MOV     AL, [BX]
                CMP     AL, "'"
                JE      SHORT CMD_SEARCH_04
CMD_SEARCH_03:  CALL    END_OF_LINE
                JZ      INPUT_ERROR
                MOV     AL, [BX]
                INC     BX
                CMP     AL, "'"
                JE      SHORT CMD_SEARCH_05
CMD_SEARCH_04:  CMP     SEARCH_SIZE, LENGTH SEARCH_BUF
                JAE     INPUT_ERROR
                MOV     [DI], AL
                INC     DI
                INC     SEARCH_SIZE
                JMP     SHORT CMD_SEARCH_03

CMD_SEARCH_05:  CALL    DELIM
                JNZ     INPUT_ERROR
                CALL    SKIP
                JMP     SHORT CMD_SEARCH_01

CMD_SEARCH_09:  CMP     SEARCH_SIZE, 0
                JE      INPUT_ERROR
CMD_SEARCH_10:  MOV     ES, SEARCH_SEL
CMD_SEARCH_11:  MOV     EDI, SEARCH_OFF
                MOV     ECX, SEARCH_LEN
                MOV     AL, SEARCH_BUF[0]
                REPNE   SCAS BYTE PTR ES:[EDI]
                JNE     SHORT CMD_SEARCH_99
                MOV     SEARCH_OFF, EDI
                MOV     SEARCH_LEN, ECX
                INC     ECX
                CMP     ECX, SEARCH_SIZE
                JB      SHORT CMD_SEARCH_99
                MOV     ECX, SEARCH_SIZE
                DEC     ECX
                JZ      SHORT CMD_SEARCH_12
                LEA     ESI, SEARCH_BUF+1
                REPE    CMPS BYTE PTR DS:[ESI], ES:[EDI]
                JNE     SHORT CMD_SEARCH_11
CMD_SEARCH_12:  MOV     ESI, SEARCH_OFF
                DEC     ESI
                MOV     FS, SEARCH_SEL
                CALL    SHOW_DATA
                JMP     SHORT CMD_SEARCH_10

CMD_SEARCH_99:  JMP     KEY

;
; A command
;
CMD_ADDR:       CALL    INPUT
                JC      INPUT_ERROR
                CALL    END_OF_LINE
                JZ      INPUT_ERROR
                MOV     DX, I_DS
                CALL    GET_ADDR
                JC      INPUT_ERROR
                MOV     ADDR_COUNT, 0
                CALL    END_OF_LINE
                JZ      SHORT ADDR_1
                CALL    GET_RANGE_MORE
                JC      INPUT_ERROR
                CALL    END_OF_LINE
                JNZ     INPUT_ERROR
                MOV     ADDR_COUNT, ECX
ADDR_1:         MOV     ADDR_SEL, DX
                MOV     ADDR_OFF, EAX
                VERR    DX
                JNZ     INVALID_SEL
ADDR_LOOP:      MOVZX   EDX, ADDR_SEL
                LSL     ECX, EDX
                CMP     EAX, ECX
                JA      BEYOND_LIMIT
                MOV     AX, ADDR_SEL
                CALL    GWORD
                MOV     AL, ":"
                CALL    GCHAR
                MOV     EAX, ADDR_OFF
                CALL    GDWORD
                MOV     AL, " "
                CALL    GCHAR
                MOV     AX, ADDR_SEL
                MOV     EBX, ADDR_OFF
                CALL    GET_LIN
                LEA     EDX, $ADDR_FAILURE
                JC      SHORT ADDR_MSG
                MOV     ADDR_LIN, EBX
                LEA     EDX, $LINEAR
                CALL    GTEXT
                CALL    SHOW_LIN
ADDR_NEXT:      ADD     ADDR_OFF, 1000H
                SUB     ADDR_COUNT, 1000H
                JA      ADDR_LOOP
                JMP     KEY

ADDR_MSG:       CALL    GTEXT
                JMP     SHORT ADDR_NEXT

;
; V command
;
CMD_VIRT:       CALL    INPUT
                JC      INPUT_ERROR
                CALL    END_OF_LINE
                JZ      INPUT_ERROR
                MOV     AL, [BX]
                CALL    UPPER
                LEA     DI, V_PHYS
                CMP     AL, "P"
                JE      VIRT_SCAN
                LEA     DI, V_EXT
                CMP     AL, "X"
                JE      VIRT_SCAN
                CALL    GET_NUMBER
                JC      INPUT_ERROR
                MOV     ADDR_LIN, EAX
                MOV     ADDR_COUNT, 0
                CALL    SKIP
                CALL    END_OF_LINE
                JZ      SHORT VIRT_LOOP
                CALL    GET_NUMBER
                JC      INPUT_ERROR
                CALL    END_OF_LINE
                JNZ     INPUT_ERROR
                SUB     EAX, ADDR_LIN
                JB      INPUT_ERROR
                INC     EAX
                MOV     ADDR_COUNT, EAX
VIRT_LOOP:      CALL    SHOW_LIN
                ADD     ADDR_LIN, 1000H
                SUB     ADDR_COUNT, 1000H
                JA      VIRT_LOOP
                JMP     KEY

;
; VP and VX commands
;
VIRT_SCAN:      MOV     LIST_ALL, NOT FALSE
                MOV     ADDR_COUNT, 0
                INC     BX
                CALL    SKIP
                CALL    END_OF_LINE
                JZ      SHORT VSCAN_1
                MOV     LIST_ALL, FALSE
                CALL    GET_NUMBER
                JC      INPUT_ERROR
                MOV     ADDR_ARG, EAX
                CALL    SKIP
                CALL    END_OF_LINE
                JZ      SHORT VSCAN_1
                CALL    GET_NUMBER
                JC      INPUT_ERROR
                CALL    END_OF_LINE
                JNZ     INPUT_ERROR
                SUB     EAX, ADDR_ARG
                JB      INPUT_ERROR
                INC     EAX
                MOV     ADDR_COUNT, EAX
VSCAN_1:        AND     ADDR_ARG, NOT 0FFFH
VSCAN_LOOP:     CALL    SCAN_PAGE_TABLES
                ADD     ADDR_ARG, 1000H
                SUB     ADDR_COUNT, 1000H
                JA      VSCAN_LOOP
                JMP     KEY

V_PHYS          PROC    NEAR
                TEST    AX, PAGE_PRESENT OR PAGE_ALLOC
                JZ      SHORT V_PHYS_RET
                CMP     LIST_ALL, FALSE
                JNE     SHORT V_PHYS_SHOW
                AND     EAX, NOT 0FFFH
                CMP     EAX, ADDR_ARG
                JNE     SHORT V_PHYS_RET
V_PHYS_SHOW:    CALL    SHOW_LIN
V_PHYS_RET:     RET
V_PHYS          ENDP

V_EXT           PROC    NEAR
                TEST    DX, SWAP_ALLOC
                JNZ     SHORT V_EXT_CMP
                MOV     AX, DX
                AND     AX, SRC_MASK
                CMP     AX, SRC_EXEC
                JNE     SHORT V_EXT_RET
V_EXT_CMP:      CMP     LIST_ALL, FALSE
                JNE     SHORT V_EXT_SHOW
                AND     EDX, NOT 0FFFH
                CMP     EDX, ADDR_ARG
                JNE     SHORT V_EXT_RET
V_EXT_SHOW:     CALL    SHOW_LIN
V_EXT_RET:      RET
V_EXT           ENDP



SEL_TAB         DW      SEL_INVALID             ; 0000 undefined
                DW      SEL_TSS                 ; 0001 16-bit TSS
                DW      SEL_LDT                 ; 0010 LDT
                DW      SEL_TSS                 ; 0011 16-bit TSS (busy)
                DW      SEL_CALL                ; 0100 16-bit call gate
                DW      SEL_TASK                ; 0101 task gate
                DW      SEL_INT                 ; 0110 16-bit interrupt gate
                DW      SEL_TRAP                ; 0111 16-bit trap gate
                DW      SEL_INVALID             ; 1000 undefined
                DW      SEL_TSS                 ; 1001 32-bit TSS
                DW      SEL_INVALID             ; 1010 undefined
                DW      SEL_TSS                 ; 1011 32-bit TSS (busy)
                DW      SEL_CALL                ; 1100 32-bit call gate
                DW      SEL_INVALID             ; 1101 undefined
                DW      SEL_INT                 ; 1110 32-bit interrupt gate
                DW      SEL_TRAP                ; 1111 32-bit trap gate

CMD_SELECTOR:   CALL    INPUT
                JC      INPUT_ERROR
                CALL    END_OF_LINE
                JZ      INPUT_ERROR
                CALL    GET_NUMBER
                JC      INPUT_ERROR
                TEST    EAX, NOT 0FFFFH
                JNZ     INPUT_ERROR
                MOV     SEL_SEL, AX
                MOV     SEL_COUNT, 0
                CALL    SKIP
                CALL    END_OF_LINE
                JZ      SHORT SEL_1
                CALL    GET_NUMBER
                JC      INPUT_ERROR
                SUB     AX, SEL_SEL
                JB      INPUT_ERROR
                SHR     AX, 3
                INC     AX
                MOV     SEL_COUNT, AX
                CALL    SKIP
                CALL    END_OF_LINE
                JNZ     INPUT_ERROR
SEL_1:
SEL_LOOP:       MOV     AX, SEL_SEL
                CALL    GWORD
                LEA     EDX, $RPL
                CALL    GTEXT
                MOV     AX, SEL_SEL
                AND     AL, 03H
                ADD     AL, "0"
                CALL    GCHAR
                MOV     AX, SEL_SEL
                LEA     EDX, $TI_GDT
                MOV     DI, G_GDT_MEM_SEL
                LEA     SI, GDT
                TEST    AX, 04H
                JZ      SHORT SEL_2
                SLDT    DI
                MOV     SI, DI
                AND     SI, NOT 07H
                SUB     SI, G_LDT_SEL
                IMUL    SI, LDT_ENTRIES
                LEA     SI, LDTS[SI]
                LEA     EDX, $TI_LDT
SEL_2:          CALL    GTEXT
                AND     DI, NOT 07H
                MOV     AX, WORD PTR GDT[DI]    ; ... ignore limit 16..19, G
                MOV     BX, SEL_SEL
                OR      BX, 07H
                CMP     BX, AX
                JA      SEL_INVALID
                AND     BX, NOT 07H
                JZ      SEL_NULL
                ADD     SI, BX
                MOV     BX, [SI+5]
                TEST    BX, 10H                 ; D1
                JNZ     SHORT SEL_SEG
                MOV     DI, BX
                AND     DI, 0FH
                SHL     DI, 1
                JMP     SEL_TAB[DI]

SEL_LDT:        LEA     EDX, $LDT
                CALL    GTEXT
                CALL    SEL_DPL_P
                JMP     SHORT SEL_SEG_1

;
; It's a segment
;
SEL_SEG:        TEST    BX, 08H                 ; Code segment?
                JNZ     SHORT SEL_CODE
                LEA     EDX, $DATA
                CALL    GTEXT
                CALL    SEL_DPL_P
                MOV     AL, "A"
                TEST    BX, 01H                 ; Accessed
                CALL    SHOW_BIT
                MOV     AL, "W"
                TEST    BX, 02H                 ; Writable?
                CALL    SHOW_BIT
                MOV     AL, "E"
                TEST    BX, 04H                 ; Expand down?
                CALL    SHOW_BIT
                MOV     AL, "B"
                TEST    BX, 4000H               ; Big?
                CALL    SHOW_BIT
SEL_SEG_1:      MOV     AL, "G"
                TEST    BX, 8000H               ; Granularity?
                CALL    SHOW_BIT
                LEA     EDX, $BASE
                CALL    GTEXT
                MOV     AH, [SI+7]
                MOV     AL, [SI+4]
                SHL     EAX, 16
                MOV     AX, [SI+2]
                CALL    GDWORD
                LEA     EDX, $LIMIT
                CALL    GTEXT
                MOV     AL, [SI+6]
                AND     AX, 0FH
                SHL     EAX, 16
                MOV     AX, [SI+0]
                TEST    BX, 8000H               ; Granularity
                JZ      SHORT SEL_LIMIT_1
                SHL     EAX, 12
                OR      EAX, 0FFFH
SEL_LIMIT_1:    CALL    GDWORD
                JMP     SEL_DONE

SEL_CODE:       LEA     EDX, $CODE
                CALL    GTEXT
                CALL    SEL_DPL_P
                MOV     AL, "A"
                TEST    BX, 01H                 ; Accessed
                CALL    SHOW_BIT
                MOV     AL, "R"
                TEST    BX, 02H                 ; Readable?
                CALL    SHOW_BIT
                MOV     AL, "C"
                TEST    BX, 04H                 ; Conforming?
                CALL    SHOW_BIT
                MOV     AL, "D"
                TEST    BX, 4000H               ; Default?
                CALL    SHOW_BIT
                JMP     SEL_SEG_1

SEL_TRAP:       LEA     EDX, $TRAP
                JMP     SHORT SEL_INT_1

SEL_INT:        LEA     EDX, $INT
SEL_INT_1:      CALL    SEL_16_32
                CALL    GTEXT
                CALL    SEL_DPL_P
                MOV     AX, [SI+2]
                CALL    GWORD
                MOV     AL, ":"
                CALL    GCHAR
                XOR     EAX, EAX
                TEST    BX, 08H
                JZ      SHORT SEL_INT_2
                MOV     AX, [SI+6]
                SHL     EAX, 16
SEL_INT_2:      MOV     AX, [SI+0]
                CALL    GDWORD
                JMP     SEL_DONE

SEL_TSS:        CALL    SEL_16_32
                LEA     EDX, $TSS
                CALL    GTEXT
                CALL    SEL_DPL_P
                MOV     AL, "B"
                TEST    BX, 02H                 ; Busy
                CALL    SHOW_BIT
                JMP     SEL_SEG_1

SEL_CALL:       CALL    SEL_16_32
                LEA     EDX, $CALL
                CALL    GTEXT
                CALL    SEL_DPL_P
                MOV     AX, [SI+2]
                CALL    GWORD
                MOV     AL, ":"
                CALL    GCHAR
                XOR     EAX, EAX
                TEST    BX, 08H
                JZ      SHORT SEL_CALL_1
                MOV     AX, [SI+6]
                SHL     EAX, 16
SEL_CALL_1:     MOV     AX, [SI+0]
                CALL    GDWORD
                MOV     AL, " "
                MOV     AL, [SI+4]
                AND     AL, 0FH
                CALL    GBYTE
                JMP     SHORT SEL_DONE

SEL_TASK:       LEA     EDX, $TASK
                CALL    GTEXT
                CALL    SEL_DPL_P
                MOV     AX, [SI+2]
                CALL    GWORD
                JMP     SHORT SEL_DONE

SEL_NULL:       LEA     EDX, $NULL
                CALL    GTEXT
                JMP     SHORT SEL_DONE

SEL_INVALID:    LEA     EDX, $SEL_INVALID
                CALL    GTEXT
SEL_DONE:       CALL    GCRLF
                ADD     SEL_SEL, 8
                SUB     SEL_COUNT, 1
                JA      SEL_LOOP
                JMP     KEY


SEL_16_32       PROC    NEAR
                PUSH    EDX
                LEA     EDX, $SEL_32
                TEST    BX, 08H
                JNZ     SHORT SEL_16_32_1
                LEA     EDX, $SEL_16
SEL_16_32_1:    CALL    GTEXT
                POP     EDX
                RET
SEL_16_32       ENDP

SEL_DPL_P       PROC    NEAR
                PUSH    EDX
                LEA     EDX, $DPL
                CALL    GTEXT
                MOV     AX, BX
                SHR     AX, 5
                AND     AL, 03H
                ADD     AL, "0"
                CALL    GCHAR
                MOV     AL, " "
                CALL    GCHAR
                MOV     AL, "P"
                TEST    BX, 80H
                CALL    SHOW_BIT
                MOV     AL, " "
                CALL    GCHAR
                POP     EDX
                RET
SEL_DPL_P       ENDP

;
; Quit
;
CMD_QUIT:       CALL    INPUT
                JC      INPUT_ERROR
                CALL    END_OF_LINE
                MOV     AL, 0
                JZ      CMD_QUIT_0
                CALL    GET_NUMBER
                JC      INPUT_ERROR
                TEST    EAX, NOT 0FFH
                JNZ     INPUT_ERROR
CMD_QUIT_0:     MOV     AH, 4CH
                INT     21H
                JMP     SHORT $


;
; Symbol
;
CMD_SYM:        CALL    INPUT
                JC      INPUT_ERROR
                CALL    GET_NUMBER
                JC      INPUT_ERROR
                MOV     EDX, EAX
                CALL    END_OF_LINE
                JNZ     INPUT_ERROR
                MOV     DI, PROCESS_PTR
                CALL    SYM_BEFORE
                JC      KEY
                CALL    GDWORD
                MOV     AL, " "
                CALL    GCHAR
                CALL    PRINT_SYM
                CALL    GCRLF
                JMP     KEY

;
; Info
;
CMD_INFO:       CALL    INPUT
                JC      INPUT_ERROR
                CALL    END_OF_LINE
                JNZ     INPUT_ERROR
                LEA     EDX, $INFO_TITLE
                CALL    GTEXT
                LEA     BX, PROCESS_TABLE
                ASSUME  BX:PTR PROCESS
                MOV     CX, MAX_PROCESSES + 1
INFO_10:        CMP     [BX].P_STATUS, PS_NONE  ; Empty slot?
                JE      SHORT INFO_19
                MOV     AL, [BX].P_PIDX
                CALL    GBYTE
                MOV     AL, " "
                CMP     BX, PROCESS_PTR
                JNE     SHORT INFO_13
                MOV     AL, "*"
INFO_13:        CALL    GCHAR
                MOV     AL, " "
                CALL    GCHAR
                MOV     EAX, [BX].P_PID
                CALL    GDWORD
                MOV     AL, " "
                CALL    GCHAR
                MOV     EAX, [BX].P_PPID
                CALL    GDWORD
;
; File handles
;
                MOV     SI, 0
INFO_LOOP_HANDLE: 
                MOV     AL, " "
                CALL    GCHAR
                MOV     AX, [BX].P_HANDLES[SI]
                CMP     AX, NO_FILE_HANDLE
                JE      SHORT INFO_NO_HANDLE
                AAM
                XCHG    AL, AH
                ADD     AL, "0"
                CALL    GCHAR
                XCHG    AL, AH
                ADD     AL, "0"
                CALL    GCHAR
                JMP     SHORT INFO_NEXT_HANDLE

INFO_NO_HANDLE: MOV     AL, "-"
                CALL    GCHAR
                CALL    GCHAR
INFO_NEXT_HANDLE: 
                ADD     SI, 2
                CMP     SI, 2 * 19
                JB      SHORT INFO_LOOP_HANDLE
                CALL    GCRLF
;
; Next process table entry
;
INFO_19:        ADD     BX, SIZE PROCESS
                DEC     CX
                JNZ     INFO_10
                ASSUME  BX:NOTHING
                CALL    CHECK_HANDLES
                JMP     KEY

;
; Continue program
;
RUN:            CMP     DEBUG_FLAG, FALSE
                JE      SHORT RUN_0
                LEA     EDX, $RUN
                CALL    GTEXT
RUN_0:          JMP     EXCEPT_RET
                ASSUME  BP:NOTHING
DEBUG_EXCEPTION ENDP


;
; Get next instruction opcode byte, skip prefix bytes
;
; In:   FS:ESI  Pointer to code
;
; Out:  AL      Opcode byte
;       ESI     Points to next opcode byte
;
GET_INST        PROC    NEAR
GINST1:         MOV     AL, FS:[ESI]
                INC     ESI
                CMP     AL, 26H                 ; ES prefix
                JE      SHORT GINST1
                CMP     AL, 2EH                 ; CS prefix
                JE      SHORT GINST1
                CMP     AL, 36H                 ; SS prefix
                JE      SHORT GINST1
                CMP     AL, 3EH                 ; DS prefix
                JE      SHORT GINST1
                CMP     AL, 64H                 ; FS prefix
                JE      SHORT GINST1
                CMP     AL, 65H                 ; GS prefix
                JE      SHORT GINST1
                CMP     AL, 66H                 ; Operand size prefix
                JE      SHORT GINST1
                CMP     AL, 67H                 ; Adress size prefix
                JE      SHORT GINST1
                CMP     AL, 0F0H                ; LOCK prefix
                JE      SHORT GINST1
                CMP     AL, 0F2H                ; REP/REPNE prefix
                JE      SHORT GINST1
                CMP     AL, 0F3H                ; REPE prefix
                JE      SHORT GINST1
                RET
GET_INST        ENDP

;
; In:   AL=0    Display empty line if jumped
;       AL!=0   Don't display empty line
;
                ASSUME  BP:PTR ISTACKFRAME
TRACE_DISASM    PROC    NEAR
                OR      AL, AL                  ; Stopped?
                MOV     AX, I_CS                ; Get CS:EIP
                MOV     ESI, I_EIP
                JNZ     SHORT TDA2              ; Yes -> no empty line
                CMP     AX, NEXT_CS             ; Are we at next instruction?
                JNE     SHORT TDA1              ; No  -> empty line
                CMP     ESI, NEXT_EIP
                JE      SHORT TDA2              ; Yes -> no empty line
TDA1:           CALL    GCRLF                   ; Display empty line
TDA2:           MOV     FS, AX
                CALL    SHOW_DISASM             ; One line of disassembly
                MOV     NEXT_CS, FS             ; Remember address of
                MOV     NEXT_EIP, ESI           ; next instruction
                RET
                ASSUME  BP:NOTHING
TRACE_DISASM    ENDP


SHOW_DISASM     PROC    NEAR
                XOR     EAX, EAX
                MOV     AX, FS
                LSL     EAX, EAX
                CMP     ESI, EAX
                JA      SDIS_RET
                PUSH    ESI
                MOV     ECX, ESI                ; No relocation
                LEA     DI, DISASM_BUF
                CALL    GET_MODE
                CALL    DISASM
                MOV     DA_SYM_FLAG, AL
                MOV     DA_EMPTY_FLAG, AH
                MOV     DA_SYM_ADDR, EDX
                MOV     AX, FS
                CMP     AX, L_CODE_SEL          ; Symbols only for L_CODE_SEL
                JNE     SHORT SDIS_S9
                POP     EDX
                PUSH    EDX
                MOV     AL, SYM_LINE
                MOV     DI, PROCESS_PTR
                CALL    SYM_BY_ADDR
                JC      SHORT SDIS_S2
                POP     EAX
                PUSH    EAX
                PUSH    DX
                MOV     EDX, EAX
                CALL    SYM_MODULE
                JC      SHORT SDIS_S1
                CALL    PRINT_SYM
SDIS_S1:        LEA     EDX, $LINE
                CALL    GTEXT
                POP     AX
                MOVZX   EAX, AX
                CALL    DECIMAL
                MOV     AL, ":"
                CALL    GCHAR
                CALL    GCRLF
SDIS_S2:        POP     EDX
                PUSH    EDX
                MOV     AL, SYM_TEXT
                CALL    SYM_BY_ADDR
                JC      SHORT SDIS_S9
                CALL    PRINT_SYM
                MOV     AL, ":"
                CALL    GCHAR
                CALL    GCRLF
SDIS_S9:        POP     EBX
                MOV     AX, FS
                CALL    GWORD
                MOV     AL, ":"
                CALL    GCHAR
                MOV     EAX, EBX
                CALL    GDWORD
                MOV     AL, " "
                CALL    GCHAR
                MOV     CX, 8
                LEA     EDX, DISASM_BUF
SDIS1:          CMP     EBX, ESI
                JAE     SHORT SDIS5
                OR      CX, CX
                JNZ     SHORT SDIS4
                CALL    SDIS_DIS
                MOV     AL, " "
                MOV     CX, 14
SDIS2:          CALL    GCHAR
                LOOP    SDIS2
                MOV     CX, 8
SDIS4:          MOV     AL, FS:[EBX]
                CALL    GBYTE
                INC     EBX
                DEC     CX
                JMP     SHORT SDIS1

SDIS5:          JCXZ    SDIS7
                MOV     AL, " "
SDIS6:          CALL    GCHAR
                CALL    GCHAR
                LOOP    SDIS6
SDIS7:          CALL    SDIS_DIS
SDIS_RET:       RET

SDIS_DIS        PROC    NEAR
                PUSH    EBX
                OR      EDX, EDX
                JZ      SHORT SDIS_DIS9
                MOV     AL, " "
                CALL    GCHAR
                CALL    GTEXT
                MOV     AX, FS
                CMP     AX, L_CODE_SEL          ; Symbols only for L_CODE_SEL
                JNE     SHORT SDIS_DIS9
                MOV     AL, DA_SYM_FLAG
                CMP     AL, SYM_NONE
                JE      SHORT SDIS_DIS9
                MOV     EDX, DA_SYM_ADDR
                CMP     EDX, 2
                JB      SHORT SDIS_DIS9         ; Avoid symbols for 0, 1
                MOV     DI, PROCESS_PTR
                CALL    SYM_BY_ADDR
                JC      SHORT SDIS_DIS9
                MOV     AL, " "
                CALL    GCHAR
                CALL    GCHAR
                MOV     AL, ";"
                CALL    GCHAR
                MOV     AL, " "
                CALL    GCHAR
                CALL    PRINT_SYM
SDIS_DIS9:      CALL    GCRLF
                XOR     EDX, EDX
                POP     EBX
                RET
SDIS_DIS        ENDP

SHOW_DISASM     ENDP

;
; Display a symbol
;
; In:   ES:EBX  Name of symbol
;
PRINT_SYM       PROC    NEAR
                PUSH    EDX
                PUSH    DI
                LEA     DI, SYMBOL_BUF
PSYM1:          MOV     AL, ES:[EBX]
                MOV     [DI], AL
                INC     EBX
                INC     DI
                OR      AL, AL
                JNZ     SHORT PSYM1
                LEA     EDX, SYMBOL_BUF
                CALL    GTEXT
                POP     DI
                POP     EDX
                RET
PRINT_SYM       ENDP


;
; Get 16/32 mode for disassembler
;
; In:   FS      Segment register
;
; Out:  AL      0:16, 1:32
;       EAX     Changed
;
GET_MODE        PROC    NEAR
                XOR     EAX, EAX
                MOV     AX, FS
                LAR     EAX, EAX                ; Ignore flags
                TEST    EAX, 1 SHL 22
                SETNZ   AL
                RET
GET_MODE        ENDP


;
; Display the status of a bit: upper case if set, lower case if clear
;
; In:  NZ       Bit is set
;      AL       Upper case character
;
SHOW_BIT        PROC    NEAR
                JNZ     SHORT SHOW_BIT_1
                ADD     AL, "a" - "A"
SHOW_BIT_1:     CALL    GCHAR
                RET
SHOW_BIT        ENDP




;
; Bug: pages protected by swapper cause exception 0EH (and dump)
;
SHOW_DATA       PROC    NEAR
                PUSHAD
                MOV     AX, FS
                CALL    GWORD
                MOV     AL, ":"
                CALL    GCHAR
                MOV     EAX, ESI
                CALL    GDWORD
                MOV     AL," "
                CALL    GCHAR
                XOR     EDX, EDX
                MOV     DX, FS
                LSL     EDX, EDX
                MOV     CX, 16
SDATA11:        CMP     ESI, EDX
                JA      SHORT SDATA12
                MOV     AL, FS:[ESI]
                CALL    GBYTE
                JMP     SHORT SDATA13
SDATA12:        MOV     AL, " "
                CALL    GCHAR
                CALL    GCHAR
SDATA13:        MOV     AL, " "
                CALL    GCHAR
                INC     ESI
                LOOP    SDATA11
                MOV     AL, " "
                CALL    GCHAR
                SUB     ESI, 16
                MOV     CX, 16
SDATA21:        CMP     ESI, EDX
                JA      SHORT SDATA29
                MOV     AL, FS:[ESI]
                INC     ESI
                CMP     AL, 20H
                JB      SHORT SDATA22
                CMP     AL, 7FH
                JB      SHORT SDATA23
SDATA22:        MOV     AL, "."
SDATA23:        CALL    GCHAR
                LOOP    SDATA21
SDATA29:        CALL    GCRLF
                POPAD
                NOP                             ; Avoid 386 bug
                RET
SHOW_DATA       ENDP


                ASSUME  BP:PTR ISTACKFRAME
REG_DUMP        PROC    NEAR
                MOV     AL, "D"
                MOV     DX, I_DS
                CALL    RD_SEG
                MOV     AL, "E"
                MOV     DX, I_ES
                CALL    RD_SEG
                MOV     AL, "F"
                MOV     DX, I_FS
                CALL    RD_SEG
                MOV     AL, "G"
                MOV     DX, I_GS
                CALL    RD_SEG
                MOV     DX, SS
                TEST    I_CS, 3                 ; RPL=0?
                JZ      SHORT RD_1
                MOV     DX, I_SS
RD_1:           MOV     AL, "S"
                CALL    RD_SEG
                MOV     AL, " "
                CALL    GCHAR
                LEA     BX, FLAG_TAB
                MOV     EDX, I_EFLAGS
RD_F1:          MOV     CX, [BX+0]
                JCXZ    RD_F9
                MOV     AL, " "
                CALL    GCHAR
                MOV     AX, [BX+2]
                TEST    DX, CX
                JZ      SHORT RD_F2
                MOV     AX, [BX+4]
RD_F2:          CALL    GCHAR
                MOV     AL, AH
                CALL    GCHAR
                ADD     BX, 6
                JMP     SHORT RD_F1
RD_F9:          CALL    GCRLF

                MOV     BL, "X"
                MOV     AH, "A"
                MOV     EDX, I_EAX
                CALL    RD_GEN
                MOV     AH, "B"
                MOV     EDX, I_EBX
                CALL    RD_GEN
                MOV     AH, "C"
                MOV     EDX, I_ECX
                CALL    RD_GEN
                MOV     AH, "D"
                MOV     EDX, I_EDX
                CALL    RD_GEN
                CALL    GCRLF
                MOV     BL, "I"
                MOV     AH, "S"
                MOV     EDX, I_ESI
                CALL    RD_GEN
                MOV     AH, "D"
                MOV     EDX, I_EDI
                CALL    RD_GEN
                MOV     BL, "P"
                MOV     AH, "B"
                MOV     EDX, I_EBP
                CALL    RD_GEN
                LEA     EDX, I_ESP
                TEST    I_CS, 3                 ; RPL=0?
                JZ      SHORT RD_2
                MOV     EDX, I_ESP
RD_2:           MOV     AH, "S"
                CALL    RD_GEN
                CALL    GCRLF
                RET
                ASSUME  BP:NOTHING
REG_DUMP        ENDP

RD_SEG          PROC    NEAR
                CALL    GCHAR
                MOV     AL, "S"
                CALL    GCHAR
                MOV     AL, "="
                CALL    GCHAR
                MOV     AX, DX
                CALL    GWORD
                MOV     AL, " "
                CALL    GCHAR
                RET
RD_SEG          ENDP

RD_GEN          PROC    NEAR
                MOV     AL, "E"
                CALL    GCHAR
                MOV     AL, AH
                CALL    GCHAR
                MOV     AL, BL
                CALL    GCHAR
                MOV     AL, "="
                CALL    GCHAR
                MOV     EAX, EDX
                CALL    GDWORD
                MOV     AL, " "
                CALL    GCHAR
                RET
RD_GEN          ENDP


;
; In:  DI       Address of routine to be called
;
; Routine pointed to by DI:
;
; In:  EAX      Page table entry
;      EDX      Swap table entry
;      ADDR_LIN Linear address
;
SCAN_PAGE_TABLES PROC   NEAR
                MOV     ADDR_LIN, 0
                MOV     AX, G_PAGEDIR_SEL
                MOV     FS, AX
                MOV     ESI, 0
SPT_10:         MOV     EBX, FS:[ESI]
                TEST    BL, PAGE_PRESENT
                JZ      SHORT SPT_18
                MOV     EAX, FS:[ESI+4096]
                OR      EAX, EAX
                JZ      SHORT SPT_18
                PUSH    ESI
                MOV     ESI, EAX
                AND     EBX, NOT 0FFFH
                MOV     AX, G_PHYS_SEL
                MOV     ES, AX
                MOV     CX, 1024
SPT_11:         MOV     EAX, ES:[EBX]
                MOV     EDX, ES:[ESI]
                PUSH    ES
                PUSH    FS
                PUSH    EBX
                PUSH    ESI
                PUSH    CX
                PUSH    DI
                CALL    DI
                POP     DI
                POP     CX
                POP     ESI
                POP     EBX
                POP     FS
                POP     ES
                ADD     EBX, 4
                ADD     ESI, 4
                ADD     ADDR_LIN, 4096
                LOOP    SPT_11
                POP     ESI
                JMP     SHORT SPT_19

SPT_18:         ADD     ADDR_LIN, 1024 * 4096
SPT_19:         ADD     ESI, 4
                CMP     ESI, 4096
                JB      SPT_10
                RET
SCAN_PAGE_TABLES ENDP


;
; Display info about linear address (ADDR_LIN)
;
SHOW_LIN        PROC    NEAR
                MOV     EAX, ADDR_LIN
                CALL    GDWORD
                MOV     AL, " "
                CALL    GCHAR
                MOV     AX, G_PAGEDIR_SEL
                MOV     ES, AX
                MOV     EAX, ADDR_LIN
                SHR     EAX, 22                 ; Compute page directory index
                MOV     ESI, ES:[4*EAX+0]       ; Get page directory entry
                MOV     EDI, ES:[4*EAX+4096]    ; Get swap directory entry
                TEST    ESI, PAGE_PRESENT       ; Page table present?
                JNZ     SHORT SL_1              ; Yes -> continue
                LEA     EDX, $PAGE_DIR          ; Display message
                CALL    GTEXT
                JMP     SL_RET                  ; Done
SL_1:           MOV     AX, G_PHYS_SEL
                MOV     ES, AX
                AND     ESI, NOT 0FFFH          ; Page table address
                MOV     EAX, ADDR_LIN           ; Get linear address
                SHR     EAX, 12
                AND     EAX, 03FFH              ; Page table index
                LEA     ESI, [ESI+4*EAX]        ; Address of page table entry
                OR      EDI, EDI
                JZ      SHORT SL_2
                LEA     EDI, [EDI+4*EAX]        ; Address of swap table entry
SL_2:           LEA     EDX, $PHYSICAL
                CALL    GTEXT
                MOV     EBX, ADDR_LIN
                MOV     EAX, ES:[ESI]
                AND     EAX, NOT 0FFFH
                AND     EBX, 0FFFH
                OR      EAX, EBX
                CALL    GDWORD
                MOV     AL, " "
                CALL    GCHAR
                MOV     EBX, ES:[ESI]
                MOV     AL, "P"
                TEST    EBX, PAGE_PRESENT
                CALL    SHOW_BIT
                MOV     AL, "A"
                TEST    EBX, PAGE_ACCESSED
                CALL    SHOW_BIT
                MOV     AL, "D"
                TEST    EBX, PAGE_DIRTY
                CALL    SHOW_BIT
                TEST    EBX, PAGE_USER
                JNZ     SHORT SL_P1
                MOV     AL, "-"
                CALL    GCHAR
                JMP     SHORT SL_P2
SL_P1:          MOV     AL, "W"
                TEST    EBX, PAGE_WRITE
                CALL    SHOW_BIT
SL_P2:          MOV     AL, "L"
                TEST    EBX, PAGE_LOCKED
                CALL    SHOW_BIT
                MOV     AL, "X"
                TEST    EBX, PAGE_ALLOC
                CALL    SHOW_BIT
                MOV     AL, " "
                CALL    GCHAR
                OR      EDI, EDI                ; Swapper info table present?
                JZ      SHORT SL_3              ; No  -> skip
                MOV     AL, ES:[EDI]            ; Get process index
                CALL    GBYTE                   ; Display process index
                MOV     AL, " "
                CALL    GCHAR
                MOV     BX, ES:[EDI]
                AND     BX, SRC_MASK
                MOV     AL, "N"
                CMP     BX, SRC_NONE
                JE      SHORT SL_SRC
                MOV     AL, "Z"
                CMP     BX, SRC_ZERO
                JE      SHORT SL_SRC
                MOV     AL, "E"
                CMP     BX, SRC_EXEC
                JE      SHORT SL_SRC
                MOV     AL, "S"
                CMP     BX, SRC_SWAP
                JE      SHORT SL_SRC
                MOV     AL, "?"
SL_SRC:         CALL    GCHAR
                TEST    WORD PTR ES:[EDI], SWAP_ALLOC
                MOV     AL, "X"
                CALL    SHOW_BIT
                JMP     SHORT SL_4

SL_3:           MOV     AL, " "
                CALL    GCHAR
                CALL    GCHAR
                CALL    GCHAR
                CALL    GCHAR
                CALL    GCHAR
SL_4:           MOV     AL, " "
                CALL    GCHAR
                MOV     BX, ES:[EDI]            ; Get swapper info table entry
                TEST    BX, SWAP_ALLOC          ; Swap space allocated?
                JNZ     SL_EXT                  ; Yes -> show address
                AND     BX, SRC_MASK            ; Extract SRC field
                CMP     BX, SRC_EXEC            ; Load from exec file?
                JNE     SHORT SL_NS             ; No  -> don't show address
SL_EXT:         LEA     EDX, $EXTERNAL
                CALL    GTEXT
                MOV     EAX, ES:[EDI]
                AND     EAX, NOT 0FFFH
                CALL    GDWORD
                MOV     AL, " "
                CALL    GCHAR
SL_NS:          CALL    GCRLF
SL_RET:         RET
SHOW_LIN        ENDP



;
; Set breakpoint 0 at address DX:EAX
;
; In:   DX:EAX  Address of breakpoint
;
; Out:  NZ      Error
;
SET_BREAKPOINT  PROC    NEAR
                PUSH    EAX
                PUSH    EBX
                VERR    DX
                JNZ     SHORT SB1
                MOV     BREAKPOINTS[0].BP_TYPE, BP_BREAKPOINT
                MOV     BREAKPOINTS[0].BP_SEL, DX
                MOV     BREAKPOINTS[0].BP_OFF, EAX
                MOV     EBX, EAX
                MOV     AX, DX
                CALL    GET_LIN
                MOV     BREAKPOINTS[0].BP_ADDR, EBX
                XOR     EAX, EAX
SB1:            POP     EBX
                POP     EAX
                RET
SET_BREAKPOINT  ENDP


;
; Set watchpoint (or breakpoint)
;
; In:   DX:EAX  Address
;       BL      Type (BP_BREAKPOINT, BP_WATCHPOINT_?)
;       BH      Length:
;                 0 BYTE
;                 1 WORD
;                 3 DWORD
;
; Out:  SI      Pointer to breakpoint table entry (0 if table full)
;       NZ      Error
;
SET_WATCHPOINT  PROC    NEAR
                PUSH    EAX
                PUSH    CX
                VERR    DX
                JNZ     SHORT SET_WP9
                LEA     SI, BREAKPOINTS
                ASSUME  SI:PTR BREAKPOINT
                MOV     CX, 3
SET_WP1:        ADD     SI, SIZE BREAKPOINT
                CMP     [SI].BP_TYPE, BP_INACTIVE
                JE      SHORT SET_WP2
                LOOP    SET_WP1
                XOR     SI, SI                  ; Table full
                OR      CL, 1                   ; Set NZ
                JMP     SHORT SET_WP9
SET_WP2:        PUSH    EBX
                MOV     [SI].BP_TYPE, BL
                MOV     [SI].BP_LEN, BH
                MOV     [SI].BP_SEL, DX
                MOV     [SI].BP_OFF, EAX
                MOV     EBX, EAX
                MOV     AX, DX
                CALL    GET_LIN
                MOV     [SI].BP_ADDR, EBX
                POP     EBX
                XOR     EAX, EAX
SET_WP9:        POP     CX
                POP     EAX
                RET
                ASSUME  SI:NOTHING
SET_WATCHPOINT  ENDP


;
; Insert breakpoints
;
                ASSUME  BP:PTR ISTACKFRAME
INS_BREAKPOINTS PROC    NEAR
                PUSH    ESI
                PUSH    EDI
                PUSH    EBX
                CMP     DEBUG_FLAG, FALSE
                JE      SHORT INSBP_0
                LEA     EDX, $INSBRK
                CALL    GTEXT
INSBP_0:        MOV     EDI, 0                  ; DR7
                LEA     SI, BREAKPOINTS
                ASSUME  SI:PTR BREAKPOINT
                MOV     DX, 4
                MOV     CL, 0
INSBP_1:        CMP     [SI].BP_TYPE, BP_BREAKPOINT     ; Code breakpoint?
                JNE     SHORT INSBP_3           ; No  -> inactive or watchpoint
                CMP     SKIP_FLAG, FALSE        ; Skipping breakpoint?
                JE      SHORT INSBP_2           ; No  -> don't check address
                MOV     EAX, [SI].BP_OFF
                CMP     EAX, I_EIP              ; Breakpoint at CS:EIP?
                JNE     SHORT INSBP_2           ; No  -> set breakpoint
                MOV     AX, [SI].BP_SEL
                CMP     AX, I_CS
                JE      SHORT INSBP_9           ; Yes -> ignore breakpoint
INSBP_2:        MOV     BL, 0
                MOV     BH, 00H                 ; Instruction execution only
                JMP     SHORT INSBP_5
INSBP_3:        MOV     BL,[SI].BP_LEN
                MOV     BH, 03H                 ; Data reads or writes only
                CMP     [SI].BP_TYPE, BP_WATCHPOINT_R
                JE      SHORT INSBP_4
                MOV     BH, 01H                 ; Data writes only
                CMP     [SI].BP_TYPE, BP_WATCHPOINT_W
                JE      SHORT INSBP_4
                CMP     [SI].BP_TYPE, BP_WATCHPOINT_X
                JNE     SHORT INSBP_9
INSBP_4:        CALL    WATCH_GET
                MOV     [SI].BP_VAL, EAX
INSBP_5:        CMP     DEBUG_FLAG, FALSE
                JE      SHORT INSBP_6
                PUSHAD
                CALL    SHOW_BREAKPOINT
                POPAD
INSBP_6:        SHL     BL, 2
                OR      BL, BH
                MOVZX   EAX, BL
                SHL     EAX, 16
                SHL     EAX, CL
                OR      EAX, 01H                ; L0
                SHL     EAX, CL
                OR      EDI, EAX
                OR      EDI, 100H               ; LE
                MOV     EAX, [SI].BP_ADDR
                CMP     DX, 4
                JE      SHORT INSBP_ADDR_0
                CMP     DX, 3
                JE      SHORT INSBP_ADDR_1
                CMP     DX, 2
                JE      SHORT INSBP_ADDR_2
INSBP_ADDR_3:   MOV     DR3, EAX
                JMP     SHORT INSBP_9
INSBP_ADDR_1:   MOV     DR1, EAX
                JMP     SHORT INSBP_9
INSBP_ADDR_2:   MOV     DR2, EAX
                JMP     SHORT INSBP_9
INSBP_ADDR_0:   MOV     DR0, EAX
INSBP_9:        ADD     SI, SIZE BREAKPOINT
                ADD     CL, 2
                DEC     DX
                JNZ     INSBP_1
                ASSUME  SI:NOTHING
                MOV     DR7, EDI
                POP     EBX
                POP     EDI
                POP     ESI
                RET
                ASSUME  BP:NOTHING
INS_BREAKPOINTS ENDP

;
; Read watch area
;
; In:   DS:SI   Pointer to breakpoint table entry
;
; Out:  EAX     Contents
;
                ASSUME  SI:PTR BREAKPOINT
WATCH_GET       PROC    NEAR
                PUSH    ES
                PUSH    EDI
                MOV     ES, [SI].BP_SEL
                MOV     EDI, [SI].BP_OFF
                CMP     [SI].BP_LEN, 0
                JE      SHORT WATCH_GET_1
                CMP     [SI].BP_LEN, 1
                JE      SHORT WATCH_GET_2
                MOV     EAX, ES:[EDI]
WATCH_GET_RET:  POP     EDI
                POP     ES
                RET
WATCH_GET_1:    MOVZX   EAX, BYTE PTR ES:[EDI]
                JMP     SHORT WATCH_GET_RET
WATCH_GET_2:    MOVZX   EAX, WORD PTR ES:[EDI]
                JMP     SHORT WATCH_GET_RET
                ASSUME  SI:NOTHING
WATCH_GET       ENDP


;
; Input command line
;
; In:   AL      First character
;
; Out:  BX      Pointer to first non-blank character (DS=SV_DATA)
;       CY      Error (never)
;
INPUT           PROC    NEAR
                CALL    GCHAR
                LEA     BX, INPUT_BUF
                MOV     CL, 0
INPUT_LOOP:     CALL    GINCHAR
                CMP     AL, " "
                JAE     SHORT INPUT_PUT
                CMP     AL, LF
                JE      SHORT INPUT_END
                CMP     AL, CR
                JE      SHORT INPUT_END
                CMP     AL, 08H                 ; ^H
                JE      SHORT INPUT_BACK
                CMP     AL, 7FH                 ; DEL
                JE      SHORT INPUT_BACK
                CMP     AL, 15H                 ; ^U
                JE      SHORT INPUT_KILL
                CMP     AL, 18H                 ; ^X
                JE      SHORT INPUT_KILL
INPUT_BEEP:     MOV     AL, 07H
                CALL    GCHAR
                JMP     SHORT INPUT_LOOP
                

INPUT_PUT:      CMP     CL, 64
                JAE     SHORT INPUT_BEEP
                MOV     [BX], AL
                INC     BX
                INC     CL
                CALL    GCHAR
                JMP     SHORT INPUT_LOOP

INPUT_BACK:     CMP     CL, 0
                JE      SHORT INPUT_LOOP
                CALL    INPUT_DEL
                JMP     SHORT INPUT_LOOP

INPUT_END:      MOV     BYTE PTR [BX], 0
                CALL    GCRLF
                LEA     BX, INPUT_BUF
                CALL    SKIP
                CLC
                RET

INPUT_KILL:     CMP     CL, 0
                JE      SHORT INPUT_LOOP
                CALL    INPUT_DEL
                JMP     SHORT INPUT_KILL

INPUT           ENDP


INPUT_DEL       PROC    NEAR
                DEC     CL
                DEC     BX
                MOV     AL, 08H
                CALL    GCHAR
                MOV     AL, " "
                CALL    GCHAR
                MOV     AL, 08H
                CALL    GCHAR
                RET
INPUT_DEL       ENDP


;
; Fetch next character
;
; In:   BX      Pointer to input (DS=SV_DATA)
;
; Out:  AL      Character (uppercase)
;       ZR      End of line
;       BX      Advanced to next character
;
FETCH           PROC    NEAR
FETCH1:         MOV     AL, [BX]
                CALL    END_OF_LINE
                JZ      SHORT FETCH9
                INC     BX
                CALL    UPPER
                OR      AL, AL
FETCH9:         RET
FETCH           ENDP

;
; Skip blanks and TABs
;
; In:   BX      Pointer to input (DS=SV_DATA)
;
; Out:  BX      Next character
;
SKIP            PROC    NEAR
SKIP1:          CMP     BYTE PTR [BX], " "
                JE      SHORT SKIP2
                CMP     BYTE PTR [BX], TAB
                JNE     SHORT SKIP9
SKIP2:          INC     BX
                JMP     SHORT SKIP1
SKIP9:          RET
SKIP            ENDP


;
; Check for argument delimiter
;
; In:   BX      Pointer to input (DS=SV_DATA)
;
; Out:  ZR      Delimiter
;
DELIM:          CMP     BYTE PTR [BX], TAB
                JE      SHORT EOL9
                CMP     BYTE PTR [BX], " "
                JE      SHORT EOL9
;
; Check for end of line
;
; In:   BX      Pointer to input (DS=SV_DATA)
;
; Out:  ZR      End of line
;
END_OF_LINE:    CMP     BYTE PTR [BX], 00H
                JE      SHORT EOL9
                CMP     BYTE PTR [BX], LF
                JE      SHORT EOL9
                CMP     BYTE PTR [BX], CR
EOL9:           RET


;
; Parse an address range
;
; In:   BX      Pointer to input (DS=SV_DATA)
;       DX      Default selector
;
; Out:  CY      Error
;       DX      Selector
;       EAX     Offset
;       ECX     Size
;       EDX     Error message
;
GET_RANGE       PROC    NEAR
                CALL    GET_ADDR
                JC      SHORT GRNG_INPUT_ERROR
GET_RANGE_MORE::VERR    DX
                JNZ     SHORT GRNG_INVALID_SEL
                MOV     RANGE_SEL, DX
                MOV     RANGE_OFF, EAX
                CALL    GET_ADDR                ; Keep DX
                JC      SHORT GRNG_INPUT_ERROR
                CMP     DX, RANGE_SEL
                JNE     SHORT GRNG_INPUT_ERROR
                AND     EDX, 0FFFFH
                LSL     ECX, EDX
                CMP     EAX, ECX
                JA      SHORT GRNG_BEYOND_LIMIT
                SUB     EAX, RANGE_OFF
                JB      SHORT GRNG_INPUT_ERROR
                INC     EAX
                MOV     ECX, EAX
                MOV     EAX, RANGE_OFF
                CLC
                RET

GRNG_BEYOND_LIMIT:
                LEA     EDX, $BEYOND_LIMIT
                JMP     SHORT GRNG_ERROR

GRNG_INVALID_SEL:
                LEA     EDX, $INVALID_SEL
                JMP     SHORT GRNG_ERROR

GRNG_INPUT_ERROR:
                LEA     EDX, $INPUT_ERROR
GRNG_ERROR:     STC
                RET
GET_RANGE       ENDP


;
; Parse an address
;
; In:   BX      Pointer to input (DS=SV_DATA)
;       DX      Default selector
;
; Out:  CY      Error
;       DX      Selector
;       EAX     Offset
;
GET_ADDR        PROC    NEAR
                PUSH    BX
                CALL    GET_NUMBER
                JC      SHORT GET_ADDR_1
                CMP     BYTE PTR [BX], ":"
                JNE     SHORT GET_ADDR_END
                INC     BX
                MOV     DX, AX
                CALL    GET_NUMBER
                JC      SHORT GET_ADDR_1
GET_ADDR_END:   CALL    DELIM
                JZ      SHORT GET_ADDR_OK
GET_ADDR_1:     POP     BX
                PUSH    BX
                PUSH    DI
                LEA     DI, SYMBOL_BUF
                MOV     BYTE PTR [DI], "_"
                INC     DI
GET_ADDR_S1:    MOV     AL, [BX]
                CMP     AL, " "
                JE      SHORT GET_ADDR_S2
                CMP     AL, TAB
                JE      SHORT GET_ADDR_S2
                CALL    END_OF_LINE
                JZ      SHORT GET_ADDR_S2
                MOV     [DI], AL
                INC     DI
                INC     BX
                JMP     SHORT GET_ADDR_S1
GET_ADDR_S2:    MOV     BYTE PTR [DI], 0
                PUSH    BX
                MOV     DI, PROCESS_PTR
                LEA     BX, SYMBOL_BUF
                CALL    SYM_BY_NAME
                JNC     GET_ADDR_S3
                POP     BX
                POP     DI
                JMP     SHORT GET_ADDR_ERROR
GET_ADDR_S3:    POP     BX
                POP     DI
                PUSH    EAX
                CALL    DELIM
                POP     EAX
                JNZ     SHORT GET_ADDR_ERROR
                XCHG    EAX, EDX
                CMP     DL, SYM_TEXT
                MOV     DX, L_CODE_SEL
                JE      SHORT GET_ADDR_OK
                MOV     DX, L_DATA_SEL
GET_ADDR_OK:    CALL    SKIP
                CLC
                JMP     SHORT GET_ADDR_RET
GET_ADDR_ERROR: STC
GET_ADDR_RET:   ADD     ESP, 2                  ; Remove BX
                RET
GET_ADDR        ENDP

;
; Parse a value (number or register or character constant)
;
; In:   BX      Pointer to input (DS=SV_DATA)
;
; Out:  CY      Error
;       BX      Points to next character
;       EAX     Value
;
GET_NUMBER      PROC    NEAR
                PUSH    EDX
                PUSH    CX
                CMP     BYTE PTR [BX], "'"
                JE      SHORT GNUM_CHR
                CALL    GET_REG
                JNC     SHORT GNUM_REG
GNUM0:          XOR     EDX, EDX
                MOV     CX, 0
GNUM1:          MOVZX   EAX, BYTE PTR [BX]
                SUB     AL, "0"
                JB      SHORT GNUM_END
                CMP     AL, 10
                JB      SHORT GNUM2
                ADD     AL, "0"-"A"+10
                CMP     AL, 10
                JB      SHORT GNUM_END
                CMP     AL, 16
                JB      SHORT GNUM2
                ADD     AL, "A"-"a"
                CMP     AL, 10
                JB      SHORT GNUM_END
                CMP     AL, 16
                JAE     SHORT GNUM_END
GNUM2:          SHL     EDX, 4
                ADD     EDX, EAX
                INC     BX
                INC     CX
                JMP     SHORT GNUM1
GNUM_END:       JCXZ    GNUM_ERROR
                MOV     EAX, EDX
GNUM_OK:        CLC
                JMP     SHORT GNUM_RET

GNUM_REG:       XCHG    BX, DX
                CMP     AL, R8
                JE      SHORT GNUM_REG_R8
                CMP     AL, R16
                JE      SHORT GNUM_REG_R16
GNUM_REG_R32:   MOV     EAX, SS:[BX]
                JMP     SHORT GNUM_REG2
GNUM_REG_R16:   MOVZX   EAX, WORD PTR SS:[BX]
                JMP     SHORT GNUM_REG2
GNUM_REG_R8:    MOVZX   EAX, BYTE PTR SS:[BX]
GNUM_REG2:      XCHG    BX, DX
                JMP     SHORT GNUM_OK

GNUM_CHR:       INC     BX
                CALL    END_OF_LINE
                JZ      SHORT GNUM_ERROR
                MOVZX   EAX, BYTE PTR [BX]
                INC     BX
                CMP     BYTE PTR [BX], "'"
                JNZ     SHORT GNUM_ERROR
                INC     BX
                JMP     SHORT GNUM_RET

GNUM_ERROR:     STC
GNUM_RET:       POP     CX
                POP     EDX
                RET
GET_NUMBER      ENDP

;
; Parse a register name
;
; In:   BX      Pointer to input (DS=SV_DATA)
;
; Out:  CY      Error
;       DX      Pointer to register (offset into stack segment; SS:DX)
;       AX      Register size (R8, R16, R32)
;
                ASSUME  BP:PTR ISTACKFRAME
GET_REG         PROC    NEAR
                PUSH    SI
                PUSH    DI
                MOV     DI, BX
                LEA     SI, REG_TAB
                CLD
GREG1:          LODS    REG_TAB
                CMP     AL, 0FFH
                JE      SHORT GREG_LOSE
                CMP     AL, R32
                JBE     SHORT GREG10
                MOV     AH, AL
                CALL    FETCH
                CMP     AL, AH
                JE      SHORT GREG1
GREG2:          LODS    REG_TAB
                CMP     AL, R32
                JA      SHORT GREG2
GREG3:          INC     SI
                MOV     BX, DI
                JMP     SHORT GREG1

GREG10:         MOVZX   DX, BYTE PTR [SI]
                CMP     DX, 56                  ; ESP, SS
                JB      SHORT GREG11
                TEST    I_CS, 3                 ; RPL=0?
                JZ      SHORT GREG3
GREG11:         ADD     DX, BP
                XOR     AH, AH
                JMP     SHORT GREG_RET

GREG_LOSE:      MOV     BX, DI
                STC
GREG_RET:       POP     DI
                POP     SI
                RET
                ASSUME  BP:NOTHING
GET_REG         ENDP


;
; Display a number in decimal notation
;
; In:   EAX     Number
;
DECIMAL         PROC    NEAR
                PUSH    EDX
                PUSH    ECX
                CALL    DECIMAL2
                POP     ECX
                POP     EDX
                RET

DECIMAL1:       OR      EAX, EAX
                JZ      SHORT DECIMAL9
DECIMAL2:       XOR     EDX, EDX
                MOV     ECX, 10
                DIV     ECX
                PUSH    EDX
                CALL    DECIMAL1
                POP     EAX
                ADD     AL, "0"
                CALL    GCHAR
DECIMAL9:       RET                
DECIMAL         ENDP


                ASSUME  DS:SV_DATA
DEBUG_INIT      PROC    NEAR
                CMP     DEBUG_SER_FLAG, FALSE
                JE      SHORT DEBUG_INIT_RET
                MOV     GDWORD, OFFSET SV_CODE:S_DWORD
                MOV     GWORD, OFFSET SV_CODE:S_WORD
                MOV     GBYTE, OFFSET SV_CODE:S_BYTE
                MOV     GTEXT, OFFSET SV_CODE:S_TEXT
                MOV     GCRLF, OFFSET SV_CODE:S_CRLF
                MOV     GCHAR, OFFSET SV_CODE:S_CHAR
                MOV     GINCHAR, OFFSET SV_CODE:S_INCHAR
DEBUG_INIT_RET: RET
DEBUG_INIT      ENDP


                ASSUME  DS:SV_DATA
DEBUG_QUIT      PROC    NEAR
                CMP     STEP_FLAG, FALSE
                JE      SHORT DQ_RET
                LEA     EDX, $SWAP_FAULTS
                CALL    GTEXT
                MOV     EAX, SWAP_FAULTS
                CALL    DECIMAL
                LEA     EDX, $SWAP_READS
                CALL    GTEXT
                MOV     EAX, SWAP_READS
                CALL    DECIMAL
                LEA     EDX, $SWAP_WRITES
                CALL    GTEXT
                MOV     EAX, SWAP_WRITES
                CALL    DECIMAL
                LEA     EDX, $SNATCH_COUNT
                CALL    GTEXT
                MOV     EAX, SNATCH_COUNT
                CALL    DECIMAL
                LEA     EDX, $SWAP_SIZE
                CALL    GTEXT
                MOV     EAX, SWAP_SIZE
                CALL    DECIMAL
                CALL    GCRLF
                CALL    CHECK_HANDLES
DQ_RET:         RET
DEBUG_QUIT      ENDP


                ASSUME  DS:SV_DATA
B_DWORD         PROC    NEAR
                ROR     EAX, 16
                CALL    B_WORD
                ROR     EAX, 16
                CALL    B_WORD
                RET
B_DWORD         ENDP

                ASSUME  DS:SV_DATA
B_WORD          PROC    NEAR
                XCHG    AL, AH
                CALL    B_BYTE
                XCHG    AL, AH
                CALL    B_BYTE
                RET
B_WORD          ENDP


                ASSUME  DS:SV_DATA
B_BYTE          PROC    NEAR
                PUSH    AX
                SHR     AL, 4
                CALL    B_NIBBLE
                POP     AX
                PUSH    AX
                CALL    B_NIBBLE
                POP     AX
                RET
B_BYTE          ENDP

                ASSUME  DS:SV_DATA
B_NIBBLE        PROC    NEAR
                AND     AL, 0FH
                ADD     AL, 30H
                CMP     AL, 3AH
                JB      SHORT SNIB1
                ADD     AL, 7
SNIB1:          CALL    B_CHAR
                RET
B_NIBBLE        ENDP


                ASSUME  DS:SV_DATA
B_TEXT          PROC    NEAR
                PUSH    AX
B_TEXT1:        MOV     AL, DS:[EDX]
                CMP     AL, 0
                JE      SHORT B_TEXT9
                CALL    B_CHAR
                INC     EDX
                JMP     SHORT B_TEXT1
B_TEXT9:        POP     AX
                RET
B_TEXT          ENDP

                ASSUME  DS:SV_DATA
B_CHAR          PROC    NEAR
                PUSH    AX
                PUSH    BX
                CMP     AL, TAB
                JE      SHORT BC_TAB
                INC     B_COLUMN
                CMP     AL, CR
                JNE     SHORT BC_1
                MOV     B_COLUMN, 0
BC_1:           MOV     BX, 0007H
                MOV     AH, 0EH
                INT     10H
BC_RET:         POP     BX
                POP     AX
                RET

BC_TAB:         MOV     AH, B_COLUMN
                NOT     AH
                AND     AH, 7
                INC     AH
                MOV     AL, " "
BC_TAB_1:       CALL    B_CHAR
                DEC     AH
                JNZ     SHORT BC_TAB_1
                JMP     SHORT BC_RET
B_CHAR          ENDP

                ASSUME  DS:SV_DATA
B_CRLF          PROC    NEAR
                PUSH    AX
                MOV     AL, CR
                CALL    B_CHAR
                MOV     AL, LF
                CALL    B_CHAR
                POP     AX
                RET
B_CRLF          ENDP

                ASSUME  DS:SV_DATA
B_INCHAR        PROC    NEAR
                XOR     AL, AL
                XCHG    AL, B_NEXT
                TEST    AL, AL
                JNZ     SHORT B_INCHAR_RET
B_INCHAR_LOOP:  MOV     AH, 0
                INT     16H
                TEST    AL, AL
                JNZ     SHORT B_INCHAR_RET
                TEST    AH, AH
                JZ      SHORT B_INCHAR_LOOP
                MOV     B_NEXT, AH
B_INCHAR_RET:   RET
B_INCHAR        ENDP

                ASSUME  DS:SV_DATA
S_DWORD         PROC    NEAR
                ROR     EAX, 16
                CALL    S_WORD
                ROR     EAX, 16
                CALL    S_WORD
                RET
S_DWORD         ENDP

                ASSUME  DS:SV_DATA
S_WORD          PROC    NEAR
                XCHG    AL, AH
                CALL    S_BYTE
                XCHG    AL, AH
                CALL    S_BYTE
                RET
S_WORD          ENDP


                ASSUME  DS:SV_DATA
S_BYTE          PROC    NEAR
                PUSH    AX
                SHR     AL, 4
                CALL    S_NIBBLE
                POP     AX
                PUSH    AX
                CALL    S_NIBBLE
                POP     AX
                RET
S_BYTE          ENDP

                ASSUME  DS:SV_DATA
S_NIBBLE        PROC    NEAR
                AND     AL, 0FH
                ADD     AL, 30H
                CMP     AL, 3AH
                JB      SHORT SNIB1
                ADD     AL, 7
SNIB1:          CALL    S_CHAR
                RET
S_NIBBLE        ENDP


                ASSUME  DS:SV_DATA
S_TEXT          PROC    NEAR
                PUSH    AX
S_TEXT1:        MOV     AL, DS:[EDX]
                CMP     AL, 0
                JE      SHORT S_TEXT9
                CALL    S_CHAR
                INC     EDX
                JMP     SHORT S_TEXT1
S_TEXT9:        POP     AX
                RET
S_TEXT          ENDP

                ASSUME  DS:SV_DATA
S_CHAR          PROC    NEAR
                PUSH    AX
                PUSH    DX
                MOV     AH, AL
S_CHAR0:        MOV     DX, DEBUG_SER_PORT
                ADD     DX, 4
                MOV     AL, 3
                OUT     DX, AL
                INC     DX
S_CHAR1:        IN      AL, DX                  ; Read line status
                TEST    AL, 01H                 ; Input character ready?
                JZ      SHORT S_CHAR2           ; No  -> check for output
                SUB     DX, 5                   ; Receiver register
                IN      AL, DX                  ; Read input character
                CMP     AL, 13H                 ; ^S (XOFF)?
                JE      SHORT S_CHAR_XOFF       ; Yes -> stop
                CMP     AL, 11H                 ; ^Q (XON)?
                JE      SHORT S_CHAR_XON        ; Yes -> continue
                CMP     LOOK_AHEAD, 0           ; Input buffer full?
                JNE     SHORT S_CHAR0           ; Yes -> discard, repeat
                MOV     LOOK_AHEAD, AL          ; Save input character
                JMP     SHORT S_CHAR0           ; (if non-zero)

S_CHAR2:        TEST    AL, 20H                 ; Transmitter empty?
                JZ      SHORT S_CHAR1           ; No  -> repeat
                CMP     XOFF, FALSE             ; Stopped?
                JNE     SHORT S_CHAR1           ; Yes -> repeat
                SUB     DX, 5                   ; Transmitter register
                MOV     AL, AH                  ; Write character
                OUT     DX, AL                  ; to transmitter register
                POP     DX
                POP     AX
                RET

S_CHAR_XON:     MOV     XOFF, FALSE
                JMP     SHORT S_CHAR1

S_CHAR_XOFF:    MOV     XOFF, NOT FALSE
                JMP     SHORT S_CHAR1

S_CHAR          ENDP

                ASSUME  DS:SV_DATA
S_CRLF          PROC    NEAR
                PUSH    AX
                MOV     AL, CR
                CALL    S_CHAR
                MOV     AL, LF
                CALL    S_CHAR
                POP     AX
                RET
S_CRLF          ENDP


                ASSUME  DS:SV_DATA
S_INCHAR        PROC    NEAR
                PUSH    DX
                XOR     AL, AL
                XCHG    AL, LOOK_AHEAD          ; Examine input buffer
                OR      AL, AL                  ; Non-empty?
                JNZ     SHORT S_INCHAR_RET      ; Yes -> use buffered character
S_INCHAR1:      MOV     DX, DEBUG_SER_PORT
                ADD     DX, 4
                MOV     AL, 3
                OUT     DX, AL
                INC     DX                      ; Line status register
S_INCHAR2:      IN      AL, DX                  ; Read line status
                TEST    AL, 01H                 ; Input character ready?
                JZ      SHORT S_INCHAR2         ; No  -> repeat
                SUB     DX, 5                   ; Receiver register
                IN      AL, DX                  ; Read input character
                OR      AL, AL                  ; NUL?
                JZ      SHORT S_INCHAR2         ; Yes -> discard, repeat
                CMP     AL, 13H                 ; ^S (XOFF)?
                JE      SHORT S_INCHAR_XOFF     ; Yes -> stop
                CMP     AL, 11H                 ; ^Q (XON)?
                JE      SHORT S_INCHAR_XON      ; Yes -> continue
S_INCHAR_RET:   POP     DX
                RET

S_INCHAR_XON:   MOV     XOFF, FALSE
                JMP     SHORT S_INCHAR1

S_INCHAR_XOFF:  MOV     XOFF, NOT FALSE
                JMP     SHORT S_INCHAR1


S_INCHAR        ENDP



SV_CODE         ENDS

                END
