;
; PROCESS.ASM -- Manage processes
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

__PROCESS       =       1
                INCLUDE EMX.INC
                INCLUDE LOADER.INC
                INCLUDE SIGNAL.INC
                INCLUDE PROCESS.INC
                INCLUDE PROFIL.INC
                INCLUDE TABLES.INC
                INCLUDE SEGMENTS.INC
                INCLUDE PAGING.INC
                INCLUDE PMINT.INC
                INCLUDE PMIO.INC
                INCLUDE FILEIO.INC
                INCLUDE MISC.INC
                INCLUDE UTILS.INC
                INCLUDE ERRORS.INC

                PUBLIC  PROCESS_TABLE, PROC0, PROCESS_PTR, PROCESS_SIG
                PUBLIC  PROCESS_FP, PROCESS_FPUEMU, PROCESS_FPUCLIENT
                PUBLIC  FPUEMU_STATE, EMX_DIR, NP1
                PUBLIC  VIDEO_LIN, TIMER_TABLE, TIMER_MIN, TIMER_TICKS
                PUBLIC  NEW_PROCESS, CREATE_PID, FIND_EXEC
                PUBLIC  REMOVE_PROCESS, ZOMBIE_PROCESS, KILL_PROCESS
                PUBLIC  FIND_PROCESS, WAIT_PROCESS
                PUBLIC  SAVE_PROCESS, REST_PROCESS, SET_TIMER, SLEEP
                PUBLIC  PROCESS_INIT


SV_DATA         SEGMENT

NO_PID          =       0FFFFFFFFH
MIN_PID         =       2

NEXT_PID        DD      MIN_PID                 ; Next process ID
PROCESS_PTR     DW      ?                       ; PTE of current process
PROCESS_SIG     DW      ?                       ; Ditto, not changed for I/O
PROCESS_FP      DW      ?                       ; The coprocessor contains
                                                ; the status of this process
PROCESS_FPUEMU  DW      ?                       ; FPU emulator PTE
PROCESS_FPUCLIENT DW    ?                       ; FPU client PTE
VIDEO_LIN       DD      ?                       ; Linear address of video mem

TIMER_MIN       DD      0
TIMER_TICKS     DD      0

CLOCK_LO        DWORD   0
CLOCK_HI        DWORD   0

;
; The offset of this table must be > 1 !
; (PROCESS_PTR = NO_PROCESS = 0 means handle translation turned off,
;  PROCESS_PTR = NO_WPROCESS = 1 is a special return value of WAIT_PROCESS)
;
PROCESS_TABLE   DB      MAX_PROCESSES DUP (SIZE PROCESS DUP (0))
;
; Dummy process (used for starting first process).  This must follow
; directly after PROCESS_TABLE, see MAP_TRANSLATE.
;
PROC0           LABEL   PROCESS
                DB      SIZE PROCESS DUP (0)

;
; This doesn't work (linker bug?):
;
; PROCESS_TABLE PROCESS MAX_PROCESSES DUP (<NO_PID>)
; PROC0           PROCESS <>
;

;
; The table of timers
;
                DALIGN  4
TIMER_TABLE     TIMER MAX_TIMERS DUP (<TT_NONE, NO_PROCESS>)

;
; Temporary variables
;
ARGV_SIZE       DD      ?
ENVP_SIZE       DD      ?

;
; This is the name of the directory from which emx.exe was loaded.
; It is empty if emx.exe is bound to the executable.  The directory
; name ends with a backslash.
;
EMX_DIR         DB      100 DUP (?)

;
; The result of FIND_EXEC is stored here.
;
EXEC_FNAME      BYTE    256 DUP (?)

;
; State of the FPU emulator
;
FPUEMU_STATE    DB      FES_OFF

;
; File name of the FPU emulator
;
$FPUEMU_NAME    DB      "emxfpemu", 0

;
; Arguments to pass to the FPU emulator (argc=0).
;
$FPUEMU_ARG     DB      0, 0

$PROC_FULL      DB      "Too many processes", CR, LF, 0
$INV_MODE       DB      "Invalid spawn mode", CR, LF, 0

;
; This structure is used for starting the initial process and
; the FPU emulator.  Some members initialized by emx.asm for
; the initial process are later used again without modification
; for starting the FPU emulator: NP_ENV_OFF, NP_ENV_SEL, NP_ENV_COUNT,
; NP_ENV_SIZE, NP_PARENT, and NP_FPROC.
;
NP1             NEW_PROC <>

SV_DATA         ENDS


SV_CODE         SEGMENT

                ASSUME  CS:SV_CODE, DS:NOTHING

NP_FRAME_SIZE   =       4
NP_ARGS         EQU     (WORD PTR [BP-2])

;
; In:   SI      Pointer to arguments (STRUC NEW_PROC)
;
; Out:  DI      Pointer to process entry
;       AX      Return code (0=ok)
;       EDX     Error message (for AX > 0)
;
; Note: This function is not reentrant (uses global variables; GDT, for
;       instance).
;
                ASSUME  DS:SV_DATA
                ASSUME  SI:PTR NEW_PROC
NEW_PROCESS     PROC    NEAR
                PUSH    EBP                     ; Build stack frame
                MOV     BP, SP                  ; (we use some local
                SUB     ESP, NP_FRAME_SIZE      ; variables)
                MOV     NP_ARGS, SI             ; Save pointer to param. table
;
                CALL    NP_CHECK_MODE
                OR      AX, AX
                JNZ     SHORT NP_RET
                CALL    NP_FIND_SLOT
                OR      AX, AX
                JNZ     SHORT NP_RET
                ASSUME  DI:PTR PROCESS
                CALL    CREATE_PID
                CALL    NP_SET_TAB
                CALL    NP_SEGMENTS

                MOV     SI, NP_ARGS
                MOV     AX, [SI].NP_FNAME_SEL   ; File name, selector
                MOV     EDX, [SI].NP_FNAME_OFF  ; File name, offset
                CALL    LOADER
                OR      AX, AX                  ; Error?
                JZ      SHORT LOAD_OK
                MOV     [DI].P_PID, NO_PID      ; Remove process
                MOV     [DI].P_STATUS, PS_NONE
                JMP     SHORT NP_RET

LOAD_OK:        CALL    NP_FILES
                CALL    NP_SIGNALS
                CALL    NP_STACK
                CALL    NP_REGS
                XOR     AX, AX                  ; Success
NP_RET:         ADD     ESP, NP_FRAME_SIZE
                POP     EBP
                RET
                ASSUME  SI:NOTHING
                ASSUME  DI:NOTHING
NEW_PROCESS     ENDP

;
; Check mode
;
                ASSUME  SI:PTR NEW_PROC
NP_CHECK_MODE   PROC    NEAR
                XOR     AX, AX
                CMP     BYTE PTR [SI].NP_MODE1, NP_SPAWN_SYNC
                JE      SHORT NP_CM_RET
                CMP     BYTE PTR [SI].NP_MODE1, NP_SPAWN_ASYNC
                JE      SHORT NP_CM_RET
                CMP     BYTE PTR [SI].NP_MODE1, NP_EXEC
                JE      SHORT NP_CM_RET
                CMP     BYTE PTR [SI].NP_MODE1, NP_DEBUG
                JE      SHORT NP_CM_RET
                MOV     AX, EINVAL
                LEA     EDX, $INV_MODE
NP_CM_RET:      RET
                ASSUME  SI:NOTHING
NP_CHECK_MODE   ENDP


;
; Find empty process table slot
;
; Out: DI       Pointer to process table entry
; 
NP_FIND_SLOT    PROC    NEAR
                XOR     AX, AX                  ; No error
                LEA     DI, PROCESS_TABLE       ; The table of processes
                ASSUME  DI:PTR PROCESS
                MOV     DL, 1                   ; First process index
                MOV     CX, MAX_PROCESSES       ; There are this many slots
NP_FS_LOOP:     CMP     [DI].P_STATUS, PS_NONE  ; Empty slot?
                JE      SHORT NP_FS_RET         ; Yes -> use it!
                ADD     DI, SIZE PROCESS        ; Next slot
                INC     DL                      ; Next process index
                LOOP    NP_FS_LOOP              ; Repeat
                ASSUME  DI:NOTHING
                MOV     AX, EAGAIN              ; Error:
                LEA     EDX, $PROC_FULL         ; Too many processes
NP_FS_RET:      RET                             ; Done
NP_FIND_SLOT    ENDP


;
; Create unique process ID
;
                TALIGN  2
CREATE_PID      PROC    NEAR
                PUSH    BX
NP_CP_LOOP:     MOV     EAX, NEXT_PID           ; Get next process ID
                INC     NEXT_PID
                CMP     EAX, NO_PID             ; Wrap around?
                JE      SHORT NP_CP_WRAP        ; Yes -> restart
                CALL    FIND_PROCESS            ; PID already used?
                CMP     BX, NO_PROCESS
                JNE     SHORT NP_CP_LOOP        ; Yes -> next PID
                POP     BX
                RET

NP_CP_WRAP:     MOV     NEXT_PID, MIN_PID
                JMP     SHORT NP_CP_LOOP
CREATE_PID      ENDP


;
; Initialize process table entry
;
                TALIGN  2
                ASSUME  SI:PTR NEW_PROC
                ASSUME  DI:PTR PROCESS
NP_SET_TAB      PROC    NEAR
                MOV     [DI].P_PID, EAX         ; Set PID
                MOV     [DI].P_PIDX, DL         ; Set process index
                DEC     DL                      ; Process number (0..)
                AND     EDX, 0FFH
                MOV     [DI].P_NUMBER, EDX
                SHL     DX, 3                   ; 8 bytes per GDT entry
                ADD     DX, G_LDT_SEL           ; LDT selector
                MOV     [DI].P_LDT, DX          ; Save LDT selector
                MOV     [DI].P_EXEC_HANDLE, NO_FILE_HANDLE
                MOV     [DI].P_PAGE_FAULTS, 0   ; No page faults yet
                MOV     [DI].P_FLAGS, 0         ; No flags set (see L_OPTIONS)
                MOV     [DI].P_TRUNC, 0         ; Set -t- (see L_OPTIONS)
                MOV     [DI].P_UFLAGS, 0        ; No user flags set
                MOV     [DI].P_STATUS, PS_INIT  ; Process not running yet
                MOV     [DI].P_HW_ACCESS, 0     ; See L_OPTIONS
                MOV     [DI].P_DRIVE, 0         ; No default drive (-r option)
                MOV     [DI].P_COMMIT_SIZE, 0   ; No add. pages (-C option)
                MOV     [DI].P_STACK_SIZE, 00800000H ; (default; see -s option)
                CMP     BYTE PTR [SI].NP_MODE1, NP_DEBUG ; Debugging?
                JNE     SHORT NP_ST_1           ; No -> skip
                MOV     [DI].P_STATUS, PS_STOP  ; Process `stopped'
                MOV     [DI].P_SIG_NO, SIGTRAP  ; by SIGTRAP
                OR      [DI].P_FLAGS, PF_DEBUG OR PF_WAIT_WAIT
                OR      [DI].P_HW_ACCESS, HW_ACCESS_CODE ; Set -ac option
NP_ST_1:        MOV     BX, [SI].NP_FPROC       ; Process which owns the files
                ASSUME  BX:PTR PROCESS
                MOV     AX, [BX].P_UMASK        ; Copy umask of that process
                MOV     [DI].P_UMASK, AX        ; to new process
                MOV     AX, [BX].P_UMASK1       ; Copy umask of that process
                MOV     [DI].P_UMASK1, AX       ; to new process
                MOV     BX, [SI].NP_PARENT      ; Parent process
                MOV     EAX, [BX].P_PID         ; Copy ID of parent to
                MOV     [DI].P_PPID, EAX        ; parent ID of new process
                MOV     [DI].P_SYMBOLS, 0       ; No symbols (yet)
                MOV     [DI].P_VESAINFO_PTR, 0  ; No VESA info data area
                MOV     [DI].P_PRF_SCALE, 0     ; Profiling not active
                RET
                ASSUME  BX:NOTHING
                ASSUME  SI:NOTHING
                ASSUME  DI:NOTHING
NP_SET_TAB      ENDP

;
; Create segments
;
                ASSUME  DI:PTR PROCESS
NP_SEGMENTS     PROC    NEAR
                LEA     SI, GDT + (G_SV_DATA_SEL AND NOT 07H)
                CALL    GET_BASE                ; SV_DATA base address -> EBX

                MOV     DX, [DI].P_LDT          ; Get LDT selector
                MOV     SI, DX
                LEA     SI, GDT[SI]             ; SI -> GDT

                SUB     DX, G_LDT_SEL
                IMUL    DX, LDT_ENTRIES
                ADD     DX, OFFSET SV_DATA:LDTS
                MOV     [DI].P_LDT_PTR, DX
                MOVZX   EDX, DX
                ADD     EBX, EDX                ; EBX -> LDT (linear)
                MOV     AX, A_LDT OR DPL_0
                MOV     ECX, 8*LDT_ENTRIES
                CALL    CREATE_SEG              ; LDT entry in GDT
                MOV     SI, DX                  ; SI -> LDT (SV_CODE)
                CALL    NULL_SEG                ; null
                ADD     SI, 8
                CALL    NULL_SEG                ; code
                ADD     SI, 8
                CALL    NULL_SEG                ; data
                ADD     SI, 8
                MOV     EBX, VIDEO_LIN
                MOV     ECX, 4000-1
                MOV     AX, A_DATA32 OR DPL_3
                CALL    CREATE_SEG              ; Video segment
                ADD     SI, 8
                CALL    NULL_SEG                ; Symbols and strings
                ADD     SI, 8
                CALL    NULL_SEG                ; Client process
                RET
                ASSUME  DI:NOTHING
NP_SEGMENTS     ENDP


;
; Create stdin/stdout/stderr handles, inherit open files
;
                ASSUME  DI:PTR PROCESS
NP_FILES        PROC    NEAR
                MOV     SI, NP_ARGS
                ASSUME  SI:PTR NEW_PROC
                MOV     SI, [SI].NP_FPROC       ; Process which owns the files
                ASSUME  SI:PTR PROCESS
                CALL    INHERIT_HANDLES
                RET
                ASSUME  SI:NOTHING
                ASSUME  DI:NOTHING
NP_FILES        ENDP


;
; Setup signal handling
;
                ASSUME  DI:PTR PROCESS
NP_SIGNALS      PROC    NEAR
                MOV     SI, NP_ARGS
                ASSUME  SI:PTR NEW_PROC
                MOV     SI, [SI].NP_FPROC       ; Parent process
                ASSUME  SI:PTR PROCESS
;
; Inherit signal handlers from parent process.  SIG_IGN is inherited
; as SIG_IGN, everything else as SIG_DFL.
;
                MOV     CX, SIGNALS
                MOV     BX, 0
NP_SIG1:        MOV     EAX, [SI].P_SIG_HANDLERS[BX]
                CMP     EAX, SIG_IGN
                JE      SHORT NP_SIG2
                MOV     EAX, SIG_DFL            ; Default signal handler
NP_SIG2:        MOV     [DI].P_SIG_HANDLERS[BX], EAX
                MOV     [DI].P_SA_MASK[BX], 0
                MOV     [DI].P_SA_FLAGS[BX], SA_ACK
                ADD     BX, 4
                LOOP    NP_SIG1
                MOV     [DI].P_SIG_PENDING, 0   ; No signals pending
                MOV     [DI].P_SIG_BLOCKED, 0   ; No signals blocked
                RET
                ASSUME  DI:NOTHING
NP_SIGNALS      ENDP


;
; Construct initial stack for C runtime startup code
; containing environment and arguments
;
                ASSUME  DI:PTR PROCESS
NP_STACK        PROC    NEAR
;
; Make the stack segment of the process accessible
;
                PUSH    SI
                MOV     SI, [DI].P_LDT
                MOV     AX, L_DATA_SEL
                CALL    GET_DESC
                MOV_ES_DS
                PUSH    DI
                LEA     DI, G_TMP2_DESC
                CLD
                MOVS    DWORD PTR ES:[DI], DWORD PTR DS:[SI]
                MOVS    DWORD PTR ES:[DI], DWORD PTR DS:[SI]
                POP     DI
                POP     SI
;
; End ->
;        Argument strings
;        Environment
;        argv[]
; ESP -> envp[]
;
                MOV     SI, NP_ARGS
                ASSUME  SI:PTR NEW_PROC
                MOVZX   EAX, [SI].NP_ENV_COUNT  ; Get number of env strings
                LEA     EAX, [4*EAX+4]          ; sizeof (envp[])
                MOV     ENVP_SIZE, EAX
                MOVZX   EAX, [SI].NP_ARG_COUNT  ; Get argc
                LEA     EAX, [4*EAX+4]          ; sizeof (argv[])
                MOV     ARGV_SIZE, EAX

                MOV     EBX, [DI].P_STACK_ADDR
                MOVZX   EAX, [SI].NP_ARG_SIZE   ; Size of arguments
                SUB     EBX, EAX                ; Argument strings
                MOVZX   EAX, [SI].NP_ENV_SIZE   ; Size of environment
                SUB     EBX, EAX                ; Environment strings
                SUB     EBX, ARGV_SIZE          ; argv[]
                SUB     EBX, ENVP_SIZE          ; envp[]
                AND     EBX, NOT 3              ; Alignment
                MOV     [DI].P_ESP, EBX         ; Initial stack pointer
                ASSUME  DI:NOTHING
                PUSH    DI                      ; Save DI
                MOV     EDI, EBX                ; Destination address
                ADD     EDI, ENVP_SIZE
                ADD     EDI, ARGV_SIZE
                CLD
                MOV     CX, [SI].NP_ENV_COUNT
                MOV     EAX, [SI].NP_ENV_OFF
                MOV     DX, [SI].NP_ENV_SEL
                MOV     EBP, 0                  ; No extra bytes
                CALL    PCOPY
                XOR     AL, AL
                STOS    BYTE PTR ES:[EDI]
                MOV     CX, [SI].NP_ARG_COUNT
                MOV     EAX, [SI].NP_ARG_OFF
                MOV     DX, [SI].NP_ARG_SEL
                MOV     EBP, 1                  ; One extra byte
                PUSH    EBX                     ; Save EBX for PQUOTE
                CALL    PCOPY
                POP     EBX
;
; If the -q option is set for the parent process, quote all arguments
;
                TEST    [SI].NP_MODE2, NP2_QUOTE
                JNZ     SHORT QUOTE_YES
                CMP     PROCESS_PTR, NO_PROCESS
                JE      SHORT QUOTE_NO
                PUSH    BX
                MOV     BX, PROCESS_PTR
                TEST    (PROCESS PTR [BX]).P_FLAGS, PF_QUOTE
                POP     BX
                JNZ     SHORT QUOTE_YES
QUOTE_NO:       MOV     AL, 00H
                JMP     SHORT QUOTE_1
QUOTE_YES:      MOV     AL, 01H
QUOTE_1:        CALL    PQUOTE                  ; In: EBX
;
; Remove temporary GDT entries for security
;
                MOV     AX, G_NULL_SEL
                MOV     ES, AX
                PUSH    SI
                LEA     SI, G_TMP1_DESC
                CALL    NULL_SEG
                LEA     SI, G_TMP2_DESC
                CALL    NULL_SEG
                POP     SI
                POP     DI                      ; Restore DI
                RET
                ASSUME  SI:NOTHING
NP_STACK        ENDP


;
; Set initial values of registers
;
; ESP is set in NP_STACK.
;
                ASSUME  DI:PTR PROCESS
NP_REGS         PROC    NEAR
                MOV     [DI].P_CS, L_CODE_SEL
                MOV     AX, L_DATA_SEL
                MOV     [DI].P_DS, AX
                MOV     [DI].P_ES, AX
                MOV     [DI].P_FS, AX
                MOV     [DI].P_GS, AX
                MOV     [DI].P_SS, AX
                MOV     EAX, [DI].P_ENTRY_POINT
                MOV     [DI].P_EIP, EAX
                MOV     [DI].P_EFLAGS, 0202H    ; IOPL=0, IF=1
                RET
                ASSUME  DI:NOTHING
NP_REGS         ENDP


;
; Copy arguments / environment
;
; In:   CX      Number of strings
;       EAX     Offset of strings
;       DX      Segment of strings
;       EBP     Add this to the table entry (skipping leading bytes)
;       EDI     Destination address for strings
;       EBX     Destination address for table
;
PCOPY           PROC    NEAR
                PUSH    ESI
                PUSH    DS                      ; Save DS & ESI
                PUSH    EAX
                .386P
                SLDT    SI
                .386
                MOV     AX, DX
                CALL    GET_DESC
                MOV_ES_DS
                PUSH    DI
                LEA     DI, G_TMP1_DESC
                CLD
                MOVS    DWORD PTR ES:[DI], DWORD PTR DS:[SI]
                MOVS    DWORD PTR ES:[DI], DWORD PTR DS:[SI]
                POP     DI
                POP     ESI
                MOV     AX, G_TMP1_SEL
                MOV     DS, AX
                ASSUME  DS:NOTHING
                MOV     AX, G_TMP2_SEL
                MOV     ES, AX
                JCXZ    PCOPY3
PCOPY1:         MOV     ES:[EBX], EDI           ; Put pointer
                ADD     DWORD PTR ES:[EBX], EBP ; Skip flag byte
                ADD     EBX, 4
PCOPY2:         LODS    BYTE PTR DS:[ESI]
                STOS    BYTE PTR ES:[EDI]
                OR      AL, AL
                JNZ     SHORT PCOPY2
                LOOP    PCOPY1
PCOPY3:         XOR     EAX, EAX
                MOV     ES:[EBX], EAX
                ADD     EBX, 4
                POP     DS
                ASSUME  DS:SV_DATA
                POP     ESI
                RET
PCOPY           ENDP


;
; Quote or unquote all arguments in the child process' stack.
;
; In:   EBX  Pointer to argv[] (in segment G_TMP2_SEL)
;       AL   00H (unquote) or 01H (quote)
;
PQUOTE          PROC    NEAR
                PUSH    EDI
                MOV     DI, G_TMP2_SEL
                MOV     ES, DI
PQ_LOOP:        MOV     EDI, ES:[EBX]
                TEST    EDI, EDI
                JZ      SHORT PQ_RET
                AND     BYTE PTR ES:[EDI-1], NOT 01H
                OR      BYTE PTR ES:[EDI-1], AL
                ADD     EBX, 4
                JMP     SHORT PQ_LOOP
PQ_RET:         POP     EDI
                RET
PQUOTE          ENDP

;
; Find process table entry for given process ID
;
; In:   EAX     Process ID
;
; Out:  BX      Pointer to process table entry (NO_PROCESS: not found)
;
                ASSUME  DS:SV_DATA
FIND_PROCESS    PROC    NEAR
                PUSH    ECX
                LEA     BX, PROCESS_TABLE
                ASSUME  BX:PTR PROCESS
                MOV     CX, MAX_PROCESSES
FP1:            CMP     EAX, [BX].P_PID
                JE      SHORT FP2
                ADD     BX, SIZE PROCESS
                LOOP    FP1
                MOV     BX, NO_PROCESS
FP2:            POP     ECX
                RET
                ASSUME  BX:NOTHING
FIND_PROCESS    ENDP


;
; Turn process into defunct process (zombie)
;
; In:   BX      Pointer to process table entry
;
                ASSUME  DS:SV_DATA
                ASSUME  BX:PTR PROCESS
ZOMBIE_PROCESS  PROC    NEAR
                CALL    FREE_PROCESS
                MOV     [BX].P_STATUS, PS_DEFUNCT
                OR      [BX].P_FLAGS, PF_WAIT_WAIT
                RET
                ASSUME  BX:NOTHING
ZOMBIE_PROCESS  ENDP


;
; Remove (kill) process
;
; In:   BX      Pointer to process table entry
;
                ASSUME  DS:SV_DATA
                ASSUME  BX:PTR PROCESS
REMOVE_PROCESS  PROC    NEAR
                CALL    FREE_PROCESS
                MOV     [BX].P_PID, NO_PID
                MOV     [BX].P_STATUS, PS_NONE
                RET
                ASSUME  BX:NOTHING
REMOVE_PROCESS  ENDP


;
; Remove last vestiges of a defunct process (zombie)
;
; In:   BX      Pointer to process table entry
;
; Note: Never call this without calling ZOMBIE_PROCESS first!
;
                ASSUME  DS:SV_DATA
                ASSUME  BX:PTR PROCESS
KILL_PROCESS    PROC    NEAR
                MOV     [BX].P_PID, NO_PID      ; Remove last vestiges
                MOV     [BX].P_STATUS, PS_NONE  ; of process
                RET
                ASSUME  BX:NOTHING
KILL_PROCESS    ENDP

;
; Free all resources used by a process (but keep the process table entry)
;
; In:   BX      Pointer to process table entry
;
                ASSUME  DS:SV_DATA
                ASSUME  BX:PTR PROCESS
FREE_PROCESS    PROC    NEAR
                PUSH    AX
                PUSH    CX
                PUSH    SI
;
; Unassign the FPU
;
                CMP     PROCESS_FP, BX          ; FPU assigned to this process?
                JNE     SHORT FP_FPU_1          ; No  -> skip
                MOV     PROCESS_FP, NO_PROCESS  ; Don't store FPU context
FP_FPU_1:
;
; Turn off profiling
;
                XOR     EAX, EAX
                XCHG    EAX, [BX].P_PRF_SCALE
                TEST    EAX, EAX
                JZ      SHORT FP_PROFIL_1
                CALL    PROFIL_STOP
FP_PROFIL_1:
;
; Reclaim timers
;
                LEA     SI, TIMER_TABLE
                ASSUME  SI:PTR TIMER
                MOV     CX, MAX_TIMERS
FP_TIMER_LOOP:  CMP     [SI].T_PROCESS, BX
                JNE     SHORT FP_TIMER_NEXT
                MOV     [SI].T_TYPE, TT_NONE
                MOV     [SI].T_PROCESS, NO_PROCESS
FP_TIMER_NEXT:  ADD     SI, SIZE TIMER
                LOOP    FP_TIMER_LOOP
                ASSUME  SI:NOTHING
;
; Reclaim memory
;
                MOV     AL, [BX].P_PIDX         ; Get process index
                CALL    FREE_LIN                ; Free linear address space
                CALL    FREE_PAGES              ; Adjust page table entries
;
; Close all files
;
                CALL    CLOSE_HANDLES
;
; Close exec file
;
                MOV     AX, NO_FILE_HANDLE
                XCHG    AX, [BX].P_EXEC_HANDLE
                CMP     AX, NO_FILE_HANDLE
                JE      SHORT RP_1
                PUSH    BX
                MOV     BX, AX
                CALL    CLOSE
                POP     BX
RP_1:           POP     SI
                POP     CX
                POP     AX
                RET
                ASSUME  BX:NOTHING
FREE_PROCESS    ENDP


;
; Store the registers saved on the stack by an interrupt
; in the process table entry
;
; In:   SS:BP   Pointer to interrupt stack frame
;       DS:BX   Pointer to process table entry
;
; Note: Only for user (ring 3) processes (ESP, SS!)
;
                ASSUME  DS:SV_DATA
                ASSUME  BX:PTR PROCESS
SAVE_PROCESS    PROC    NEAR
                PUSH    DS
                PUSH    ES
                PUSH    SI
                PUSH    DI
                PUSH    CX
                LEA     DI, [BX].P_GS
                MOV_ES_DS
                MOV     SI, BP
                PUSH    SS
                POP     DS
                CLD
                MOV     CX, I_REG_DWORDS
                REP     MOVS DWORD PTR ES:[DI], DWORD PTR DS:[SI]
                POP     CX
                POP     DI
                POP     SI
                POP     ES
                POP     DS
                RET
                ASSUME  BX:NOTHING
SAVE_PROCESS    ENDP


;
; Copy the registers from a process table entry to the interrupt
; stack frame, set TS bit of CR0, set spawnve() return value if required
;
; In:   SS:BP   Pointer to interrupt stack frame
;       DS:BX   Pointer to process table entry
;
; Out:  LDTR
;       CR0     TS set
;
; Note: Only for user (ring 3) processes (ESP, SS!)
;
                ASSUME  DS:SV_DATA
                ASSUME  BX:PTR PROCESS
                ASSUME  BP:PTR ISTACKFRAME
REST_PROCESS    PROC    NEAR
                PUSH    DS
                PUSH    ES
                PUSH    SI
                PUSH    DI
                PUSH    EAX
                PUSH    ECX
                LEA     SI, [BX].P_GS
                MOV     DI, BP
                PUSH    SS
                POP     ES
                CLD
                MOV     CX, I_REG_DWORDS
                REP     MOVS DWORD PTR ES:[DI], DWORD PTR DS:[SI]
                .386P
                LLDT    [BX].P_LDT
                MOV     ECX, CR0
                OR      ECX, CR0_TS             ; For coprocessor task switches
                MOV     CR0, ECX
                .386
                CMP     [BX].P_STATUS, PS_WAIT_SPAWN    ; In spawnve()?
                JNE     SHORT RP_RET
                MOV     EAX, [BX].P_SPAWN_RC    ; Return code or PID of child
                MOV     I_EAX, EAX              ; returned in EAX
                TEST    [BX].P_FLAGS, PF_PSEUDO_ASYNC   ; Pseudo async child?
                JZ      SHORT RP_RET            ; No  -> continue
                CMP     [BX].P_SIG_HANDLERS[4*SIGCLD], SIG_IGN
                JE      SHORT RP_RET
                CMP     [BX].P_SIG_HANDLERS[4*SIGCLD], SIG_DFL
                JE      SHORT RP_RET
                BTS     [BX].P_SIG_PENDING, SIGCLD      ; Generate SIGCLD
RP_RET:         MOV     [BX].P_STATUS, PS_RUN   ; Running again
                POP     ECX
                POP     EAX
                POP     DI
                POP     SI
                POP     ES
                POP     DS
                RET
                ASSUME  BX:NOTHING
                ASSUME  BP:NOTHING
REST_PROCESS    ENDP

;
; Wait for a child process
;
; In:   EAX     Parent process ID
;
; Out:  BX      Pointer to process table entry or:
;               NO_PROCESS: no child process found
;               NO_WPROCESS: no child process stopped/terminated
;
                ASSUME  DS:SV_DATA
WAIT_PROCESS    PROC    NEAR
                PUSH    ECX
                PUSH    DX
                MOV     DX, NO_PROCESS          ; Default return value
                LEA     BX, PROCESS_TABLE
                ASSUME  BX:PTR PROCESS
                MOV     CX, MAX_PROCESSES
WP_LOOP:        CMP     [BX].P_STATUS, PS_NONE  ; Empty slot?
                JE      SHORT WP_NEXT           ; Yes -> skip
                CMP     EAX, [BX].P_PPID        ; Child process?
                JNE     SHORT WP_NEXT           ; No  -> skip
                MOV     DX, NO_WPROCESS         ; Change default return value
                TEST    [BX].P_FLAGS, PF_WAIT_WAIT      ; stopped/terminated?
                JNZ     SHORT WP_RET            ; Yes -> found
WP_NEXT:        ADD     BX, SIZE PROCESS
                LOOP    WP_LOOP
                ASSUME  BX:NOTHING
                MOV     BX, DX
WP_RET:         POP     DX
                POP     ECX
                RET
WAIT_PROCESS    ENDP

;
; Suspend process
;
; In:  BX       Pointer to process table entry
;      EAX      Number of timer ticks
;
; Out: EAX      Number of timer ticks remaining
;
                ASSUME  BX:PTR PROCESS
SLEEP           PROC    NEAR
                PUSH    CLOCK_LO
                PUSH    EAX
                AND     [BX].P_FLAGS, NOT PF_SLEEP_FLAG
                MOV     DX, TT_SLEEP
                CALL    SET_TIMER
                JC      SHORT SLEEP_DONE
SLEEP_WAIT:     MOV     EAX, [BX].P_SIG_BLOCKED ; Compute set of pending,
                NOT     EAX                     ; unblocked signals
                AND     EAX, [BX].P_SIG_PENDING
                JNZ     SHORT SLEEP_DONE
                TEST    [BX].P_FLAGS, PF_SLEEP_FLAG
                JZ      SHORT SLEEP_WAIT
SLEEP_DONE:     POP     EAX
                POP     ECX
                SUB     ECX, CLOCK_LO
                NEG     ECX                     ; Elapsed time
                SUB     EAX, ECX
                JNC     SHORT SLEEP_RET
                XOR     EAX, EAX
SLEEP_RET:      RET
SLEEP           ENDP
                ASSUME  BX:NOTHING


;
; Set a timer. If there is already a timer of the given type, the timer
; value is set to the new value.
;
; In:   BX      Pointer to process table entry
;       DX      Timer type
;       EAX     Timer value (ticks), stop the timer if this is zero
;
; Out:  CY      Table overflow
;       EAX     Time remaining previously
;
                ASSUME  DS:SV_DATA
                TALIGN  2
SET_TIMER       PROC    NEAR
                PUSH    CX
                PUSH    SI
                LEA     SI, TIMER_TABLE
                ASSUME  SI:PTR TIMER
                MOV     CX, MAX_TIMERS
                PUSH    EAX
                CLI
                MOV     EAX, TIMER_TICKS
ST_ADJUST:      SUB     [SI].T_TICKS, EAX
                ADD     SI, SIZE TIMER
                LOOP    ST_ADJUST
                MOV     TIMER_TICKS, 0
                POP     EAX
                LEA     SI, TIMER_TABLE
                MOV     CX, MAX_TIMERS
ST_FIND_LOOP:   CMP     [SI].T_TYPE, DX
                JNE     SHORT ST_FIND_NEXT
                CMP     [SI].T_PROCESS, BX
                JE      SHORT ST_EXISTS
ST_FIND_NEXT:   ADD     SI, SIZE TIMER
                LOOP    ST_FIND_LOOP
                OR      EAX, EAX                ; Stop the timer?
                JZ      SHORT ST_OK_ZERO        ; Yes -> don't start it
ST_NEW:         LEA     SI, TIMER_TABLE
                MOV     CX, MAX_TIMERS
ST_NEW_LOOP:    CMP     [SI].T_TYPE, TT_RUNNING
                JB      SHORT ST_NEW_FOUND
                ADD     SI, SIZE TIMER
                LOOP    ST_NEW_LOOP
                STC
                JMP     SHORT ST_RET

ST_NEW_FOUND:   MOV     [SI].T_TICKS, EAX
                MOV     [SI].T_PROCESS, BX
                MOV     [SI].T_TYPE, DX
                CMP     TIMER_MIN, 0
                JE      SHORT ST_NEW_1
                CMP     EAX, TIMER_MIN
                JAE     SHORT ST_OK_ZERO
ST_NEW_1:       MOV     TIMER_MIN, EAX
ST_OK_ZERO:     XOR     EAX, EAX
                JMP     SHORT ST_OK

ST_CLEAR:       MOV     [SI].T_TYPE, TT_NONE
                MOV     EAX, [SI].T_TICKS
                JMP     SHORT ST_OK

ST_EXISTS:      OR      EAX, EAX
                JZ      SHORT ST_CLEAR
                PUSH    [SI].T_TICKS
                MOV     [SI].T_TICKS, EAX
                MOV     [SI].T_PROCESS, BX
                MOV     [SI].T_TYPE, DX
                CMP     TIMER_MIN, 0
                JE      SHORT ST_RESET_1
                CMP     EAX, TIMER_MIN
                JAE     SHORT ST_RESET_2
ST_RESET_1:     MOV     TIMER_MIN, EAX
ST_RESET_2:     POP     EAX
ST_OK:          CLC
ST_RET:         STI
                POP     SI
                POP     CX
                RET
                ASSUME  SI:NOTHING
SET_TIMER       ENDP

;
; Set the PAGE_ALLOC bit of EBX (for SET_PAGES) if PF_COMMIT is set.
;
; In:  EBX      Process table entry
;      DI       Pointer to process table entry
;
; Out: EBX      Process table entry
;
                ASSUME  DS:SV_DATA
                ASSUME  DI:PTR PROCESS
                TALIGN  2
SET_COMMIT      PROC    NEAR
                TEST    [DI].P_FLAGS, PF_COMMIT
                JZ      SHORT SC_RET
                OR      BX, PAGE_ALLOC
SC_RET:         RET
                ASSUME  DI:NOTHING
SET_COMMIT      ENDP

;
; Find an executable file
;
; In:  SI       Pointer to program name
;
; Out: DX       Pointer to path name
;
                ASSUME  DS:SV_DATA
FIND_EXEC       PROC    NEAR
;
; Use program name as is if it contains \, /, or :
;
                MOV     BX, SI
FE_1:           MOV     AL, [BX]
                INC     BX
                CMP     AL, "/"
                JE      SHORT FE_DEFAULT
                CMP     AL, "\"
                JE      SHORT FE_DEFAULT
                CMP     AL, ":"
                JE      SHORT FE_DEFAULT
                OR      AL, AL
                JNZ     SHORT FE_1
;
; Executable file in current directory?
;
                MOVZX   EDX, SI
                CALL    ACCESS
                JNC     SHORT FE_RET
;
; Search EMXPATH and PATH for executable file
;
                MOV     AX, G_ENV_SEL
                MOV     ES, AX
                LEA     BX, EXEC_FNAME          ; Buffer
                MOVZX   EDI, ENV_EMXPATH
                OR      EDI, EDI
                JZ      SHORT FE_2
                CALL    FIND_FILE
                JNC     SHORT FE_OK
FE_2:           MOVZX   EDI, ENV_PATH
                OR      EDI, EDI
                JZ      SHORT FE_DEFAULT
                CALL    FIND_FILE
                JNC     SHORT FE_OK
;
; Executable file not found (or name contains "\/:"); be naive
;
FE_DEFAULT:     MOVZX   EDX, SI
                JMP     SHORT FE_RET
;
; Executable file found in path
;
FE_OK:          XOR     EDX, EDX
                LEA     DX, EXEC_FNAME
FE_RET:         RET
FIND_EXEC       ENDP


;
; Try to load a floating point unit emulator
;
                ASSUME  DS:SV_DATA
FPUEMU_LOAD     PROC    NEAR
                MOV     FPUEMU_STATE, FES_LOADING
;
; Find the FPU emulator
;
                CMP     EMX_DIR[0], 0
                JE      SHORT USE_PATH
                LEA     DI, EXEC_FNAME
                LEA     SI, EMX_DIR
                CALL    NSTRCPY
                MOV     DI, AX
                LEA     SI, $FPUEMU_NAME
                CALL    NSTRCPY
                LEA     EDX, EXEC_FNAME
                CALL    ACCESS
                JNC     SHORT LOAD
USE_PATH:       LEA     SI, $FPUEMU_NAME
                CALL    FIND_EXEC
;
; Set up NP1 for loading the FPU emulator.  Don't touch the members
; related to the environment and to the parent process, they have been
; already initialized for the initial process and can be reused.
;
LOAD:           MOV     NP1.NP_FNAME_OFF, EDX
                MOV     NP1.NP_FNAME_SEL, DS
                MOV     NP1.NP_ARG_OFF, OFFSET $FPUEMU_ARG
                MOV     NP1.NP_ARG_SEL, DS
                MOV     NP1.NP_ARG_COUNT, 0
                MOV     NP1.NP_ARG_SIZE, 0
                MOV     NP1.NP_MODE1, NP_SPAWN_SYNC
                MOV     NP1.NP_MODE2, 0
                LEA     SI, NP1                 ; Arguments
                CALL    NEW_PROCESS             ; Load the program
                OR      AX, AX                  ; Ok?
                JNZ     SHORT FAIL
;
; Put the interrupted process to sleep
;
                MOV     BX, PROCESS_PTR
                MOV     PROCESS_FPUCLIENT, BX
                MOV     (PROCESS PTR [BX]).P_STATUS, PS_FPUEMU_CLIENT
                CALL    SAVE_PROCESS
;
; Run the FPU emulator.  Don't set PROCESS_SIG as SIGINT should be sent
; to the client process instead of the FPU emulator.
;
                MOV     PROCESS_PTR, DI
                MOV     BX, DI
                CALL    REST_PROCESS
                CALL    BREAK_AFTER_IRET                ; TODO
                RET

FAIL:           MOV     FPUEMU_STATE, FES_NONE
                RET
FPUEMU_LOAD     ENDP

;
; Switch to the floating point unit emulator for emulating one or more
; floating point instructions
;
                ASSUME  DS:SV_DATA
                ASSUME  BP:PTR ISTACKFRAME
FPUEMU_CALL     PROC    NEAR
;
; Put the interrupted process to sleep
;
                MOV     DI, PROCESS_PTR
                ASSUME  DI:PTR PROCESS
                MOV     PROCESS_FPUCLIENT, DI
                MOV     [DI].P_STATUS, PS_FPUEMU_CLIENT
                MOV     BX, DI
                CALL    SAVE_PROCESS
;
; Set up segment and FS register for the emulator
;
                MOV     BX, PROCESS_FPUEMU
                ASSUME  BX:PTR PROCESS
                MOV     SI, [DI].P_LDT_PTR
                ADD     SI, L_DATA_SEL AND NOT 07H
                MOV     EAX, [SI+0]
                MOV     EDX, [SI+4]
                MOV     SI, [BX].P_LDT_PTR
                ADD     SI, L_CLIENT_SEL AND NOT 07H
                MOV     [SI+0], EAX
                MOV     [SI+4], EDX
                MOV     [BX].P_FS, L_CLIENT_SEL
;
; Wake up the emulator
;
                MOV     PROCESS_PTR, BX
                CALL    REST_PROCESS
;;;;            CALL    BREAK_AFTER_IRET
;
; Fill-in the communication area
;
                MOV     EAX, [DI].P_NUMBER
                ASSUME  DI:NOTHING
                MOV     ES, I_DS
                MOV     EDI, I_EDX
                ASSUME  EDI:NEAR32 PTR FPUEMU_COM
                MOV     ES:[EDI].FEC_PNUM, EAX
                MOV     ES:[EDI].FEC_NOTIFY, FPUN_EMU
                MOV     ES:[EDI].FEC_SIGNAL, 0
                LEA     EDI, [EDI].FEC_FRAME
                MOVZX   ESI, PROCESS_FPUCLIENT
                LEA     SI, (PROCESS PTR [SI]).P_GS
                MOV     ECX, I_REG_DWORDS
                REP     MOVS DWORD PTR ES:[EDI], DWORD PTR DS:[ESI]
                MOV     I_EAX, 0                ; Return 0 from __fpuemu()
                MOV     I_ECX, 0
                RET
                ASSUME  BP:NOTHING
                ASSUME  BX:NOTHING
                ASSUME  EDI:NOTHING
FPUEMU_CALL     ENDP


;
; A process has terminated, a different process is about to be run
;
; In:  EAX      P_NUMBER of the terminated process
;      BP       Pointer to stack frame
;
                ASSUME  DS:SV_DATA
                ASSUME  BP:PTR ISTACKFRAME
FPUEMU_ENDPROC  PROC    NEAR
                CMP     FPUEMU_STATE, FES_ON
                JNE     SHORT FIN
                MOV     FPUEMU_STATE, FES_ENDPROC
                PUSH    EAX
                MOV     BX, PROCESS_PTR         ; Save the incoming process
                MOV     PROCESS_FPUCLIENT, BX
                MOV     (PROCESS PTR [BX]).P_STATUS, PS_FPUEMU_CLIENT
                CALL    SAVE_PROCESS
                MOV     BX, PROCESS_FPUEMU      ; Restore the FPU emulator
                MOV     PROCESS_PTR, BX
                CALL    REST_PROCESS
                MOV     ES, I_DS
                MOV     EDI, I_EDX
                ASSUME  EDI:NEAR32 PTR FPUEMU_COM
                POP     ES:[EDI].FEC_PNUM
                MOV     ES:[EDI].FEC_NOTIFY, FPUN_ENDPROC
                MOV     ES:[EDI].FEC_SIGNAL, 0
                MOV     I_EAX, 0
                MOV     I_ECX, 0
FIN:            RET
FPUEMU_ENDPROC  ENDP


;
; A new process is about to start
;
; In:  EAX      P_NUMBER of the terminated process
;      BP       Pointer to stack frame
;
                ASSUME  DS:SV_DATA
                ASSUME  BP:PTR ISTACKFRAME
FPUEMU_NEWPROC  PROC    NEAR
                CMP     FPUEMU_STATE, FES_ON
                JNE     SHORT FIN
                MOV     FPUEMU_STATE, FES_NEWPROC
                PUSH    EAX
                MOV     BX, PROCESS_PTR         ; Save the incoming process
                MOV     PROCESS_FPUCLIENT, BX
                MOV     (PROCESS PTR [BX]).P_STATUS, PS_FPUEMU_CLIENT
                CALL    SAVE_PROCESS
                MOV     BX, PROCESS_FPUEMU      ; Restore the FPU emulator
                MOV     PROCESS_PTR, BX
                CALL    REST_PROCESS
                MOV     ES, I_DS
                MOV     EDI, I_EDX
                ASSUME  EDI:NEAR32 PTR FPUEMU_COM
                POP     ES:[EDI].FEC_PNUM
                MOV     ES:[EDI].FEC_NOTIFY, FPUN_NEWPROC
                MOV     ES:[EDI].FEC_SIGNAL, 0
                MOV     I_EAX, 0
                MOV     I_ECX, 0
FIN:            RET
FPUEMU_NEWPROC  ENDP


SV_CODE         ENDS

;
; Real-mode code
;
INIT_CODE       SEGMENT

                ASSUME  CS:INIT_CODE, DS:NOTHING


                ASSUME  DS:SV_DATA

PROCESS_INIT    PROC    NEAR
                LEA     BX, PROCESS_TABLE
                ASSUME  BX:PTR PROCESS
                MOV     CX, MAX_PROCESSES
PI_LOOP_PROC:   MOV     [BX].P_PID, NO_PID
                MOV     [BX].P_STATUS, PS_NONE
                MOV     [BX].P_EXEC_HANDLE, NO_FILE_HANDLE
                MOV     SI, 0
PI_LOOP_HANDLE: MOV     [BX].P_HANDLES[SI], NO_FILE_HANDLE
                MOV     [BX].P_HFLAGS[SI], 0
                ADD     SI, 2
                CMP     SI, 2 * MAX_FILES
                JB      SHORT PI_LOOP_HANDLE
                ADD     BX, SIZE PROCESS
                LOOP    PI_LOOP_PROC
                ASSUME  BX:NOTHING
                MOV     PROCESS_PTR, NO_PROCESS
                MOV     PROCESS_SIG, NO_PROCESS
                MOV     PROCESS_FP, NO_PROCESS  ; Noone is using the FPU
                MOV     PROCESS_FPUEMU, NO_PROCESS ; No emulator loaded
;
; Initialize dummy parent process.  INIT_FILEIO has already initialized
; the P_HANDLES and P_HFLAGS arrays of PROC0.
;
                MOV     PROC0.P_PID, 0
                MOV     PROC0.P_STATUS, PS_INIT
                MOV     PROC0.P_FLAGS, 0
                MOV     PROC0.P_TRUNC, 0
                MOV     PROC0.P_UMASK, 022Q
                MOV     PROC0.P_UMASK1, 644Q
                MOV     PROC0.P_PRF_SCALE, 0

                MOV     CX, SIGNALS
                MOV     BX, 0
PI_LOOP_SIG:    MOV     PROC0.P_SIG_HANDLERS[BX], SIG_DFL
                MOV     PROC0.P_SA_MASK[BX], 0
                MOV     PROC0.P_SA_FLAGS[BX], SA_ACK
                ADD     BX, 4
                LOOP    PI_LOOP_SIG

                MOV     PROC0.P_SIG_PENDING, 0
                MOV     PROC0.P_SIG_BLOCKED, NOT 0 ; No signals allowed
                MOV     PROC0.P_EXEC_HANDLE, NO_FILE_HANDLE
                RET
PROCESS_INIT    ENDP

INIT_CODE       ENDS

                END
