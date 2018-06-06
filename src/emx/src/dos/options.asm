;
; OPTIONS.ASM -- Parse options
;
; Copyright (c) 1991-2000 by Eberhard Mattes
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
                INCLUDE A20.INC
                INCLUDE PMIO.INC
                INCLUDE DEBUG.INC
                INCLUDE MEMORY.INC
                INCLUDE SIGNAL.INC              ; Required by PROCESS.INC
                INCLUDE PROCESS.INC
                INCLUDE LOADER.INC
                INCLUDE EXCEPT.INC
                INCLUDE RPRINT.INC
                INCLUDE VERSION.INC
                INCLUDE XMS.INC
                INCLUDE MISC.INC

                PUBLIC  TEST_FLAGS, OVERRIDE_FLAG, FP_IGNORE, MACHINE
                PUBLIC  PM_OPTIONS
                PUBLIC  RM_OPTIONS, RM_SKIP_BLANKS, USAGE

SV_DATA         SEGMENT

;
; Options
;
                DALIGN  4
TEST_FLAGS      DWORD   0                       ; Test flags
OVERRIDE_FLAG   BYTE    FALSE                   ; Override some checks
FP_IGNORE       BYTE    FALSE                   ; Don't check for coprocessor

;
; Hardware
;
MACHINE         BYTE    MACH_PC                 ; Default: AT compatible

OPTT_CALL_NOARG =       0                       ; Call function: address
OPTT_CALL_ARG   =       1                       ; Call function: address
OPTT_GFLAG      =       2                       ; Global flag: address
OPTT_LFLAG      =       3                       ; Local flag: mask

DOPTION         STRUCT
OPT_NAME        BYTE    ?
OPT_TYPE        BYTE    ?
OPT_PM          BYTE    ?
OPT_DATA        WORD    ?
DOPTION         ENDS

OPTIONS_TABLE   LABEL   DOPTION
                DOPTION  <"!", OPTT_CALL_ARG,   0, OPTF_TEST>
                DOPTION  <"a", OPTT_CALL_ARG,   1, OPTF_ACCESS>
                DOPTION  <"c", OPTT_LFLAG,      1, PF_NO_CORE>
                DOPTION  <"d", OPTT_GFLAG,      0, DISABLE_EXT_MEM>
                DOPTION  <"e", OPTT_LFLAG,      1, PF_REDIR_STDERR>
                DOPTION  <"h", OPTT_CALL_ARG,   0, OPTF_HANDLES>
                DOPTION  <"m", OPTT_CALL_ARG,   0, OPTF_MACHINE>
                DOPTION  <"o", OPTT_GFLAG,      0, STDOUT_DUMP>
                DOPTION  <"p", OPTT_GFLAG,      0, DISABLE_LOW_MEM>
                DOPTION  <"q", OPTT_LFLAG,      1, PF_QUOTE>
                DOPTION  <"r", OPTT_CALL_ARG,   1, OPTF_DRIVE>
                DOPTION  <"s", OPTT_CALL_ARG,   1, OPTF_STACK>
                DOPTION  <"t", OPTT_CALL_ARG,   1, OPTF_TRUNC>
                DOPTION  <"C", OPTT_CALL_ARG,   1, OPTF_COMMIT>
                DOPTION  <"D", OPTT_GFLAG,      0, DEBUG_FLAG>
                DOPTION  <"E", OPTT_GFLAG,      0, FP_IGNORE>
                DOPTION  <"F", OPTT_GFLAG,      0, USE_FAST_A20>
                DOPTION  <"L", OPTT_LFLAG,      1, PF_PRELOAD>
                DOPTION  <"O", OPTT_GFLAG,      0, OVERRIDE_FLAG>
                DOPTION  <"P", OPTT_GFLAG,      0, USE_A20_PATCH>
                DOPTION  <"R", OPTT_CALL_ARG,   0, OPTF_RSX>
                DOPTION  <"S", OPTT_CALL_ARG,   0, OPTF_DEBUG>
                DOPTION  <"V", OPTT_CALL_NOARG, 0, OPTF_VERSION>
                DOPTION  <"X", OPTT_GFLAG,      0, DISABLE_XMS_MEM>
                DOPTION  <"Z", OPTT_LFLAG,      1, PF_DONT_ZERO>
                DOPTION  <0, 0, 0>

;
; Program titel
;
$TITLE          BYTE    "emx ", VERSION, " (rev ", REV_INDEX_TXT, ")"
                BYTE    " -- Copyright (c) 1991-2000 by Eberhard Mattes"
                BYTE    CR, LF, 0

$USAGE          BYTE    "Usage: emx [-cdeoqOV] [-s<stack_size>] "
                BYTE    "<program> [<arguments>]", CR, LF, 0

SV_DATA         ENDS

SV_CODE         SEGMENT

                ASSUME  CS:SV_CODE, DS:NOTHING

;
; Skip over blanks
;
; In:   ES:SI   Pointer to string
;
; Out:  ES:SI   Pointer to first non-blank character
;       AL      First non-blank character
;
PM_SKIP_BLANKS  PROC    NEAR
PSB_1:          MOV     AL, ES:[SI]
                INC     SI
                CMP     AL, " "
                JE      SHORT PSB_1
                CMP     AL, TAB
                JE      SHORT PSB_1
                DEC     SI
                RET
PM_SKIP_BLANKS  ENDP


;
; Parse options.
;
; In:   ES:SI   Pointer to null-terminated string
;       DI      Pointer to process table entry
;
; Out:  CY      Error
;
                ASSUME  DS:SV_DATA
                ASSUME  DI:PTR PROCESS
PM_OPTIONS      PROC    NEAR
                PUSH    BX
;
; We're looking at the beginning of an argument (or at the whitespace
; preceding the argument) or at the end of the string.  If the argument
; does not start with a dash, return.
;
PMO_MAIN:       CALL    PM_SKIP_BLANKS          ; Skip blanks
                OR      AL, AL                  ; End of line?
                JZ      PMO_END                 ; Yes -> done
                CMP     AL, "-"                 ; Option?
                JNE     PMO_ERROR               ; No  -> error
                INC     SI                      ; Skip dash
;
; We're looking at the name of an option.  It's either preceded by
; a dash or by another option which does not take an argument.
;
PMO_OPTION:     MOV     AL, ES:[SI]             ; Get name
                OR      AL, AL                  ; End of line?
                JZ      PMO_ERROR
                INC     SI                      ; Skip name
                LEA     BX, OPTIONS_TABLE       ; Search table
                ASSUME  BX:PTR DOPTION
PMO_FIND:       CMP     [BX].OPT_NAME, 0        ; End of table?
                JE      PMO_ERROR               ; Yes -> error
                CMP     [BX].OPT_NAME, AL       ; Matching entry?
                JE      SHORT PMO_FOUND         ; Yes -> found
                ADD     BX, SIZE DOPTION        ; Move to next table entry
                JMP     PMO_FIND                ; Loop through the table

;
; We've found a table entry for the current option.
;
PMO_FOUND:      CMP     [BX].OPT_PM, 0          ; Is it a PM option?
                JE      SHORT PMO_SKIP          ; No  -> ignore & skip it
                CMP     [BX].OPT_TYPE, OPTT_CALL_ARG    ; Call handler?
                JE      SHORT PMO_CALL_ARG      ; Yes ->
                CMP     [BX].OPT_TYPE, OPTT_CALL_NOARG  ; Call handler?
                JE      SHORT PMO_CALL_NOARG    ; Yes ->
                CMP     [BX].OPT_TYPE, OPTT_LFLAG ; Local flag?
                JE      SHORT PMO_LFLAG         ; Yes -> set it
                JMP     SHORT PMO_ERROR

;
; Set a local flag. OPT_DATA is the bit mask for P_FLAGS
;
PMO_LFLAG:      MOVZX   EAX, [BX].OPT_DATA      ; Get bit mask
                OR      [DI].P_FLAGS, EAX       ; and set the flag
;
; Look for next option.  The option we've just handled does not take
; an argument, therefore we can cluster options without requiring a
; blank and a dash.
;
PMO_NEXT:       CMP     BYTE PTR ES:[SI], 0     ; End of line?
                JE      SHORT PMO_END           ; Yes -> done
                CMP     BYTE PTR ES:[SI], " "   ; Blank?
                JE      PMO_MAIN                ; Yes -> new argument
                CMP     BYTE PTR ES:[SI], TAB   ; Tab?
                JE      PMO_MAIN                ; Yes -> new argument
                JMP     PMO_OPTION              ; Clustered options

;
; Call handler for an option with argument.
;
PMO_CALL_ARG:   CALL    [BX].OPT_DATA           ; Call the handler function
                JC      SHORT PMO_ERROR         ; Error -> return
                CMP     BYTE PTR ES:[SI], 0     ; End of line?
                JE      SHORT PMO_END           ; Yes -> done
                CMP     BYTE PTR ES:[SI], " "   ; Blank?
                JE      SHORT PMO_MAIN          ; Yes -> new argument
                CMP     BYTE PTR ES:[SI], TAB   ; Tab?
                JE      SHORT PMO_MAIN          ; Yes -> new argument
                JMP     SHORT PMO_ERROR         ; Error

;
; Call handler for an option without argument.
;
PMO_CALL_NOARG: CALL    [BX].OPT_DATA           ; Call the handler function
                JC      SHORT PMO_ERROR         ; Error -> return
                JMP     SHORT PMO_NEXT          ; Next option

;
; Skip an option.
;
PMO_SKIP:       CMP     [BX].OPT_TYPE, OPTT_CALL_ARG    ; With argument?
                JNE     PMO_NEXT                ; No  -> simply ignore it
;
; Skip the argument.
;
PMO_SKIP_ARG:   MOV     AL, ES:[SI]             ; Fetch next character
                OR      AL, AL                  ; End of line?
                JZ      SHORT PMO_END           ; Yes -> done
                CMP     AL, " "                 ; Blank?
                JE      PMO_MAIN                ; Yes -> new argument
                CMP     AL, TAB                 ; Tab?
                JE      PMO_MAIN                ; Yes -> new argument
                INC     SI                      ; Skip the character
                JMP     SHORT PMO_SKIP_ARG      ; Repeat

;
; Done.
;
PMO_END:        CLC
PMO_RET:        POP     BX
                RET

;
; Error.
;
PMO_ERROR:      STC
                JMP     PMO_RET

                ASSUME  BX:NOTHING
PM_OPTIONS      ENDP

;
; -C option
;
OPTF_COMMIT     PROC    NEAR
                MOV     EAX, 0                  ; Default: 0
                MOV     AL, ES:[SI]
                CALL    PM_OPT_ISEND
                JE      SHORT OC_1
                CALL    PM_OPT_NUMBER
                JC      SHORT OC_ERROR
                CMP     EAX, 512*1024           ; 512 MB maximum
                JA      SHORT OC_ERROR
                SHL     EAX, 10                 ; Multiply by 1K
                ADD     EAX, 0FFFH              ; Round to multiple
                AND     EAX, NOT 0FFFH          ; of page size (4096)
OC_1:           MOV     [DI].P_COMMIT_SIZE, EAX ; Set size
                OR      [DI].P_FLAGS, PF_COMMIT ; Set flag
                CLC
                RET
OC_ERROR:       STC
                RET
OPTF_COMMIT     ENDP



;
; -a option
;
OPTF_ACCESS     PROC    NEAR
                MOV     AH, 0
OA_1:           MOV     AL, ES:[SI]
                MOV     DL, HW_ACCESS_CODE
                CMP     AL, "c"
                JE      SHORT OA_2
                MOV     DL, HW_ACCESS_MEM
                CMP     AL, "m"
                JE      SHORT OA_2
                MOV     DL, HW_ACCESS_MEM OR HW_ACCESS_WRITE
                CMP     AL, "w"
                JE      SHORT OA_2
                MOV     DL, HW_ACCESS_IO
                CMP     AL, "i"
                JE      SHORT OA_2
                CMP     AH, 0
                JE      SHORT OA_ERROR
                OR      [DI].P_HW_ACCESS, AH
                CLC
                RET

OA_2:           OR      AH, DL
                INC     SI
                JMP     SHORT OA_1

OA_ERROR:       STC
                RET
OPTF_ACCESS     ENDP

;
; -r option
;
OPTF_DRIVE      PROC    NEAR
                MOV     AL, ES:[SI]
                SUB     AL, "A"
                CMP     AL, 26
                JB      SHORT OD_1
                MOV     AL, ES:[SI]
                SUB     AL, "a"
                CMP     AL, 26
                JAE     SHORT OD_ERROR
OD_1:           ADD     AL, "A"
                MOV     [DI].P_DRIVE, AL
                INC     SI
                CLC
                RET

OD_ERROR:       STC
                RET
OPTF_DRIVE      ENDP

;
; -s option
;
OPTF_STACK      PROC    NEAR
                CALL    PM_OPT_NUMBER
                JC      SHORT OS_ERROR
                CMP     EAX, 8                  ; 8 KB minimum
                JB      SHORT OS_ERROR
                CMP     EAX, 512*1024           ; 512 MB maximum
                JA      SHORT OS_ERROR
                SHL     EAX, 10                 ; Multiply by 1K
                ADD     EAX, 0FFFH              ; Round to multiple
                AND     EAX, NOT 0FFFH          ; of page size (4096)
                MOV     [DI].P_STACK_SIZE, EAX  ; Set stack size
                CLC
                RET
OS_ERROR:       STC
                RET
OPTF_STACK      ENDP

;
; -t option
;
OPTF_TRUNC      PROC    NEAR
                XOR     EAX, EAX
                MOV     AL, ES:[SI]
                CALL    PM_OPT_ISEND
                JZ      SHORT OT_ALL
                CMP     AL, "-"
                JE      OT_MINUS
                MOV     CH, 1
OT_LOOP:        SUB     AL, "A"
                CMP     AL, 26
                JB      SHORT OT_LETTER
                MOV     AL, ES:[SI]
                SUB     AL, "a"
                CMP     AL, 26
                JB      SHORT OT_LETTER
                CMP     BYTE PTR ES:[SI], "/"
                JNE     SHORT OT_ERROR
                MOV     AL, -1
OT_LETTER:      INC     AL
                AND     AL, 1FH
                TEST    CH, CH
                JZ      OT_CLEAR
                BTS     [DI].P_TRUNC, EAX
OT_NEXT:        INC     SI
                MOV     AL, ES:[SI]
                CALL    PM_OPT_ISEND
                JNZ     OT_LOOP
                CLC
                RET

OT_CLEAR:       BTR     [DI].P_TRUNC, EAX
                JMP     OT_NEXT

OT_MINUS:       INC     SI
                MOV     AL, ES:[SI]
                CALL    PM_OPT_ISEND
                JZ      SHORT OT_ZERO
                MOV     CH, 0
                JMP     OT_LOOP

OT_ZERO:        MOV     [DI].P_TRUNC, 0
                CLC
                RET

OT_ALL:         MOV     [DI].P_TRUNC, NOT 0
                CLC
                RET

OT_ERROR:       STC
                RET
OPTF_TRUNC      ENDP

                ASSUME  DI:NOTHING

;
; Set the ZR flag if AL ends the argument of an option
;
PM_OPT_ISEND    PROC    NEAR
                TEST    AL, AL
                JZ      SHORT POE_RET
                CMP     AL, " "
                JE      SHORT POE_RET
                CMP     AL, TAB
POE_RET:        RET
PM_OPT_ISEND    ENDP

;
; Parse a number for options (blank or zero terminated)
;
; In:   ES:SI   Pointer to string
;
; Out:  EAX     Number
;       CY      Error
;       ES:SI   Points to blank or zero
;
PM_OPT_NUMBER   PROC    NEAR
                PUSH    CX
                PUSH    EDX
                XOR     CX, CX
                XOR     EAX, EAX
PON_1:          MOVZX   EDX, BYTE PTR ES:[SI]
                OR      DL, DL
                JE      SHORT PON_END
                CMP     DL, " "
                JE      SHORT PON_END
                CMP     DL, TAB
                JE      SHORT PON_END
                SUB     DL, "0"
                CMP     DL, 9
                JA      SHORT PON_ERROR
                IMUL    EAX, 10
                JC      SHORT PON_ERROR
                ADD     EAX, EDX
                JC      SHORT PON_ERROR
                INC     SI
                INC     CX
                JMP     PON_1
PON_END:        OR      CX, CX
                JNZ     SHORT PON_RET
PON_ERROR:      STC
PON_RET:        POP     EDX
                POP     CX
                RET
PM_OPT_NUMBER   ENDP

SV_CODE         ENDS


INIT_CODE       SEGMENT

                ASSUME  CS:INIT_CODE, DS:NOTHING

;
; Skip over blanks
;
; In:   ES:SI   Pointer to string
;
; Out:  ES:SI   Pointer to first non-blank character
;       AL      First non-blank character
;
RM_SKIP_BLANKS  PROC    NEAR
RSB_1:          MOV     AL, ES:[SI]
                INC     SI
                CMP     AL, " "
                JE      SHORT RSB_1
                CMP     AL, TAB
                JE      SHORT RSB_1
                DEC     SI
                RET
RM_SKIP_BLANKS  ENDP


;
; Parse startup options.  Jump to USAGE if there is an error.
;
; In:   ES:SI   Pointer to null-terminated string
;
; Out:  ES:SI   Pointer to first non-option argument
;
                ASSUME  DS:SV_DATA
RM_OPTIONS      PROC    NEAR
;
; We're looking at the beginning of an argument (or at the whitespace
; preceding the argument) or at the end of the string.  If the argument
; does not start with a dash, return.
;
RMO_MAIN:       CALL    RM_SKIP_BLANKS          ; Skip blanks
                CMP     AL, "-"                 ; Option?
                JNE     RMO_END                 ; No  -> done
                INC     SI                      ; Skip dash
;
; We're looking at the name of an option.  It's either preceded by
; a dash or by another option which does not take an argument.
;
RMO_OPTION:     MOV     AL, ES:[SI]             ; Get and skip name (skipping 0
                INC     SI                      ; is benign: we jump to USAGE)
                LEA     BX, OPTIONS_TABLE       ; Search table
                ASSUME  BX:PTR DOPTION
RMO_FIND:       CMP     [BX].OPT_NAME, 0        ; End of table?
                JE      SHORT RMO_ERROR         ; Yes -> error
                CMP     [BX].OPT_NAME, AL       ; Matching entry?
                JE      SHORT RMO_FOUND         ; Yes -> found
                ADD     BX, SIZE DOPTION        ; Move to next table entry
                JMP     RMO_FIND                ; Loop through the table

;
; We've found a table entry for the current option.
;
RMO_FOUND:      CMP     [BX].OPT_PM, 0          ; Is it a PM option?
                JNE     SHORT RMO_SKIP          ; Yes -> ignore & skip it
                CMP     [BX].OPT_TYPE, OPTT_CALL_ARG    ; Call handler?
                JE      SHORT RMO_CALL_ARG      ; Yes ->
                CMP     [BX].OPT_TYPE, OPTT_CALL_NOARG  ; Call handler?
                JE      SHORT RMO_CALL_NOARG    ; Yes ->
                CMP     [BX].OPT_TYPE, OPTT_GFLAG ; Global flag?
                JE      SHORT RMO_GFLAG         ; Yes -> set it
RMO_ERROR:      JMP     USAGE                   ; Error, abort!

;
; Set a global flag. OPT_DATA is the offset in the SV_DATA segment.
;
RMO_GFLAG:      MOV     DI, [BX].OPT_DATA       ; Get pointer
                MOV     BYTE PTR [DI], NOT FALSE ; and set the flag to true
;
; Look for next option.  The option we've just handled does not take
; an argument, therefore we can cluster options without requiring a
; blank and a dash.
;
RMO_NEXT:       CMP     BYTE PTR ES:[SI], 0     ; End of line?
                JE      SHORT RMO_END           ; Yes -> done
                CMP     BYTE PTR ES:[SI], " "   ; Blank?
                JE      RMO_MAIN                ; Yes -> new argument
                CMP     BYTE PTR ES:[SI], TAB   ; Tab?
                JE      RMO_MAIN                ; Yes -> new argument
                JMP     RMO_OPTION              ; Clustered options

;
; Call handler for an option with argument.
;
RMO_CALL_ARG:   CALL    [BX].OPT_DATA           ; Call the handler function
                CMP     BYTE PTR ES:[SI], 0     ; End of line?
                JE      SHORT RMO_END           ; Yes -> done
                CMP     BYTE PTR ES:[SI], " "   ; Blank?
                JE      SHORT RMO_MAIN          ; Yes -> new argument
                CMP     BYTE PTR ES:[SI], TAB   ; Tab?
                JE      SHORT RMO_MAIN          ; Yes -> new argument
                JMP     RMO_ERROR               ; Error

;
; Call handler for an option without argument.
;
RMO_CALL_NOARG: CALL    [BX].OPT_DATA           ; Call the handler function
                JMP     SHORT RMO_NEXT          ; Next option

;
; Skip an option.
;
RMO_SKIP:       CMP     [BX].OPT_TYPE, OPTT_CALL_ARG    ; With argument?
                JNE     RMO_NEXT                ; No  -> simply ignore it
;
; Skip the argument.
;
RMO_SKIP_ARG:   CALL    RM_SKIP_ARG             ; Fetch next non-white char
                OR      AL, AL
                JNZ     SHORT RMO_MAIN
;
; Done.
;
RMO_END:        RET
                ASSUME  BX:NOTHING
RM_OPTIONS      ENDP

;
; Skip the argument of an option
;
; In:  ES:SI    Pointer to string
;
; Out: SI       Pointer to next non-white character
;      AL       Next non-white character
;
RM_SKIP_ARG     PROC    NEAR
SKIP_LOOP:      MOV     AL, ES:[SI]             ; Fetch next character
                OR      AL, AL                  ; End of line?
                JZ      SHORT FIN               ; Yes -> done
                CMP     AL, " "                 ; Blank?
                JE      SHORT FIN               ; Yes -> done
                CMP     AL, TAB                 ; Tab?
                JE      SHORT FIN               ; Yes -> done
                INC     SI                      ; Skip the character
                JMP     SHORT SKIP_LOOP         ; Repeat
FIN:            RET
RM_SKIP_ARG     ENDP

;
; Parse a number for options (blank or zero terminated)
;
; In:   ES:SI   Pointer to string
;
; Out:  EAX     Number
;       ES:SI   Points to blank or zero
;
RM_OPT_NUMBER   PROC    NEAR
                PUSH    CX
                PUSH    EDX
                XOR     CX, CX
                XOR     EAX, EAX
RON_1:          MOVZX   EDX, BYTE PTR ES:[SI]
                OR      DL, DL
                JE      SHORT RON_END
                CMP     DL, " "
                JE      SHORT RON_END
                SUB     DL, "0"
                CMP     DL, 9
                JA      USAGE
                IMUL    EAX, 10
                JC      USAGE
                ADD     EAX, EDX
                JC      USAGE
                INC     SI
                INC     CX
                JMP     RON_1
RON_END:        OR      CX, CX
                JZ      USAGE
                POP     EDX
                POP     CX
                RET
RM_OPT_NUMBER   ENDP

;
; -! option
;
OPTF_TEST       PROC    NEAR
                CALL    RM_OPT_NUMBER
                MOV     TEST_FLAGS, EAX
                RET
OPTF_TEST       ENDP

;
; -h option
;
OPTF_HANDLES    PROC    NEAR
                CALL    RM_OPT_NUMBER
                CMP     EAX, 10
                JB      USAGE
                CMP     EAX, 65536
                JA      USAGE
                MOV     BX, AX
                MOV     AH, DOS_MAJOR
                MOV     AL, DOS_MINOR
                CMP     AX, 031EH               ; 3.30 or later?
                JB      SHORT OH_RET            ; No  -> ignore
                MOV     AH, 67H                 ; Set handle count
                INT     21H
OH_RET:         RET
OPTF_HANDLES    ENDP

;
; -m option
;
OPTF_MACHINE    PROC    NEAR
                CALL    RM_OPT_NUMBER
                CMP     EAX, MACH_MAX           ; Valid machine code?
                JA      USAGE                   ; No  -> error
                MOV     MACHINE, AL             ; Set machine code
                RET
OPTF_MACHINE    ENDP


;
; -R option (rsx options, ignored by emx)
;
OPTF_RSX        PROC    NEAR
                CALL    RM_SKIP_ARG             ; Skip the argument if any
                RET
OPTF_RSX        ENDP


;
; -S option (only available if debugger is loaded)
;
OPTF_DEBUG      PROC    NEAR
                CMP     DEBUG_AVAIL, FALSE      ; Debugger loaded?
                JE      USAGE                   ; No  -> error
                MOV     AL, ES:[SI]
                INC     SI
                MOV     DX, 03F8H               ; COM1
                CMP     AL, "1"
                JE      SHORT OD_1
                MOV     DX, 02F8H               ; COM2
                CMP     AL, "2"
                JE      SHORT OD_1
                DEC     SI                      ; Back up
                MOV     DEBUG_SER_FLAG, FALSE
                JMP     SHORT OD_2

OD_1:           MOV     DEBUG_SER_PORT, DX      ; Set port address
                MOV     DEBUG_SER_FLAG, NOT FALSE
OD_2:           MOV     STEP_FLAG, NOT FALSE
                RET
OPTF_DEBUG      ENDP


;
; -V option
;
OPTF_VERSION    PROC    NEAR
                LEA     DX, $TITLE
                CALL    RTEXT
                RET
OPTF_VERSION    ENDP


;
;
;
USAGE           PROC    NEAR
                XOR     EDX, EDX
                LEA     DX, $USAGE
                CALL    RTEXT
                MOV     AL, 1
                JMP     EXIT
USAGE           ENDP

INIT_CODE       ENDS

                END
