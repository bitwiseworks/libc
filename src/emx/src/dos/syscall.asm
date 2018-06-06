;
; SYSCALL.ASM -- System calls (without BIOS and DOS calls)
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

                INCLUDE EMX.INC
                INCLUDE SIGNAL.INC
                INCLUDE PROCESS.INC
                INCLUDE PAGING.INC
                INCLUDE SEGMENTS.INC
                INCLUDE LOADER.INC
                INCLUDE FILEIO.INC
                INCLUDE TABLES.INC
                INCLUDE PMINT.INC
                INCLUDE SWAPPER.INC
                INCLUDE MEMORY.INC
                INCLUDE VCPI.INC
                INCLUDE MISC.INC
                INCLUDE XMS.INC
                INCLUDE CORE.INC
                INCLUDE PROFIL.INC
                INCLUDE VPRINT.INC
                INCLUDE UTILS.INC
                INCLUDE VERSION.INC
                INCLUDE TERMIO.INC
                INCLUDE STAT.INC
                INCLUDE TIMEB.INC
                INCLUDE ULIMIT.INC
                INCLUDE ERRORS.INC

                PUBLIC  DO_SYSCALL

FD_SETSIZE      =       256

MY_DATETIME     STRUCT
SECONDS         DD      ?
MINUTES         DD      ?
HOURS           DD      ?
DAY             DD      ?
MONTH           DD      ?
YEAR            DD      ?
MY_DATETIME     ENDS

SELECT_ARGS     STRUCT
SELA_NFDS       DD      ?
SELA_READFDS    DD      ?
SELA_WRITEFDS   DD      ?
SELA_EXCEPTFDS  DD      ?
SELA_TIMEOUT    DD      ?
SELECT_ARGS     ENDS

TIMEVAL         STRUCT
TV_SEC          DD      ?
TV_USEC         DD      ?
TIMEVAL         ENDS

SV_DATA         SEGMENT

;
; This structure is passed by spawnve() to NEW_PROCESS
;
                TALIGN  2
NP2             NEW_PROC        <>

;
; Variables for __select()
;
SEL_COUNT       WORD    ?
SEL_RBITS       BYTE    FD_SETSIZE/8 DUP (?)

;
;
;
INO_NUMBER      DWORD   100000H

;
; Number of days of a month
;
MONTH_LEN       DB      31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31

$FAT            DB      "FAT", 0
$LAN            DB      "LAN", 0

;
; __open() maps "/dev/null" and "/dev/tty" to "nul" and "con",
; respectively.
;
$DEVNULL        BYTE    "/dev/null", 0
$DEVTTY         BYTE    "/dev/tty", 0
$NUL            BYTE    "nul", 0
$CON            BYTE    "con", 0

;
; Special program names
;
$COMMAND        BYTE    "COMMAND.COM", 0
$4DOS           BYTE    "4DOS.COM", 0

;
; DOS_COMMAND sets this flag to non-FALSE if command.com or 4dos.com
; is to be run.
;
IS_COMMAND      BYTE    FALSE

;
; Parameter block for DOS function 4BH
;
PAR_BLOCK       LABEL   BYTE
PB_ENV          DW      0
                DD      CMD_LINE                ; Command line
                DD      FCB                     ; FCB1
                DD      FCB                     ; FCB2

;
; He who uses FCBs loses
;
FCB             DB      0                       ; Drive name
                DB      11 DUP (" ")            ; File name
                DB      20 DUP (0)              ; Fill to 32 bytes

;
; Program name and command line for DOS programs
;
PGM_NAME_SIZE   =       65
CMD_LINE_SIZE   =       128

PGM_NAME        DB      PGM_NAME_SIZE DUP (0)
CMD_LINE        DB      CMD_LINE_SIZE DUP (0)

FIND_BUF        STRUC
FIND_RESERVED   DB      15H DUP (?)
FIND_ATTR       DB      ?
FIND_TIME       DW      ?
FIND_DATE       DW      ?
FIND_SIZE       DD      ?
FIND_NAME       DB      13 DUP (?)
FIND_BUF        ENDS

STAT_CWD        DB      "x:\"
                DB      65 DUP (?)

;
; Error message
;
$SYSCALL        DB      "Invalid syscall function code: 7f", 0

SV_DATA         ENDS

SV_CODE         SEGMENT

                ASSUME  CS:SV_CODE, DS:NOTHING

TIME_2_UNIX     PROTO   PASCAL, MDT:PTR MY_DATETIME
                EXTRN   PTRACE:NEAR

;
; Jump table for syscall functions
;
                TALIGN  2
SYSCALL_JMP     DW      SYS00                   ; __sbrk
                DW      SYS01                   ; __brk
                DW      SYS02                   ; __ulimit
                DW      SYS03                   ; __vmstat
                DW      SYS04                   ; __umask1
                DW      SYS05                   ; __getpid
                DW      SYS06                   ; __spawnve
                DW      SYSCALL_ERROR           ; (obsolete)
                DW      PTRACE                  ; __ptrace
                DW      SYS09                   ; __wait
                DW      SYS0A                   ; __version
                DW      SYS0B                   ; __memavail
                DW      SYS0C                   ; __signal
                DW      SYS0D                   ; __kill
                DW      SYS0E                   ; __raise
                DW      SYS0F                   ; __uflags
                DW      SYS10                   ; __unwind
                DW      SYS11                   ; __core
                DW      SYS12                   ; __portaccess
                DW      SYS13                   ; __memaccess
                DW      SYS14                   ; __ioctl2
                DW      SYS15                   ; __alarm
                DW      SYS16                   ; Poll the keyboard
                DW      SYS17                   ; __sleep
                DW      SYS18                   ; __chsize
                DW      SYS19                   ; __fcntl
                DW      SYS1A                   ; __pipe
                DW      SYS1B                   ; __fsync
                DW      SYS1C                   ; __fork
                DW      SYS1D                   ; __scrsize
                DW      SYS1E                   ; __select
                DW      SYS1F                   ; __syserrno
                DW      SYS20                   ; __stat
                DW      SYS21                   ; __fstat
                DW      SYSCALL_ERROR           ; No longer used
                DW      SYS23                   ; __filesys
                DW      SYS24                   ; __utimes
                DW      SYS25                   ; __ftruncate
                DW      SYS26                   ; __clock
                DW      SYS27                   ; __ftime
                DW      SYS28                   ; __umask
                DW      SYS29                   ; __getppid
                DW      SYS2A                   ; __nls_memupr
                DW      SYS2B                   ; __open
                DW      SYS2C                   ; __newthread
                DW      SYS2D                   ; __endthread
                DW      SYS2E                   ; __waitpid
                DW      SYS2F                   ; __read_kbd
                DW      SYS30                   ; __sleep2
                DW      SYS31                   ; __unwind2
                DW      SYS32                   ; __pause
                DW      SYS33                   ; __execname
                DW      SYS34                   ; __initthread
                DW      SYS35                   ; __sigaction
                DW      SYS36                   ; __sigpending
                DW      SYS37                   ; __sigprocmask
                DW      SYS38                   ; __sigsuspend
                DW      SYS39                   ; __imphandle
                DW      SYS3A                   ; __fpuemu
                DW      SYSCALL_ERROR           ; __getsockhandle
                DW      SYSCALL_ERROR           ; __socket
                DW      SYSCALL_ERROR           ; __bind
                DW      SYSCALL_ERROR           ; __listen
                DW      SYSCALL_ERROR           ; __recv
                DW      SYSCALL_ERROR           ; __send
                DW      SYSCALL_ERROR           ; __accept
                DW      SYSCALL_ERROR           ; __connect
                DW      SYSCALL_ERROR           ; __getsockopt
                DW      SYSCALL_ERROR           ; __setsockopt
                DW      SYSCALL_ERROR           ; __getsockname
                DW      SYSCALL_ERROR           ; __getpeername
                DW      SYSCALL_ERROR           ; __gethostbyname
                DW      SYSCALL_ERROR           ; __gethostbyaddr
                DW      SYSCALL_ERROR           ; __getservbyname
                DW      SYSCALL_ERROR           ; __getservbyport
                DW      SYSCALL_ERROR           ; __getprotobyname
                DW      SYSCALL_ERROR           ; __getprotobynumber
                DW      SYSCALL_ERROR           ; __getnetbyname
                DW      SYSCALL_ERROR           ; __getnetbyaddr
                DW      SYSCALL_ERROR           ; __gethostname
                DW      SYSCALL_ERROR           ; __gethostid
                DW      SYSCALL_ERROR           ; __shutdown
                DW      SYSCALL_ERROR           ; __recvfrom
                DW      SYSCALL_ERROR           ; __sendto
                DW      SYSCALL_ERROR           ; __impsockhandle
                DW      SYSCALL_ERROR           ; __recvmsg
                DW      SYSCALL_ERROR           ; __sendmsg
                DW      SYSCALL_ERROR           ; __ttyname
                DW      SYS58                   ; __settime
                DW      SYS59                   ; __profil
                DW      SYS5A                   ; __nls_ctype
                DW      SYSCALL_ENOSYS		; __setsyserrno
SYSCALL_LAST    =       ($-SYSCALL_JMP)/2-1

;
; Syscall functions
;
; In:   AH      7FH
;       AL      Function code
;
                ASSUME  DS:SV_DATA
                ASSUME  BP:PTR ISTACKFRAME
                TALIGN  4
DO_SYSCALL:     CMP     AL, SYSCALL_LAST        ; Valid function code?
                JA      SHORT SYSCALL_ENOSYS    ; Yes -> return ENOSYS
                MOV     DI, PROCESS_PTR         ; Process table entry
                MOVZX   EAX, AL                 ; Keep only lower 8 bits
                JMP     SYSCALL_JMP[EAX*2]      ; Jump!

SYSCALL_ERROR:  LEA     EDX, $SYSCALL           ; Display error message
                MOV     BL, AL                  ; (function code = AL)
                JMP     DOSF_ERROR              ; and abort

SYSCALL_ENOSYS: MOV     I_EAX, -1
                MOV     I_ECX, ENOSYS
                RET

; ----------------------------------------------------------------------------
; AX=7F00H: EAX:=sbrk(EDX)
; AX=7F01H: EAX:=brk(EDX)
;
; Change data segment space allocation
;
; ----------------------------------------------------------------------------
;
; AX=7F00H: EAX:=sbrk(EDX)
;
; In:   EDX     Value to be added to current break value (address of
;               first byte beyond the end of the data segment). Positive
;               values grow the data segment, negative values shrink the
;               data segment.
;
; Out:  EAX     Previous break value or -1 if error.
;
; ----------------------------------------------------------------------------
;
; AX=7F01H: EAX:=brk(EDX)
;
; In:   EDX     New break value.
;
; Out:  EAX     0 if successful or -1 if error.
;
; ----------------------------------------------------------------------------
;
; Bug: doesn't fail if there isn't enough swap space
; Bug: Doesn't free pages with negative argument
; Bug: Doesn't work with stack above data/bss!!!
; Bug?: Doesn't zero-fill new memory
;
; ----------------------------------------------------------------------------
                ASSUME  DI:PTR PROCESS
SYS00:          MOV     EAX, [DI].P_BRK         ; Get current break value
                PUSH    EAX                     ; Save it
                MOV     EDX, I_EDX              ; Get argument
                CMP     EDX, 0                  ; No change?
                JE      SHORT SYS00_OK          ; Yes -> done
                JS      SHORT SYS00_NEG
                ADD     EAX, EDX
                JMP     SHORT SYS00_1
SYS00_NEG:      NEG     EDX
                SUB     EAX, EDX
SYS00_1:        JC      SHORT SYS00_ERR
                CALL    BRK
                JNC     SHORT SYS00_OK
SYS00_ERR:      POP     EAX                     ; Throw away saved value
                MOV     I_EAX, -1               ; Return -1
                RET

SYS00_OK:       POP     I_EAX                   ; Return previous break value
                RET

                ASSUME  DI:NOTHING

SYS01:          MOV     EAX, I_EDX              ; Argument
                CALL    BRK
                SBB     EAX, EAX                ; ok -> 0, error -> -1
                MOV     I_EAX, EAX
                RET


;
; Common code for brk and sbrk
;
; In:   DS:DI   Pointer to process table entry
;       EAX     New break value
;
; Out:  CY      Error
;
                ASSUME  DI:PTR PROCESS
BRK             PROC    NEAR
                CMP     EAX, [DI].P_INIT_BRK
                JB      BRK_ERR
                MOV     EBX, [DI].P_STACK_ADDR
                CMP     EBX, [DI].P_BRK         ; Stack above data/bss ?
                JB      SHORT BRK_1             ; No  -> don't check stack
                SUB     EBX, 4096               ; Leave at least one page
                CMP     EAX, EBX                ; of stack space (note 1)
                JA      BRK_ERR                 ; Out of memory -> failure
BRK_1:          MOV     ESI, EAX                ; ESI := new break value
                CMP     EAX, [DI].P_BRK
                JB      SHORT BRK_SEG           ; Set segment size
                MOV     ECX, EAX
                DEC     ECX                     ; New last byte
                SHR     ECX, 12                 ; New last page number
                MOV     EAX, [DI].P_BRK
                DEC     EAX
                JS      SHORT BRK_2
                SHR     EAX, 12                 ; Old last page number
BRK_2:          SUB     ECX, EAX                ; Number of new pages
                JBE     SHORT BRK_SEG
                INC     EAX                     ; First new page number
                SHL     EAX, 12                 ; Byte address of new page
                ADD     EAX, [DI].P_LINEAR      ; Linear address
;
; Note 1: We must not change the attributes of stack pages!
;         This is assured by leaving at least 4096 bytes between data
;         and stack.
;
                CALL    INIT_PAGES
                JC      SHORT BRK_ERR           ; Out of memory
                MOV     EBX, PAGE_WRITE OR PAGE_USER
                MOV     EDX, SRC_NONE
                TEST    [DI].P_FLAGS, PF_DONT_ZERO
                JNZ     SHORT BRK_3
                MOV     EDX, SRC_ZERO
BRK_3:          MOV     DL, [DI].P_PIDX
                CALL    SET_COMMIT              ; Set PAGE_ALLOC bit
                CALL    SET_PAGES
                JC      SHORT BRK_ERR           ; Committing failed
;
; Adjust data segment size
;
BRK_SEG:        MOV     EBX, [DI].P_STACK_ADDR
                CMP     EBX, [DI].P_BRK         ; Stack above data/bss ?
                JA      BRK_DONE                ; Yes -> skip
                MOV     ECX, ESI
                PUSH    ESI
                MOV     SI, [DI].P_LDT_PTR
                ADD     SI, L_DATA_SEL AND NOT 07H
                CALL    SEG_SIZE
                POP     ESI
BRK_DONE:       MOV     [DI].P_BRK, ESI
                CLC
                RET

BRK_ERR:        STC
                RET

                ASSUME  DI:NOTHING
BRK             ENDP


; ----------------------------------------------------------------------------
; AX=7F02H: EAX:=ulimit
;
; Get greatest possible break value
;
; ----------------------------------------------------------------------------
;
; In:   ECX     Command code (must be UL_GMEMLIM or UL_OBJREST)
;       EDX     New limit (currently ignored)
;
; Out:  EAX     Return value:   
;                   greatest possible break value for UL_GMEMLIM
;                   remaining bytes in current heap object for UL_OBJREST
;       ECX     errno, if non-zero
;
; ----------------------------------------------------------------------------

                TALIGN  4
                ASSUME  DI:PTR PROCESS
SYS02:          CMP     I_ECX, UL_GMEMLIM
                JE      ULIMIT_GMEMLIM
                CMP     I_ECX, UL_OBJREST
                JE      ULIMIT_OBJREST
ULIMIT_ERR:     MOV     I_EAX, -1
                MOV     I_ECX, EINVAL
                RET

ULIMIT_OBJREST: CALL    HEAP_LIMIT
                SUB     EAX, [DI].P_BRK
                JMP     SHORT ULIMIT_RET

ULIMIT_GMEMLIM: CALL    HEAP_LIMIT
ULIMIT_RET:     MOV     I_EAX, EAX
                MOV     I_ECX, 0
                RET

HEAP_LIMIT      PROC    NEAR
                MOV     EAX, [DI].P_BRK         ; Return this on error
                MOV     EDX, [DI].P_STACK_ADDR
                CMP     EDX, [DI].P_BRK
                MOV     EBX, 80000000H          ; ...bad estimate
                JB      SHORT HEAP_LIMIT_1
                MOV     EBX, I_ESP
                SUB     EBX, 4096               ; Leave at least on page
                JC      SHORT HEAP_LIMIT_RET    ; of stack space (note 1)
HEAP_LIMIT_1:   CMP     EBX, [DI].P_BRK
                JB      SHORT HEAP_LIMIT_RET
                MOV     EAX, EBX
HEAP_LIMIT_RET: RET
HEAP_LIMIT      ENDP

                ASSUME  DI:NOTHING

; ----------------------------------------------------------------------------
; AX=7F03H: [EBX]:=statistics
;
; Virtual memory statistics
;
; In:   DS:EBX  Buffer
;       ECX     Size (in bytes) of buffer pointed to by DS:EBX
;
; Out:  [EBX+0] Number of page faults caused by this process
;       [EBX+4] Total number of page faults
;
; ----------------------------------------------------------------------------
                TALIGN  4
                ASSUME  DI:PTR PROCESS
SYS03:          MOV     EBX, I_EBX
                MOV     ES, I_DS
                MOV     ECX, I_ECX
                CMP     ECX, 1*4
                JB      SHORT SYS03_RET
                MOV     EAX, [DI].P_PAGE_FAULTS
                MOV     ES:[EBX+0], EAX
                CMP     ECX, 2*4
                JB      SHORT SYS03_RET
                MOV     EAX, SWAP_FAULTS
                MOV     ES:[EBX+4], EAX
SYS03_RET:      RET

                ASSUME  DI:NOTHING

; ----------------------------------------------------------------------------
; AX=7F04H: EAX:=__umask1(EDX)
;
; Set file-permission mask
;
; ----------------------------------------------------------------------------
                TALIGN  4
                ASSUME  DI:PTR PROCESS
SYS04:          MOVZX   EAX, [DI].P_UMASK1
                MOV     I_EAX, EAX
                MOV     AX, WORD PTR I_EDX
                NOT     AX
                AND     [DI].P_UMASK1, AX
                RET

                ASSUME  DI:NOTHING

; ----------------------------------------------------------------------------
; AX=7F05H: EAX:=getpid()
;
; Get process ID
;
; ----------------------------------------------------------------------------
                TALIGN  4
                ASSUME  DI:PTR PROCESS
SYS05:          MOV     EAX, [DI].P_PID
                MOV     I_EAX, EAX
                RET

                ASSUME  DI:NOTHING

; ----------------------------------------------------------------------------
; AX=7F06H: spawnve()
;
; Spawn child process
;
; In:   DS:EDX  Pointer to parameter table (NEW_PROC, see PROCESS.INC)
;               NP_PARENT will be filled in by this function.
;
; Out:  CY      Error
;       NC      Success
;       EAX     Error code (CY)
;       EAX     Process id (NC, asynchronous process)
;       EAX     Return code (NC, synchronous process)
;
; Note: Not reentrant (uses global variable NP2!).
;
; ----------------------------------------------------------------------------
                TALIGN  4
                ASSUME  DI:PTR PROCESS
SYS06:          MOV     BX, DI
                CALL    SAVE_PROCESS            ; Save registers
                CALL    MAKE_NP2                ; Copy param. table to NP2
                CALL    SPAWN_FNAME             ; Create program file name
                MOV     NP2.NP_FNAME_SEL, DS
                MOV     NP2.NP_FNAME_OFF, OFFSET PGM_NAME
                MOV     DI, PROCESS_PTR         ; Current process
                CMP     BYTE PTR NP2.NP_MODE1, NP_EXEC ; exec?
                JNE     SHORT SPAWN_2           ; No -> process is parent
                MOV     EAX, [DI].P_PPID        ; PID of parent process
                CALL    FIND_PROCESS            ; Find parent process
                CMP     BX, NO_PROCESS          ; Success?
                JNE     SHORT SPAWN_2           ; Yes -> has real parent
                LEA     BX, PROC0               ; No  -> use dummy parent
SPAWN_2:        MOV     NP2.NP_PARENT, BX       ; Set NP_PARENT field
                MOV     NP2.NP_FPROC, DI        ; Set source of file handles
                LEA     SI, NP2                 ; Parameter table
                CALL    NEW_PROCESS             ; Create new process
                OR      AX, AX                  ; Failure?
                JNZ     SHORT SPAWN_FAIL        ; Yes -> try DOS pgm or return
                MOV     BX, PROCESS_PTR         ; Outgoing process
                ASSUME  BX:PTR PROCESS
                CMP     BYTE PTR NP2.NP_MODE1, NP_DEBUG ; Debugging?
                JE      SHORT SPAWN_ASYNC       ; Yes -> return to parent
                CMP     BYTE PTR NP2.NP_MODE1, NP_SPAWN_ASYNC ; Asynchronous?
                JNE     SHORT SPAWN_3           ; No -> continue
                MOV     EAX, [DI].P_PID         ; Process ID of child
                MOV     [BX].P_SPAWN_RC, EAX    ; will be returned by spawn()
                OR      [BX].P_FLAGS, PF_PSEUDO_ASYNC
SPAWN_3:        MOV     [BX].P_STATUS, PS_WAIT_SPAWN
;
; Now run the new process synchronously
;
                MOV     PROCESS_PTR, DI         ; Switch to incoming process
                MOV     PROCESS_SIG, DI
                CMP     BYTE PTR NP2.NP_MODE1, NP_EXEC
                JNE     SHORT SPAWN_4
                CALL    REMOVE_PROCESS          ; TODO: FPUEMU_ENDPROC
SPAWN_4:        MOV     BX, DI
                CALL    REST_PROCESS            ; Start incoming process
                CALL    BREAK_AFTER_IRET
                MOV     EAX, [BX].P_NUMBER
                CALL    FPUEMU_NEWPROC          ; Notify FPU emulator
SPAWN_RET:      RET

;
; Return to the parent process after starting an asynchronous process.
; This is currently used for NP_DEBUG only.
;
SPAWN_ASYNC:    MOV     EAX, [DI].P_PID         ; Process ID of child
                MOV     I_EAX, EAX              ; Return process ID
                MOV     EAX, [DI].P_NUMBER
                CALL    FPUEMU_NEWPROC          ; Notify FPU emulator
                JMP     SHORT SPAWN_RET

SPAWN_FAIL:     CMP     AX, ENOEXEC             ; Not an emx program?
                JE      SHORT SPAWN_DOS         ; Yes -> try to run a DOS pgm
SPAWN_ERROR:    MOVZX   EAX, AX
                MOV     I_EAX, EAX              ; Failure
                OR      BYTE PTR I_EFLAGS, FLAG_C
                RET

                ASSUME  BX:NOTHING
                ASSUME  DI:NOTHING
;
; Try to run the program as ordinary DOS program
;
SPAWN_DOS:      CMP     BYTE PTR NP2.NP_MODE1, NP_SPAWN_SYNC ; Synchronous?
                JE      SHORT SPAWN_DOS_1       ; Yes -> continue
                CMP     BYTE PTR NP2.NP_MODE1, NP_SPAWN_ASYNC ; Asynchronous?
                JNE     SHORT SPAWN_ERROR       ; No -> error
SPAWN_DOS_1:    CMP     DISABLE_LOW_MEM, FALSE  ; Low memory in use?
                JNE     SHORT DOS_10            ; No  -> ok
                MOV     AX, ENOMEM              ; Not enough memory
                JMP     SHORT SPAWN_ERROR
;
; Build command line
;
DOS_10:         CALL    DOS_COMMAND             ; Look for COMMAND.COM
                CALL    DOS_ARGS                ; Build command line
                JC      SHORT DOS_TOO_BIG
                CALL    DOS_ENV                 ; Build environment
                OR      AX, AX
                JNZ     SHORT SPAWN_ERROR       ; Error -> return errno
                CALL    MAP_ALL_HANDLES         ; Remap file handles
                JC      SHORT SPAWN_ERROR       ; Error -> return errno
;
; Try to run the program
;
                LEA     DX, PGM_NAME
                LEA     BX, PAR_BLOCK
                MOV     AX, 4B00H               ; Load and execute program
                INT     21H
                MOV     I_EAX, EAX
                JNC     SHORT DOS_OK
                OR      BYTE PTR I_EFLAGS, FLAG_C
                JMP     SPAWN_RET

DOS_OK:         MOV     AH, 4DH                 ; Get ret code of child process
                INT     21H
                MOVZX   EAX, AX                 ; Put it in EAX
                MOV     BX, PROCESS_PTR         ; Parent process
                ASSUME  BX:PTR PROCESS
                CMP     BYTE PTR NP2.NP_MODE1, NP_SPAWN_ASYNC ; Pseudo async?
                JNE     SHORT DOS_SPAWN_RC      ; No -> return return code
                MOV     [BX].P_RC, AL           ; Save the return code
                CALL    CREATE_PID              ; Return a process ID
                OR      [BX].P_FLAGS, PF_PSEUDO_ASYNC
                CMP     [BX].P_SIG_HANDLERS[4*SIGCLD], SIG_IGN
                JE      SHORT DOS_SPAWN_RC
                CMP     [BX].P_SIG_HANDLERS[4*SIGCLD], SIG_DFL
                JE      SHORT DOS_SPAWN_RC
                BTS     [BX].P_SIG_PENDING, SIGCLD      ; Generate SIGCLD
DOS_SPAWN_RC:   MOV     [BX].P_SPAWN_RC, EAX
                MOV     I_EAX, EAX
                JMP     SPAWN_RET
                ASSUME  BX:NOTHING

;
; Arguments or environment too big
;
DOS_TOO_BIG:    MOV     AX, E2BIG
                JMP     SPAWN_ERROR

;
;
;
                TALIGN  4
SPAWN_FNAME     PROC    NEAR
                PUSH    ESI
                PUSH    EDI
                PUSH    DS
                LEA     DI, PGM_NAME
                MOV_ES_DS
                MOV     ESI, NP2.NP_FNAME_OFF
                MOV     DS, NP2.NP_FNAME_SEL
                ASSUME  DS:NOTHING
                MOV     CX, PGM_NAME_SIZE - 1
SPAWN_FNAME_1:  LODS    BYTE PTR DS:[ESI]
                OR      AL, AL
                JZ      SHORT SPAWN_FNAME_2
                STOS    BYTE PTR ES:[DI]
                LOOP    SPAWN_FNAME_1
SPAWN_FNAME_2:  XOR     AL, AL
                STOS    BYTE PTR ES:[DI]
                POP     DS
                ASSUME  DS:SV_DATA
                LEA     EDI, PGM_NAME
                CALL    TRUNCATE
                POP     EDI
                POP     ESI
                RET
SPAWN_FNAME     ENDP

;
; Check for COMMAND.COM and 4DOS.COM
;

DOS_COMMAND     PROC    NEAR
                MOV     IS_COMMAND, FALSE       ; Standard quoting by default
                LEA     SI, PGM_NAME            ; Find base name
                MOV     DI, SI
DC_1:           LODSB
                OR      AL, AL
                JZ      SHORT DC_3
                CMP     AL, "/"
                JE      SHORT DC_2
                CMP     AL, "\"
                JE      SHORT DC_2
                CMP     AL, ":"
                JNE     SHORT DC_1
DC_2:           MOV     DI, SI
                JMP     SHORT DC_1

DC_3:           MOV_ES_DS
                LEA     SI, $COMMAND            ; command.com?
                CALL    STRICMP
                JE      SHORT DC_4              ; Yes ->
                LEA     SI, $4DOS               ; 4dos.com?
                CALL    STRICMP
                JNE     SHORT DC_RET            ; No  -> skip
DC_4:           MOV     IS_COMMAND, NOT FALSE   ; Don't quote arguments
DC_RET:         RET
DOS_COMMAND     ENDP

;
; Build command line for DOS programs
;
; In:  NP2      Parameter block
;
; Out: CY       Command line too big
;
DOS_ARGS        PROC    NEAR PASCAL
                LOCAL   ARG_COUNTER:WORD
                LOCAL   NO_QUOTE:BYTE
                LOCAL   FORCE_QUOTE:BYTE

                PUSH    DS                      ; Save DS
                MOV     AL, IS_COMMAND          ; No quoting if command.com
                MOV     NO_QUOTE, AL            ; Copy to stack segment
                MOV     FORCE_QUOTE, FALSE
                TEST    NP2.NP_MODE2, NP2_QUOTE
                JNZ     SHORT DA_1
                MOV     BX, PROCESS_PTR
                CMP     BX, NO_PROCESS
                JE      SHORT DA_2
                TEST    (PROCESS PTR [BX]).P_FLAGS, PF_QUOTE
                JZ      SHORT DA_2
DA_1:           NOT     FORCE_QUOTE
DA_2:           MOV     DX, 0                   ; Size of command line
                MOV     AX, NP2.NP_ARG_COUNT    ; Number of arguments
                MOV_ES_DS
                LEA     DI, CMD_LINE + 1
                MOV     ESI, NP2.NP_ARG_OFF
                MOV     DS, NP2.NP_ARG_SEL
                ASSUME  DS:NOTHING
                CLD
;
; Skip argv[0]
;
                CMP     AX, 2
                JB      DONE                    ; No arguments -> done
                DEC     AX                      ; argv[0] skipped
                MOV     ARG_COUNTER, AX
                LODS    BYTE PTR DS:[ESI]       ; Get flags byte
                OR      AL, AL                  ; End of arguments?
                JZ      DONE                    ; Yes -> done
SKIP_ARGV0:     LODS    BYTE PTR DS:[ESI]       ; Fetch character
                OR      AL, AL                  ; End of argument?
                JNZ     SKIP_ARGV0              ; No  -> repeat
;
; Next argument
;
NEXT:           LODS    BYTE PTR DS:[ESI]       ; Get flags byte
                OR      DX, DX
                JZ      SHORT ARG_SKIP_FLAG     ; First argument -> skip
                INC     DX
                CMP     DX, CMD_LINE_SIZE - 2   ; Too long?
                JA      DONE                    ; Yes -> end
                MOV     AL, " "                 ; Delimit arguments
                STOS    BYTE PTR ES:[DI]
;
; Look for blanks and tabs. Quote the entire argument if there is
; a blank or a tab.
;
ARG_SKIP_FLAG:  CMP     NO_QUOTE, FALSE         ; Quote?
                JNE     SHORT ARG_CHECK_NO      ; No  -> skip
                CMP     BYTE PTR DS:[ESI], 0    ; Empty argument?
                JE      SHORT ARG_CHECK_YES     ; Yes -> needs quoting
                PUSH    ESI
                XOR     AH, AH                  ; No quoting required
ARG_CHECK:      LODS    BYTE PTR DS:[ESI]       ; Fetch character
                TEST    AL, AL                  ; End of argument?
                JZ      SHORT ARG_CHECK_END     ; Yes -> no quoting required
                CMP     AL, " "                 ; Blank?
                JE      SHORT ARG_CHECK_END     ; Yes -> quote the argument
                CMP     AL, TAB                 ; Tab?
                JNE     SHORT ARG_CHECK         ; No  -> check next character
ARG_CHECK_END:  POP     ESI
                TEST    AL, AL
                JNZ     SHORT ARG_CHECK_YES
                CMP     FORCE_QUOTE, FALSE      ; P_QUOTE or -q?
                JE      SHORT ARG_CHECK_NO      ; No  -> don't quote
;
; Check for @, ? or *
;
                PUSH    ESI
                MOV     AL, 1
                CMP     BYTE PTR DS:[ESI], "@"  ; Argument starting with "@"?
                JNE     SHORT AC_200
                CMP     BYTE PTR DS:[ESI+1], 0
                JNE     SHORT AC_299            ; Yes -> quote (AL non-zero)
AC_200:         LODS    BYTE PTR DS:[ESI]       ; Fetch character
                TEST    AL, AL                  ; End of argument?
                JZ      SHORT AC_299            ; Yes -> don't quote (AL=0)
                CMP     AL, "?"                 ; Wildcard character?
                JE      SHORT AC_299            ; Yes -> quote (AL non-zero)
                CMP     AL, "*"                 ; Wildcard character?
                JNE     SHORT AC_200            ; No  -> check next character
AC_299:         POP     ESI
                TEST    AL, AL
                JZ      SHORT ARG_CHECK_NO
ARG_CHECK_YES:  MOV     AH, 1                   ; Flag for closing quote
                INC     DX
                CMP     DX, CMD_LINE_SIZE - 2   ; Too long?
                JA      SHORT DONE              ; Yes -> end
                MOV     AL, '"'                 ; Store opening quote
                STOS    BYTE PTR ES:[DI]
ARG_CHECK_NO:   XOR     CX, CX                  ; Clear backslash counter
;
; Copy argument
;
ARG_COPY:       LODS    BYTE PTR DS:[ESI]       ; Fetch character
                OR      AL, AL                  ; End of argument?
                JZ      SHORT ARG_END           ; Yes -> next argument
                CMP     NO_QUOTE, FALSE         ; Quoting disabled?
                JNE     SHORT ARG_STORE         ; Yes -> simply store the char
                CMP     AL, "\"                 ; Backslash?
                JE      SHORT ARG_BACKSLASH     ; Yes  -> count
                CMP     AL, '"'                 ; Quote?
                JE      SHORT ARG_QUOTE         ; Yes -> handle backslashes
                XOR     CX, CX                  ; Clear backslash counter
ARG_STORE:      INC     DX
                CMP     DX, CMD_LINE_SIZE - 2   ; Too long?
                JA      SHORT DONE              ; Yes -> end
                STOS    BYTE PTR ES:[DI]
                JMP     SHORT ARG_COPY

                TALIGN  4
ARG_BACKSLASH:  INC     CX                      ; Increment backslash counter
                JMP     SHORT ARG_STORE         ; Store character

                TALIGN  4
ARG_QUOTE:      INC     CX                      ; 2n+1 backslashes
                ADD     DX, CX
                CMP     DX, CMD_LINE_SIZE - 2   ; Too long?
                JA      SHORT DONE              ; Yes -> end
                MOV     AL, "\"                 ; Store backslashes
ARG_QUOTE_1:    STOSB
                LOOP    ARG_QUOTE_1
                MOV     AL, '"'
                JMP     SHORT ARG_STORE

ARG_END:        OR      AH, AH                  ; Closing quote required?
                JZ      SHORT MORE              ; No  -> skip
                JCXZ    ARG_END_2
                ADD     DX, CX
                CMP     DX, CMD_LINE_SIZE - 2   ; Too long?
                JA      SHORT DONE              ; Yes -> end
                MOV     AL, "\"
ARG_END_1:      STOSB
                LOOP    ARG_END_1
ARG_END_2:      MOV     AL, '"'
                INC     DX
                CMP     DX, CMD_LINE_SIZE - 2   ; Too long?
                JA      SHORT DONE              ; Yes -> end
                STOSB
MORE:           DEC     ARG_COUNTER
                JNZ     NEXT
DONE:           POP     DS                      ; Restore DS
                ASSUME  DS:SV_DATA
                CMP     DX, CMD_LINE_SIZE - 2
                JA      SHORT TOO_BIG
                MOV     CMD_LINE[0], DL         ; Length = 0
                MOV     AL, CR
                STOS    BYTE PTR ES:[DI]        ; End of command line
                CLC
FIN:            RET

TOO_BIG:        STC
                JMP     SHORT FIN
DOS_ARGS        ENDP


; ----------------------------------------------------------------------------
;
; Build environment for DOS program
;
; Out:  AX      errno (0 if no error)
;       PB_ENV  Segment of environment
;
; ----------------------------------------------------------------------------
DOS_ENV         PROC    NEAR
                CMP     NP2.NP_ENV_SIZE, 8000H  ; Fail with E2BIG if the
                MOV     AX, E2BIG               ; environment is too big
                JAE     SHORT ERR
;
; Copy the environment to the real-mode buffer (that buffer is not used
; for INT 21H, AH=4BH).
;
                MOV     AX, NP2.NP_ENV_SEL
                MOV     ESI, NP2.NP_ENV_OFF
                MOV     EDI, 0                  ; OFFSET_1 is not zero!
                MOVZX   ECX, NP2.NP_ENV_SIZE
                CALL    MOVE_TO_RM
                MOV     AX, BUF_SEG             ; Set the environment pointer
                MOV     PB_ENV, AX              ; in the parameter block
OK:             XOR     AX, AX                  ; No error
ERR:            RET
DOS_ENV         ENDP


; ----------------------------------------------------------------------------
; Copy NEW_PROC (_new_proc) structure from user process' data segment to
; supervisor data segment
;
; In:   I_EDX   Pointer to structure in user process' data segment
;
; ----------------------------------------------------------------------------
                TALIGN  4
MAKE_NP2        PROC    NEAR
                PUSH    DS
                XOR     EDI, EDI
                LEA     DI, NP2
                MOV_ES_DS
                MOV     ESI, I_EDX
                MOV     DS, I_DS
                ASSUME  DS:NOTHING
                MOV     ECX, NP_USER_SIZE
                REP     MOVS BYTE PTR ES:[EDI], BYTE PTR DS:[ESI]
                POP     DS
                ASSUME  DS:SV_DATA
                MOV     NP2.NP_MODE2, 0
                TEST    NP2.NP_MODE1, 8000H
                JZ      SHORT MNP2_1
                MOV     ES, I_DS
                MOV     ESI, I_EDX
                ASSUME  ESI:NEAR32 PTR NEW_PROC
                MOV     AX, ES:[ESI].NP_MODE2
                MOV     NP2.NP_MODE2, AX
                ASSUME  ESI:NOTHING
MNP2_1:         RET
MAKE_NP2        ENDP


; ----------------------------------------------------------------------------
; AX=7F09H: __wait()
;
; Wait for child process
;
; In:   --
;
; Out:  EAX     Process ID of child process (-1 if no children)
;       ECX     errno
;       EDX     Termination status
;
; ----------------------------------------------------------------------------
                TALIGN  4
                ASSUME  DI:PTR PROCESS
SYS09:          TEST    [DI].P_FLAGS, PF_PSEUDO_ASYNC ; Pseudo async child?
                JNZ     SHORT WAIT_PSEUDO       ; Yes -> return its status
                MOV     EAX, [DI].P_PID         ; Get process ID
                CALL    WAIT_PROCESS            ; Look for children
                CMP     BX, NO_PROCESS          ; Children?
                JE      SHORT SYS09_ERROR       ; No  -> error
                CMP     BX, NO_WPROCESS         ; Stopped/terminated children?
                JE      SHORT SYS09_WAIT        ; No  -> wait
                ASSUME  BX:PTR PROCESS
                AND     [BX].P_FLAGS, NOT PF_WAIT_WAIT  ; wait done
                MOV     EAX, [BX].P_PID         ; Return process ID
                MOV     I_EAX, EAX
                XOR     EAX, EAX                ; Termination status := 0
                CMP     [BX].P_STATUS, PS_STOP  ; Stopped process?
                JE      SHORT SYS09_STOP        ; Yes ->
                MOV     AH, [BX].P_RC           ; Return code -> bits 8..15
                JMP     SHORT SYS09_OK
SYS09_STOP:     MOV     AL, 177Q
                MOV     AH, [BX].P_SIG_NO       ; Signal number
SYS09_OK:       MOV     I_EDX, EAX              ; Return status in EDX
                MOV     I_ECX, 0                ; No error
                CMP     [BX].P_STATUS, PS_DEFUNCT ; Zombie process?
                JNE     SHORT SYS09_RET         ; No  -> skip
                CALL    KILL_PROCESS            ; Remove last vestiges
                ASSUME  BX:NOTHING
SYS09_RET:      RET


;
; There are children, but no one stopped/terminated. Wait.
; Not implemented, return error.
;
SYS09_WAIT:
SYS09_ERROR:    MOV     I_ECX, ECHILD
                MOV     I_EAX, -1
                RET

;
; We had a pseudo asynchronous child process.  Return its termination
; status.
;
WAIT_PSEUDO:    AND     [DI].P_FLAGS, NOT PF_PSEUDO_ASYNC       ; Child is dead
                XOR     EAX, EAX
                MOV     AH, [DI].P_RC           ; Get the return code
                MOV     I_EDX, EAX              ; Set termination status
                MOV     EAX, [DI].P_SPAWN_RC    ; Process ID
                MOV     I_EAX, EAX
                MOV     I_ECX, 0                ; No error
                RET

                ASSUME  DI:NOTHING


; ----------------------------------------------------------------------------
; AX=7F0AH: version
;
; Return emx version
;
; In:   --
;
; Out:  EAX     Version number
;                 Bits 0..7:    version letter ("a".."z")
;                 Bits 8..15:   minor version number ("0".."9")
;                 Bits 16..23:  2EH (".")
;                 Bits 24..31:  major version number ("0".."9")
;       EBX     Environment:
;                 Bit 0:        VCPI
;                 Bit 1:        XMS
;                 Bit 2:        VDISK.SYS 3.3
;                 Bit 3:        DESQview
;                 Bit 4:        287
;                 Bit 5:        387
;                 Bit 6:        486      (not implemented)
;                 Bit 7:        DPMI 0.9 (not implemented)
;                 Bit 8:        DPMI 1.0 (not implemented)
;                 Bit 9:        OS/2 2.0
;                 Bit 10:       -t option given
;                 Bits 6..31:   0 (reserved for future expansion)
;       ECX     Revision index
;       EDX     0 (reserved for future expansion)
;
; ----------------------------------------------------------------------------
                TALIGN  4
                ASSUME  DI:PTR PROCESS
SYS0A:          MOV     I_EAX, VERSION          ; Return version number in EAX
                XOR     EAX, EAX                ; Clear all environment bits
                CMP     VCPI_FLAG, FALSE
                JE      SHORT SYS0A_1
                OR      AL, 01H                 ; Set VCPI bit
SYS0A_1:        CMP     XMS_FLAG, FALSE
                JE      SHORT SYS0A_2
                OR      AL, 02H                 ; Set XMS bit
SYS0A_2:        CMP     VDISK_FLAG, FALSE
                JE      SHORT SYS0A_3
                OR      AL, 04H                 ; Set VDISK bit
SYS0A_3:        CMP     DV_FLAG, FALSE
                JE      SHORT SYS0A_4
                OR      AL, 08H                 ; Set DESQview bit
SYS0A_4:        CMP     FP_FLAG, FP_287
                JNE     SHORT SYS0A_5
                OR      AL, 10H                 ; Set 287 bit
SYS0A_5:        CMP     FP_FLAG, FP_387
                JNE     SHORT SYS0A_6
                OR      AL, 20H                 ; Set 387 bit
SYS0A_6:        CMP     [DI].P_TRUNC, 0
                JE      SHORT SYS0A_7
                OR      AX, 400H                ; Set -t bit
SYS0A_7:        TEST    [DI].P_HW_ACCESS, HW_ACCESS_CODE
                JZ      SHORT SYS0A_8
                OR      AX, 800H                ; Set -ac bit
SYS0A_8:        MOV     I_EBX, EAX
                MOV     I_ECX, REV_INDEX_BIN    ; Revision index
                MOV     I_EDX, 0                ; Reserved
                RET

                ASSUME  DI:NOTHING

; ----------------------------------------------------------------------------
; AX=7F0BH: memavail
;
; Return number of available pages of memory (w/o swapping)
;
; In:   --
;
; Out:  EAX     Number of available pages
;
; ----------------------------------------------------------------------------
                TALIGN  4
SYS0B:          CALL    PM_AVAIL                ; This call does all the work
                MOV     I_EAX, EAX              ; Return the result in EAX
                RET

; ----------------------------------------------------------------------------
; AX=7F0CH: __signal()
;
; Set signal handler
;
; In:   ECX     Signal number
;       EDX     Address of signal handler or SIG_ACK, SIG_DFL, SIG_IGN
;
; Out:  EAX     Previous value (success) or SIG_ERR (error)
;
; ----------------------------------------------------------------------------
                TALIGN  4
                ASSUME  DI:PTR PROCESS
SYS0C:          MOV     EAX, I_ECX              ; Signal number
                MOV     EDX, I_EDX              ; Signal handler
                CMP     EDX, SIG_ACK            ; Acknowledge?
                JE      SHORT SIGNAL_ACK        ; Yes ->
                CALL    DO_SIGNAL
                MOV     I_EAX, EAX
                RET

;
; Unblock the signal EAX.  If there is a pending signal, it will be
; raised on return to the current process.
;
SIGNAL_ACK:     CMP     EAX, SIGNALS            ; Valid signal number?
                JAE     SHORT SIGNAL_ERROR      ; No  -> failure
                CMP     SIG_VALID[EAX], FALSE
                JE      SHORT SIGNAL_ERROR      ; No  -> failure
                MOV     BX, AX
                SHL     BX, 2
                TEST    [DI].P_SA_FLAGS[BX], SA_ACK ; SA_ACK set?
                JZ      SHORT SIGNAL_ERROR      ; No  -> failure
                BTR     [DI].P_SIG_BLOCKED, EAX ; Unblock the signal
                MOV     EAX, [DI].P_SIG_HANDLERS[BX] ; Get previous value
                MOV     I_EAX, EAX              ; Return previous value
                RET

SIGNAL_ERROR:   MOV     I_EAX, SIG_ERR          ; Return SIG_ERR
                RET

                ASSUME  DI:NOTHING

; ----------------------------------------------------------------------------
; AX=7F0DH: __kill()
;
; Send a signal to a process
;
; In:   ECX     Signal number
;       EDX     Process ID
;
; Out:  EAX     0 if successful, -1 otherwise
;       ECX     errno
;
; (As we don't have real multitasking, we should switch to the other
; process.)
;
; ----------------------------------------------------------------------------
                TALIGN  4
SYS0D:          MOV     EAX, I_EDX              ; Get process ID
                CALL    FIND_PROCESS            ; Find process for given ID
                CMP     BX, NO_PROCESS          ; Found?
                JE      SHORT KILL_BAD_PID      ; No  -> error
                CMP     I_ECX, 0                ; Signal 0 (check PID)?
                JE      SHORT RAISE_OK          ; Yes -> ok
                MOV     DI, BX                  ; Copy process pointer to DI
;
; Fall through to __raise() code
;
; ----------------------------------------------------------------------------
; AX=7F0EH: __raise()
;
; Raise a signal (in current process)
;
; In:   ECX     Signal number
;
; Out:  EAX     0 if successful, a non-zero value otherwise
;       ECX     errno
;
; ----------------------------------------------------------------------------
                TALIGN  4
                ASSUME  DI:PTR PROCESS
SYS0E:          MOV     EAX, I_ECX              ; Get signal number
                CMP     EAX, SIGNALS            ; Valid signal number?
                JAE     SHORT RAISE_ERROR       ; No  -> failure
                CMP     SIG_VALID[EAX], FALSE
                JE      SHORT RAISE_ERROR       ; No  -> failure
                BTS     [DI].P_SIG_PENDING, EAX ; Generate signal
RAISE_OK:       MOV     I_ECX, 0                ; No error
                MOV     I_EAX, 0                ; Success
                RET

RAISE_ERROR:    MOV     I_ECX, EINVAL
                MOV     I_EAX, -1               ; Error
                RET

KILL_BAD_PID:   MOV     I_ECX, ESRCH
                MOV     I_EAX, -1
                RET

                ASSUME  DI:NOTHING

; ----------------------------------------------------------------------------
; AX=7F0FH: EAX:=uflags(ECX,EDX)
;
; Set user flags
;
; ----------------------------------------------------------------------------
                TALIGN  4
                ASSUME  DI:PTR PROCESS
SYS0F:          MOV     EAX, [DI].P_UFLAGS
                MOV     I_EAX, EAX              ; Return previous value
                MOV     ECX, I_ECX              ; Get mask
                MOV     EDX, I_EDX              ; Get new bits
                AND     EDX, ECX                ; Clear unwanted bits
                NOT     ECX
                AND     EAX, ECX                ; Clear bits to be changed
                OR      EAX, EDX                ; Insert new bits
                MOV     [DI].P_UFLAGS, EAX      ; Store new value
                RET

                ASSUME  DI:NOTHING

; ----------------------------------------------------------------------------
; AX=7F10H: __unwind()
;
; Unwind signal handlers for longjmp()
;
; Currently not used by the DOS version of emx.
;
; ----------------------------------------------------------------------------

                TALIGN  4
SYS10:          RET

; ----------------------------------------------------------------------------
; AX=7F11H: __core()
;
; Write core image file
;
; In:   EBX     File handle
;
; Out:  CY      Error
;       EAX     errno (CY)
;
; ----------------------------------------------------------------------------

                TALIGN  4
SYS11:          MOV     BX, DI                  ; Process table entry
                CALL    CORE_REGS_I
                MOV     EAX, I_EBX              ; Get file handle
                CALL    CORE_MAIN               ; Write core image file
                JNC     SHORT SYS11_RET
                OR      I_EFLAGS, FLAG_C
                CALL    DOS_ERROR_TO_ERRNO
                MOV     I_EAX, EAX
SYS11_RET:      RET

; ----------------------------------------------------------------------------
; AX=7F12H: _portaccess()
;
; Enable access to I/O ports
;
; In:   ECX    First port number
;       EDX    Last port number
;
; Out:  CY     Failed
;       EAX    errno (0 if successful)
;
; ----------------------------------------------------------------------------

                TALIGN  4
                ASSUME  DI:PTR PROCESS
SYS12:          MOV     EAX, I_ECX
                CMP     EAX, 10000H
                JAE     SHORT PORTACCESS_ERR
                MOV     ECX, I_EDX
                CMP     ECX, 10000H
                JAE     SHORT PORTACCESS_ERR
                SUB     ECX, EAX
                JB      SHORT PORTACCESS_ERR
                INC     ECX
                TEST    [DI].P_HW_ACCESS, HW_ACCESS_IO
                JZ      SHORT PORTACCESS_ERR
                CMP     NEW_TSS, FALSE
                JE      SHORT PORTACCESS_ERR
                MOV     DX, G_TSS_MEM_SEL
                MOV     FS, DX
                MOVZX   EBX, (TSS_STRUC PTR FS:[0]).TSS_BIT_MAP_OFF
PORTACCESS_1:   BTR     DWORD PTR FS:[EBX], EAX
                INC     EAX
                LOOPD   PORTACCESS_1
                MOV     I_EAX, 0
                RET

PORTACCESS_ERR: MOV     I_EAX, EACCES
                OR      I_EFLAGS, FLAG_C
                RET

                ASSUME  DI:NOTHING

; ----------------------------------------------------------------------------
; AX=7F13H: _memaccess()
;
; Enable access to memory
;
; In:   EBX    First address
;       ECX    Last address
;       EDX    0: read access, 1:write access
;
; Out:  CY     Failed
;       EAX    errno if failed (CY)
;       EAX    Pointer to start of mapped memory area (NC)
;
; ----------------------------------------------------------------------------

                TALIGN  4
                ASSUME  DI:PTR PROCESS
SYS13:          CMP     I_EDX, 1
                JA      MEMACCESS_INV
                MOV     EAX, I_EBX
                TEST    EAX, 0FFFH
                JNZ     MEMACCESS_ACC
                MOV     ECX, I_ECX
                INC     ECX
                JNZ     SHORT MEMACCESS_01
                OR      EAX, EAX
                JZ      MEMACCESS_MEM
                SUB     ECX, EAX
                JMP     SHORT MEMACCESS_02
MEMACCESS_01:   TEST    ECX, 0FFFH
                JNZ     MEMACCESS_ACC
                SUB     ECX, EAX
                JBE     MEMACCESS_ACC
MEMACCESS_02:   TEST    [DI].P_HW_ACCESS, HW_ACCESS_MEM
                JZ      MEMACCESS_ACC
                MOV     EBX, I_EBX
                OR      EBX, PAGE_PRESENT OR PAGE_USER OR PAGE_LOCKED
                CMP     I_EDX, 0
                JE      SHORT MEMACCESS_RD
                CMP     I_EBX, 0A0000H
                JB      SHORT MEMACCESS_DNG     ; Danger!
                CMP     I_ECX, 0BFFFFH
                JBE     SHORT MEMACCESS_WR
MEMACCESS_DNG:  TEST    [DI].P_HW_ACCESS, HW_ACCESS_WRITE
                JZ      SHORT MEMACCESS_ACC
MEMACCESS_WR:   OR      EBX, PAGE_WRITE
MEMACCESS_RD:   MOV     EDX, SRC_NONE
                SHR     ECX, 12
                CALL    MAP_PHYS
                TEST    EAX, EAX
                JZ      SHORT MEMACCESS_MEM
                MOV     I_EAX, EAX
                RET

MEMACCESS_INV:  MOV     I_EAX, EINVAL
                OR      I_EFLAGS, FLAG_C
                RET

MEMACCESS_ACC:  MOV     I_EAX, EACCES
                OR      I_EFLAGS, FLAG_C
                RET

MEMACCESS_MEM:  MOV     I_EAX, ENOMEM
                OR      I_EFLAGS, FLAG_C
                RET

                ASSUME  DI:NOTHING

; ----------------------------------------------------------------------------
; AX=7F14H: __ioctl2()
;
; UNIX-like termio ioctl()
;
; In:   EBX     Handle
;       ECX     Request code
;       EDX     Argument
;
; Out:  EAX     Return value
;       ECX     errno
;
; ----------------------------------------------------------------------------

                TALIGN  4
SYS14:          CMP     I_ECX, TCGETA
                JE      SHORT TC_GETA
                CMP     I_ECX, TCSETA
                JE      SHORT TC_SETA
                CMP     I_ECX, TCSETAW
                JE      SHORT TC_SETA
                CMP     I_ECX, TCSETAF
                JE      SHORT TC_SETAF
                CMP     I_ECX, TCFLSH
                JE      IOCTL_FLUSH
                CMP     I_ECX, TCXONC
                JE      IOCTL_XONC
                CMP     I_ECX, TCSBRK
                JE      IOCTL_SBRK
                CMP     I_ECX, _TCGA
                JE      SHORT TCS_GETA
                CMP     I_ECX, _TCSANOW
                JE      SHORT TCS_SETA
                CMP     I_ECX, _TCSADRAIN
                JE      SHORT TCS_SETA
                CMP     I_ECX, _TCSAFLUSH
                JE      SHORT TCS_SETAF
                CMP     I_ECX, FIONREAD
                JE      IOCTL_NREAD
                CMP     I_ECX, FGETHTYPE
                JE      IOCTL_HTYPE
                JMP     EMX_EINVAL

                TALIGN  4
TC_GETA:        MOV     EAX, I_EBX
                MOV     ES, I_DS
                MOV     EBX, I_EDX
                CALL    TERMIO_GET
TC_RET:         MOV     I_EAX, EAX
                MOV     I_ECX, ECX
                RET

                TALIGN  4
TC_SETAF:       MOV     EAX, I_EBX
                CALL    TERMIO_FLUSH
                OR      EAX, EAX
                JNZ     SHORT TC_RET
TC_SETA:        MOV     EAX, I_EBX
                MOV     ES, I_DS
                MOV     EBX, I_EDX
                CALL    TERMIO_SET
                JMP     SHORT TC_RET

                TALIGN  4
TCS_GETA:       MOV     EAX, I_EBX
                MOV     ES, I_DS
                MOV     EBX, I_EDX
                CALL    TERMIOS_GET
                JMP     SHORT TC_RET

                TALIGN  4
TCS_SETAF:      MOV     EAX, I_EBX
                CALL    TERMIOS_FLUSH
                OR      EAX, EAX
                JNZ     SHORT TC_RET
TCS_SETA:       MOV     EAX, I_EBX
                MOV     ES, I_DS
                MOV     EBX, I_EDX
                CALL    TERMIOS_SET
                JMP     SHORT TC_RET

;
; TCFLSH
;
                TALIGN  4
IOCTL_FLUSH:    CMP     I_EBX, 0                ; stdin?
                JNE     EMX_EBADF
                CMP     I_EDX, 0                ; Flush input queue
                JE      SHORT IOCTL_FLUSH_1
                CMP     I_EDX, 1                ; Flush output queue
                JE      EMX_OK                  ; Not implemented -- ignore
                CMP     I_EDX, 2                ; Flush input & output queues
                JNE     EMX_EINVAL              ; No  -> error
IOCTL_FLUSH_1:  CALL    KBD_FLUSH
                JMP     EMX_OK

;
; TCSBRK (not implemented)
;
                TALIGN  4
IOCTL_SBRK:     JMP     EMX_OK

;
; TCXCONC (not yet implemented)
;
                TALIGN  4
IOCTL_XONC:     JMP     EMX_OK

                TALIGN  4
IOCTL_NREAD:    CMP     I_EBX, 0                ; stdin?
                JNE     SHORT IOCTL_NREAD2      ; No  -> check input status
                TEST    STDIN_TERMIO.C_LFLAG, IDEFAULT OR ICANON ; termio?
                JNZ     SHORT IOCTL_NREAD2      ; No  -> check input status
                CALL    STDIN_AVAIL
IOCTL_NREAD1:   MOV     EBX, I_EDX
                MOV     ES, I_DS
                MOV     ES:[EBX], EAX
                JMP     EMX_OK

IOCTL_NREAD2:   MOV     EBX, I_EBX              ; Get file handle
                MOV     AX, 4406H               ; IOCTL: Check input status
                INT     21H
                JC      SHORT IOCTL_ERR         ; Error -> done
                OR      AL, AL                  ; 00H=not ready, 0FFH=ready
                MOV     EAX, 0                  ; No characters ready
                JZ      SHORT IOCTL_NREAD1      ; Not ready -> 0 characters
                INC     EAX                     ; Ready     -> 1 character
                JMP     SHORT IOCTL_NREAD1

IOCTL_ERR:      MOV     I_ECX, EAX
                MOV     I_EAX, -1
                RET

HT_FILE         =       0
HT_UPIPE        =       1
HT_NPIPE        =       2
HT_DEV_OTHER    =       3
HT_DEV_NUL      =       4
HT_DEV_CON      =       5
HT_DEV_CLK      =       7

                TALIGN  4
IOCTL_HTYPE:    MOV     AX, 4400H
                MOV     EBX, I_EBX
                INT     21H
                JC      SHORT HTYPE_ERR
                MOV     ECX, HT_FILE
                TEST    EDX, 80H
                JZ      SHORT HTYPE_RET
                MOV     ECX, HT_DEV_CON
                TEST    EDX, 03H
                JNZ     SHORT HTYPE_RET
                MOV     ECX, HT_DEV_NUL
                TEST    EDX, 04H
                JNZ     SHORT HTYPE_RET
                MOV     ECX, HT_DEV_CLK
                TEST    EDX, 08H
                JNZ     SHORT HTYPE_RET
                MOV     ECX, HT_DEV_OTHER
HTYPE_RET:      MOV     EBX, I_EDX
                MOV     ES, I_DS
                MOV     ES:[EBX], ECX
                JMP     EMX_OK

HTYPE_ERR:      MOV     I_ECX, EAX
                MOV     I_EAX, -1
                RET


; ----------------------------------------------------------------------------
; AX=7F15H: __alarm()
;
; Set alarm clock
;
; In:   EDX     Seconds
;
; Out:  EAX     Time remaining
;
; ----------------------------------------------------------------------------

                TALIGN  4
SYS15:          MOV     EAX, I_EDX
                MOV     ECX, 91
                MUL     ECX                     ; Multiply by 18.2
                ADD     EAX, 4
                ADC     EDX, 0
                MOV     ECX, 5
                CMP     EDX, ECX                ; Overflow?
                JAE     SHORT ALARM_OV          ; Yes -> use maximum value
                DIV     ECX
ALARM_SET:      MOV     BX, DI
                MOV     DX, TT_ALARM
                CALL    SET_TIMER
                MOV     I_EAX, 0
                JC      SHORT SYS15_RET
                MOV     ECX, 5
                MUL     ECX                     ; Divide by 18.2
                MOV     ECX, 91
                DIV     ECX
                MOV     I_EAX, EAX
SYS15_RET:      RET

ALARM_OV:       MOV     EAX, MAX_32             ; Use maximum value
                JMP     SHORT ALARM_SET

; ----------------------------------------------------------------------------
; AX=7F16H: Used internally for polling the keyboard
; ----------------------------------------------------------------------------

                TALIGN  4
SYS16:          TEST    I_CS, 3                 ; Called from user code?
                JNZ     SYSCALL_ERROR           ; Yes -> error
                MOV     RM_AX, 7F16H
                CALL    INT_RM
                RET

; ----------------------------------------------------------------------------
; AX=7F17H: __sleep()
;
; Suspend process
;
; In:   EDX     Seconds
;
; Out:  EAX     Remaining seconds
;
; ----------------------------------------------------------------------------

                TALIGN  4
SYS17:          MOV     EAX, I_EDX
                TEST    EAX, EAX
                JZ      SHORT SYS17_RET
                MOV     ECX, 91
                MUL     ECX                     ; Multiply by 18.2 = 91/5
                ADD     EAX, 4
                ADC     EDX, 0
                MOV     ECX, 5
                CMP     EDX, ECX                ; Overflow?
                JAE     SHORT SLEEP_OV
                DIV     ECX
SLEEP_DO:       MOV     BX, DI
                CALL    SLEEP
                MOV     ECX, 5
                MUL     ECX                     ; Divide by 18.2
                MOV     ECX, 91
                DIV     ECX
SYS17_RET:      MOV     I_EAX, EAX
                RET

SLEEP_OV:       MOV     EAX, MAX_32             ; Use maximum value
                JMP     SHORT SLEEP_DO

; ----------------------------------------------------------------------------
; AX=7F18H: __chsize()
;
; Change file size
;
; In:   EBX     File handle
;       EDX     File size
;
; Out:  CY      Error
;       EAX     errno (CY)
;
; ----------------------------------------------------------------------------

                TALIGN  4
SYS18:          MOV     EBX, I_EBX              ; File handle
                MOV     EDX, I_EDX              ; Distance
                MOV     AL, 0                   ; SEEK_SET
                MOV     AH, 42H                 ; Move file pointer
                INT     21H
                JC      SHORT SYS18_RET
                MOV     EBX, I_EBX              ; File handle
                MOV     ECX, 0                  ; Number of bytes
                MOV     EDX, 0                  ; Buffer
                MOV     AH, 40H                 ; Write handle
                INT     21H
                JC      SHORT SYS18_RET
                XOR     EAX, EAX
SYS18_RET:      MOV     I_EAX, EAX
                RET


; ----------------------------------------------------------------------------
; AX=7F19H: __fcntl()
;
; Unix-like file control
;
; In:   EBX     File handle
;       ECX     Request code
;       EDX     Argument
;
; Out:  EAX     Return value
;       ECX     errno
;
; ----------------------------------------------------------------------------

                TALIGN  4
                ASSUME  DI:PTR PROCESS
SYS19:          CMP     I_ECX, F_SETFL
                JE      SHORT FCNTL_SETFL
                CMP     I_ECX, F_SETFD
                JE      SHORT FCNTL_SETFD
                CMP     I_ECX, F_GETFD
                JE      SHORT FCNTL_GETFD
                CMP     I_ECX, F_GETFL
                JE      FCNTL_GETFL
EMX_EINVAL:     MOV     I_ECX, EINVAL
                MOV     I_EAX, -1
                RET

                TALIGN  4
FCNTL_GETFD:    MOV     BX, WORD PTR I_EBX
                CMP     BX, MAX_FILES
                JAE     EMX_EBADF
                XOR     EAX, EAX
                SHL     BX, 1
                TEST    [DI].P_HFLAGS[BX], HF_NOINHERIT
                JZ      SHORT GETFD_1
                OR      EAX, 1                  ; FD_CLOEXEC
GETFD_1:        MOV     I_EAX, EAX
                MOV     I_ECX, 0
                RET

                TALIGN  4
FCNTL_SETFD:    MOV     BX, WORD PTR I_EBX
                CMP     BX, MAX_FILES
                JAE     EMX_EBADF
                SHL     BX, 1
                AND     [DI].P_HFLAGS[BX], NOT HF_NOINHERIT
                TEST    I_EDX, 1                ; FD_CLOEXEC
                JZ      SHORT SETFD_1
                OR      [DI].P_HFLAGS[BX], HF_NOINHERIT
SETFD_1:        JMP     SHORT EMX_OK

                TALIGN  4
FCNTL_SETFL:    MOV     BX, WORD PTR I_EBX
                CMP     BX, MAX_FILES
                JAE     SHORT EMX_EBADF
                SHL     BX, 1
                MOV     EAX, I_EDX
                TEST    EAX, NOT (O_NDELAY OR O_APPEND)
                JNZ     SHORT EMX_EINVAL
                .ERRE   O_NDELAY EQ HF_NDELAY
                .ERRE   O_APPEND EQ HF_APPEND
                AND     [DI].P_HFLAGS[BX], NOT (HF_NDELAY OR HF_APPEND)
                OR      [DI].P_HFLAGS[BX], AX
                CMP     I_EBX, 0                ; stdin?
                JNE     SHORT EMX_OK
                MOV     STDIN_FL, EAX
                TALIGN  4
EMX_OK:         MOV     I_ECX, 0
                MOV     I_EAX, 0
                RET

                TALIGN  4
FCNTL_GETFL:    MOV     BX, WORD PTR I_EBX
                CMP     BX, MAX_FILES
                JAE     SHORT EMX_EBADF
                SHL     BX, 1
                XOR     EAX, EAX
                MOV     AX, [DI].P_HFLAGS[BX]
                .ERRE   O_NDELAY EQ HF_NDELAY
                .ERRE   O_APPEND EQ HF_APPEND
                AND     AX, HF_NDELAY OR HF_APPEND
                MOV     I_EAX, EAX
                MOV     I_ECX, 0
                RET

                TALIGN  4
EMX_EBADF:      MOV     I_ECX, EBADF
                MOV     I_EAX, -1
                RET
                ASSUME  DI:NOTHING

; ----------------------------------------------------------------------------
; AX=7F1AH: __pipe()
;
; Create unnamed pipe
;
; In:   ECX     Size of pipe
;       EDX     Pointer to storage for two handles (two DWORDs)
;
; Out:  EAX     0 (success) or -1 (failure)
;       ECX     errno
;
; ----------------------------------------------------------------------------

                TALIGN  4
SYS1A:          MOV     I_ECX, ENOSYS
                MOV     I_EAX, -1
                RET

; ----------------------------------------------------------------------------
; AX=7F1BH: __fsync()
;
; Update file system
;
; In:   EBX     File handle
;
; Out:  EAX     0 (success) or -1 (failure)
;       ECX     errno
;
; ----------------------------------------------------------------------------

                TALIGN  4
SYS1B:          MOV     I_ECX, ENOSYS
                MOV     I_EAX, -1
                RET

; ----------------------------------------------------------------------------
; AX=7F1CH: __fork()
;
; Duplicate process
;
;
; Out: EAX      Process ID or 0 (in new process) or -1 (failure)
;      ECX      errno (0 if successful)
;
; ----------------------------------------------------------------------------
                TALIGN  4
SYS1C:          MOV     I_ECX, ENOSYS
                MOV     I_EAX, -1
                RET

; ----------------------------------------------------------------------------
; AX=7F1DH: __scrsize()
;
; Get number of rows and columns
;
; In:   EDX     Pointer to structure
;
; Out:  --
;
; ----------------------------------------------------------------------------

                TALIGN  4
SYS1D:          MOV     EBX, I_EDX
                MOV     ES, I_DS
                MOVZX   EAX, VWIDTH
                MOV     DWORD PTR ES:[EBX+0], EAX
                MOVZX   EAX, VHEIGHT
                MOV     DWORD PTR ES:[EBX+4], EAX
                RET

; ----------------------------------------------------------------------------
; AX=7F1EH: __select()
;
; Synchronous I/O multiplexing
;
; In:   EDX     pointer to structure
;
; Out:  EAX     0 (timeout), > 0 (ready) or -1 (failure)
;       ECX     errno (0 if successful)
;
; ----------------------------------------------------------------------------

                TALIGN  4
                ASSUME  DI:PTR PROCESS
SYS1E:          AND     [DI].P_FLAGS, NOT PF_SLEEP_FLAG ; Clear counter flag
                MOV     SEL_COUNT, 0            ; No ready handles
                MOV     ESI, I_EDX              ; Get pointer to structure
                MOV     ES, I_DS                ; and segment register
                ASSUME  ESI:NEAR32 PTR SELECT_ARGS
                MOV     EBX, ES:[ESI].SELA_TIMEOUT ; Get timeout pointer
                TEST    EBX, EBX                ; NULL pointer?
                JZ      SHORT SELECT_LOOP       ; Yes -> indefinite wait
                ASSUME  EBX:NEAR32 PTR TIMEVAL
                MOV     EAX, ES:[EBX].TV_USEC   ; Get microseconds
                XOR     EDX, EDX
                MOV     ECX, 54945
                DIV     ECX                     ; Convert to timer ticks
                PUSH    EAX                     ; Save ticks for microseconds
                MOV     EAX, ES:[EBX].TV_SEC    ; Get seconds
                ASSUME  EBX:NOTHING
                MOV     ECX, 91
                MUL     ECX                     ; Multiply by 18.2 = 91/5
                ADD     EAX, 4
                ADC     EDX, 0
                MOV     ECX, 5
                CMP     EDX, ECX
                JAE     SHORT SELECT_OV1
                DIV     ECX
                POP     ECX                     ; Ticks for microseconds
                ADD     EAX, ECX                ; Total number of ticks
                JC      EMX_EINVAL              ; Overflow -> error
                JZ      SHORT SELECT_ONCE       ; Zero -> return after one loop
                MOV     DX, TT_SLEEP            ; Set `sleep' timer
                MOV     BX, DI                  ; Process table entry
                CALL    SET_TIMER
                JC      EMX_EINVAL              ; Table overflow -> error
                JMP     SHORT SELECT_LOOP       ; Start loop

SELECT_OV1:     POP     ECX                     ; Ticks for microseconds
                JMP     EMX_EINVAL              ; Error

SELECT_ONCE:    OR      [DI].P_FLAGS, PF_SLEEP_FLAG ; Return after first loop
                ASSUME  DI:NOTHING
                LEA     DI, SEL_RBITS           ; Clear all bits in the
                MOV_ES_DS                       ; bitmap of ready handles
                MOV     CX, FD_SETSIZE/8
                XOR     AL, AL
                REP     STOSB
                MOV     ES, I_DS
;
; This is the main loop
;
SELECT_LOOP:    MOV     BX, PROCESS_PTR
                ASSUME  BX:PTR PROCESS
                MOV     EAX, [BX].P_SIG_BLOCKED ; Compute set of pending,
                NOT     EAX                     ; unblocked signals
                AND     EAX, [BX].P_SIG_PENDING
                ASSUME  BX:NOTHING
;
; TODO: Deliver signals set to SIG_DFL now
; TODO: Ignore signals set to SIG_IGN
;
                JNZ     SELECT_INTR             ; Signal -> return EINTR
                MOV     EDI, ES:[ESI].SELA_READFDS ; Get readfds pointer
                TEST    EDI, EDI                ; NULL pointer?
                JZ      SELECT_TIME             ; Yes -> wait until timeout
                XOR     EBX, EBX                ; Start with handle 0
SELECT_POLL:    CMP     EBX, ES:[ESI].SELA_NFDS ; All handles checked?
                JAE     SELECT_CHECK            ; Yes -> any ready handles?
                BT      DWORD PTR ES:[EDI], EBX ; Handle in readfds bitmap?
                JNC     SELECT_NEXT             ; No  -> next handle
                CMP     EBX, 0                  ; stdin?
                JNZ     SHORT SELECT_TEST       ; No  -> check input status
                TEST    STDIN_TERMIO.C_LFLAG, IDEFAULT ; termio?
                JNZ     SHORT SELECT_TEST       ; No  -> check input status
                CALL    STDIN_AVAIL             ; Get number of available chars
                TEST    EAX, EAX                ; Any characters available?
                JNZ     SHORT SELECT_HIT        ; Yes -> handle ready
                JMP     SHORT SELECT_NEXT       ; Next handle

SELECT_TEST:    PUSH    EBX
                PUSH    ESI
                PUSH    EDI
                MOV     AX, 4406H               ; IOCTL: Check input status
                INT     21H
                POP     EDI
                POP     ESI
                POP     EBX
                JC      SELECT_ERR              ; Error -> done
                TEST    AL, AL                  ; 00H=not ready, 0FFH=ready
                JZ      SHORT SELECT_NEXT       ; Not ready -> next handle
SELECT_HIT:     INC     SEL_COUNT               ; Increment # of ready handles
                BTS     DWORD PTR SEL_RBITS, EBX ; Set bit in bitmap
SELECT_NEXT:    INC     EBX                     ; Next handle
                JMP     SELECT_POLL             ; Check next handle

;
; Having looped though the handles, check if a handle is ready
;
SELECT_CHECK:   CMP     SEL_COUNT, 0            ; Are there ready handles?
                JNE     SHORT SELECT_READY      ; Yes -> success
SELECT_TIME:    MOV     EBX, ES:[ESI].SELA_TIMEOUT ; Get pointer to timeout
                TEST    EBX, EBX                ; NULL pointer?
                JZ      SELECT_LOOP             ; Yes -> indefinite wait
                MOV     BX, PROCESS_PTR         ; Process table entry
                TEST    (PROCESS PTR [BX]).P_FLAGS, PF_SLEEP_FLAG ; Timer down?
                JZ      SELECT_LOOP             ; No  -> check handles again
;
; Time out, clear all sets
;
                MOV     EDI, ES:[ESI].SELA_READFDS
                CALL    CLEAR_FDSET
                MOV     EDI, ES:[ESI].SELA_WRITEFDS
                CALL    CLEAR_FDSET
                MOV     EDI, ES:[ESI].SELA_EXCEPTFDS
                CALL    CLEAR_FDSET
                MOV     I_EAX, 0                ; Time out
                MOV     I_ECX, 0                ; No error
                JMP     SELECT_RET              ; Done

;
; At least one handle is ready
;
SELECT_READY:   MOVZX   EAX, SEL_COUNT          ; Return number of ready
                MOV     I_EAX, EAX              ; handles
                MOV     I_ECX, 0                ; No error
                PUSH    ESI                     ; Save pointer to structure
                LEA     ESI, SEL_RBITS          ; Copy bitmap of ready handles
                MOV     ECX, FD_SETSIZE/8       ; to readfds
                REP     MOVS BYTE PTR ES:[EDI], BYTE PTR DS:[ESI]
                POP     ESI                     ; Restore pointer to structure
                MOV     EDI, ES:[ESI].SELA_WRITEFDS ; Get writefds pointer
                CALL    CLEAR_FDSET
                MOV     EDI, ES:[ESI].SELA_EXCEPTFDS ; Get exceptfds pointer
                CALL    CLEAR_FDSET
SELECT_RET:     RET                             ; Done

;
; A signal occured while waiting in select().  Return EINTR and raise
; the signal.
;
SELECT_INTR:    MOV     EAX, EINTR

SELECT_ERR:     MOV     I_ECX, EAX              ; Return errno in ECX
                MOV     I_EAX, -1               ; Error
                JMP     SHORT SELECT_RET        ; Done

;
; Clear a fd_set object pointed to by ES:EDI if EDI is not NULL
;
                ALIGN   4
CLEAR_FDSET     PROC    NEAR
                TEST    EDI, EDI
                JZ      SHORT FIN
                MOV     ECX, FD_SETSIZE/8
                XOR     AL, AL
                MOV     ECX, FD_SETSIZE/8
                REP     STOS BYTE PTR ES:[EDI]
FIN:            RET
CLEAR_FDSET     ENDP

                ASSUME  ESI:NOTHING

; ----------------------------------------------------------------------------
; AX=7F1FH: __syserrno()
;
; Return DOS error number for last syscall
;
; Out:  EAX     error number
;
; ----------------------------------------------------------------------------

                TALIGN  4
SYS1F:          MOV     I_EAX, 0
                RET

; ----------------------------------------------------------------------------
; AX=7F20H __stat()
;
; Get information about a path name
;
; In:  EDX      path name
;      EDI      pointer to structure
;
; Out: EAX      0 (ok), -1 (error)
;      ECX      errno (0 if successful)
;
; ----------------------------------------------------------------------------
                TALIGN  4
                ASSUME  EDI:NEAR32 PTR STAT
SYS20:          MOV     ESI, I_EDX              ; Pointer to path name
                MOV     ES, I_DS
;
; Return ENOENT if the path name contains wildcard characters
;
                TALIGN  4
NSTAT_WILD:     LODS    BYTE PTR ES:[ESI]
                TEST    AL, AL
                JZ      SHORT NSTAT_NOWILD
                CMP     AL, "?"
                JE      SHORT NSTAT_FAILURE1
                CMP     AL, "*"
                JNE     SHORT NSTAT_WILD
NSTAT_FAILURE1: JMP     NSTAT_FAILURE

NSTAT_NOWILD:   MOV     EDX, I_EDX              ; Pointer to path name
                MOV     ESI, I_ESP              ; Allocate FIND_BUF in user
                SUB     ESI, SIZE FIND_BUF      ; stack (same segment as path!)
                ASSUME  ESI:NEAR32 PTR FIND_BUF
                PUSH    DS
                MOV     DS, I_DS
                MOV     CX, 16H                 ; Dir & system & hidden
                MOV     AH, 4EH                 ; Find first file
                INT     21H
                POP     DS
                JC      NSTAT_10                ; Error -> root directory?
                MOV     EDI, I_EDI              ; Pointer to structure
                MOV     ES, I_DS                ; Use ES for user data segment
                MOV     EAX, S_IFREG            ; File
                TEST    ES:[ESI].FIND_ATTR, 10H ; Is it a directory?
                JZ      SHORT NSTAT_1           ; No  -> it's a file
                MOV     ES:[ESI].FIND_SIZE, 0   ; The size of a directory is 0
                MOV     EAX, S_IFDIR OR ((S_IREAD+S_IWRITE+S_IEXEC) SHR 6) * 111Q
                JMP     SHORT NSTAT_2
                TALIGN  4
NSTAT_1:        OR      EAX, (S_IREAD SHR 6) * 111Q ; Read-only access
                TEST    ES:[ESI].FIND_ATTR, 01H ; Ready-only attribute set?
                JNZ     SHORT NSTAT_2           ; Yes -> skip
                OR      EAX, (S_IWRITE SHR 6) * 111Q ; Read-write access
NSTAT_2:        MOV     ES:[EDI].ST_MODE, EAX   ; Set mode
                MOVZX   EAX, ES:[ESI].FIND_ATTR ; Get attributes
                MOV     ES:[EDI].ST_ATTR, EAX   ; and store them
                MOV     EAX, ES:[ESI].FIND_SIZE ; Get size
                MOV     ES:[EDI].ST_SIZE, EAX   ; and store it
                MOV     CX, ES:[ESI].FIND_TIME  ; Get time
                MOV     DX, ES:[ESI].FIND_DATE  ; and date
                CALL    PACKED_2_UNIX           ; and convert them to time_t
NSTAT_MORE:     MOV     ES:[EDI].ST_ATIME, EAX  ; Time of last access
                MOV     ES:[EDI].ST_MTIME, EAX  ; Time of last modification
                MOV     ES:[EDI].ST_CTIME, EAX  ; Time of creation
                MOV     ES:[EDI].ST_NLINK, 1    ; One link
                MOV     ES:[EDI].ST_UID, 0      ; root
                MOV     ES:[EDI].ST_GID, 0      ; root
                MOV     ES:[EDI].ST_DEV, 0      ; Device numbers not supported
                MOV     ES:[EDI].ST_RDEV, 0     ; Device numbers not supported
                MOV     ES:[EDI].ST_RESERVED, 0 ; Nomen est omen
                MOV     EAX, INO_NUMBER         ; Mock inode number
                MOV     ES:[EDI].ST_INO, EAX
                INC     INO_NUMBER
                JNZ     SHORT NSTAT_INO_1
                INC     INO_NUMBER
NSTAT_INO_1:    JMP     EMX_OK                  ; Done

;
; Not found -- check for root directory
;
; Get current working directory and save it. Then try to change to the
; "directory" given as argument to __stat(). If successful, it's the
; root directory: set the structure appropriately and restore the
; previous working directory. Otherwise, the file/directory does not
; exist (or it's a device -- which is not supported by this implementation
; of stat).
;
                TALIGN  4
NSTAT_10:       MOV     EBX, I_EDX
                MOV     ES, I_DS
                MOV     AL, ES:[EBX]
                CALL    UPPER
                CMP     AL, "A"
                JB      SHORT NSTAT_11
                CMP     AL, "Z"
                JA      SHORT NSTAT_11
                CMP     BYTE PTR ES:[EBX+1], ":"
                JE      SHORT NSTAT_12
NSTAT_11:       MOV     AH, 19H
                INT     21H
                ADD     AL, "A"
NSTAT_12:       MOV     STAT_CWD, AL
                SUB     AL, "A" - 1
                MOV     DL, AL
                LEA     ESI, STAT_CWD + 3
                MOV     AH, 47H
                INT     21H
                JC      SHORT NSTAT_FAILURE
                PUSH    DS
                MOV     EDX, I_EDX
                MOV     DS, I_DS
                MOV     AH, 3BH
                INT     21H
                POP     DS
                JC      SHORT NSTAT_FAILURE
                LEA     EDX, STAT_CWD
                MOV     AH, 3BH
                INT     21H
                MOV     EDI, I_EDI
                MOV     ES, I_DS
                MOV     ES:[EDI].ST_MODE, S_IFDIR+((S_IREAD+S_IWRITE+S_IEXEC) SHR 6)*111Q
                MOV     ES:[EDI].ST_ATTR, 0     ; Look here to find out about
                                                ; root directory (not 10H!)
                MOV     ES:[EDI].ST_SIZE, 0
                XOR     EAX, EAX
                JMP     NSTAT_MORE

                TALIGN  4
NSTAT_FAILURE:  MOV     I_EAX, -1
                MOV     I_ECX, ENOENT
                RET

                ASSUME  ESI:NOTHING
                ASSUME  EDI:NOTHING

; ----------------------------------------------------------------------------
; AX=7F21H __fstat()
;
; Get information about an open file
;
; In:  EBX      file handle
;      EDI      pointer to structure
;
; Out: EAX      0 (ok), -1 (error)
;      ECX      errno (0 if successful)
;
; ----------------------------------------------------------------------------
                TALIGN  4
                ASSUME  EDI:NEAR32 PTR STAT
SYS21:          MOV     EBX, I_EBX
                MOV     AX, 4400H
                INT     21H
                JC      FSTAT_ERROR
                MOV     EDI, I_EDI
                MOV     ES, I_DS
                TEST    DL, 80H                 ; Device?
                JNZ     FSTAT_DEV
                MOV     ES:[EDI].ST_MODE, S_IFREG
                MOV     ES:[EDI].ST_ATTR, 0
                MOV     ES:[EDI].ST_RESERVED, 0
                MOV     AX, 5700H
                INT     21H
                JC      FSTAT_ERROR
                CALL    PACKED_2_UNIX
                MOV     ES:[EDI].ST_ATIME, EAX
                MOV     ES:[EDI].ST_MTIME, EAX
                MOV     ES:[EDI].ST_CTIME, EAX
                MOV     AX, 4201H
                MOV     EDX, 0
                INT     21H
                JC      FSTAT_ERROR
                MOV     ESI, EAX
                MOV     AX, 4202H
                MOV     EDX, 0
                INT     21H
                JC      FSTAT_ERROR
                MOV     ES:[EDI].ST_SIZE, EAX
                MOV     AX, 4200H
                MOV     EDX, ESI
                INT     21H
                JC      FSTAT_ERROR
                JMP     SHORT FSTAT_MORE

                TALIGN  4
FSTAT_DEV:      MOV     ES:[EDI].ST_MODE, S_IFCHR
                MOV     ES:[EDI].ST_SIZE, 0
                MOV     ES:[EDI].ST_ATIME, 0
                MOV     ES:[EDI].ST_MTIME, 0
                MOV     ES:[EDI].ST_CTIME, 0
FSTAT_MORE:     MOV     EAX, INO_NUMBER
                MOV     ES:[EDI].ST_INO, EAX
                INC     INO_NUMBER
                JNZ     SHORT FSTAT_INO_1
                INC     INO_NUMBER
FSTAT_INO_1:    MOV     ES:[EDI].ST_UID, 0
                MOV     ES:[EDI].ST_GID, 0
                MOV     ES:[EDI].ST_NLINK, 1
                MOV     ES:[EDI].ST_DEV, 0
                MOV     ES:[EDI].ST_RDEV, 0
                OR      ES:[EDI].ST_MODE, ((S_IREAD+S_IWRITE) SHR 6) * 111Q
                JMP     EMX_OK

FSTAT_ERROR:    MOV     I_EAX, -1
                MOV     I_ECX, EAX
                RET

                ASSUME  EDI:NOTHING


; ----------------------------------------------------------------------------
; AX=7F23H __filesys()
;
; Get name of file-system driver
;
; In:  EDX      pointer to drive name
;      EDI      pointer to output buffer
;      ECX      size of output buffer
;
; Out: EAX      0 (ok) or -1 (error)
;      ECX      errno (0 if successful)
;
; ----------------------------------------------------------------------------
                TALIGN  4
SYS23:          MOV     EBX, I_EDX
                MOV     ES, I_DS
                MOV     AL, ES:[EBX+0]
                CALL    UPPER
                CMP     AL, "A"
                JB      SHORT FILESYS_INVALID
                CMP     AL, "Z"
                JA      SHORT FILESYS_INVALID
                CMP     BYTE PTR ES:[EBX+1], ":"
                JNE     SHORT FILESYS_INVALID
                CMP     BYTE PTR ES:[EBX+2], 0
                JNE     SHORT FILESYS_INVALID
                CMP     I_ECX, 4
                JB      SHORT FILESYS_2BIG
                SUB     AL, "A"-1
                MOV     BL, AL
                MOV     AX, 4409H
                INT     21H
                JC      SHORT FILESYS_ERROR
                LEA     ESI, $FAT
                TEST    DX, 1 SHL 12
                JZ      SHORT FILESYS_1
                LEA     ESI, $LAN
FILESYS_1:      MOV     ECX, 4
                MOV     EDI, I_EDI
                REP     MOVS BYTE PTR ES:[EDI], BYTE PTR DS:[ESI]
                JMP     EMX_OK

FILESYS_2BIG:   MOV     EAX, E2BIG
                JMP     SHORT FILESYS_ERROR

FILESYS_INVALID:MOV     EAX, EINVAL
FILESYS_ERROR:  MOV     I_EAX, -1
                MOV     I_ECX, EAX
                RET

; ----------------------------------------------------------------------------
; AX=7F24H __utimes()
;
; Set access and modification time of a file
;
; In:  EDX      pointer to path name
;      ESI      pointer to array of structures
;
; Out: EAX      0 (ok) or -1 (error)
;      ECX      errno (0 if successful)
;
; ----------------------------------------------------------------------------
                TALIGN  4
SYS24:          PUSH    PROCESS_PTR
                MOV     PROCESS_PTR, NO_PROCESS
                PUSH    DS
                MOV     EDX, I_EDX
                MOV     DS, I_DS
                ASSUME  DS:NOTHING
                MOV     AX, 3D11H
                INT     21H
                POP     DS
                ASSUME  DS:SV_DATA
                JC      SHORT UTIMES_ERROR
                MOV     EBX, EAX
                MOV     ESI, I_ESI
                MOV     ES, I_DS
                MOV     EAX, ES:[ESI+8]         ; Modification time
                CALL    UNIX_2_PACKED
                MOV     AX, 5701H
                INT     21H
                JC      SHORT UTIMES_1
                MOV     AH, 3EH
                INT     21H
                JC      SHORT UTIMES_ERROR
                MOV     I_EAX, 0
                MOV     I_ECX, 0
UTIMES_RET:     POP     PROCESS_PTR
                RET

UTIMES_1:       PUSH    EAX
                MOV     AH, 3EH
                INT     21H
                POP     EAX
UTIMES_ERROR:   MOV     I_EAX, -1
                MOV     I_ECX, EAX
                JMP     SHORT UTIMES_RET


; ----------------------------------------------------------------------------
; AX=7F25H: __ftruncate()
;
; Truncate a file
;
; In:   EBX     File handle
;       EDX     File size
;
; Out:  EAX     0 (ok), -1 (error)
;       ECX     errno
;
; ----------------------------------------------------------------------------

                TALIGN  4
SYS25:          MOV     EBX, I_EBX              ; File handle
                MOV     EDX, 0                  ; Distance
                MOV     AL, 2                   ; SEEK_END
                MOV     AH, 42H                 ; Move file pointer
                INT     21H
                JC      SHORT SYS25_ERR         ; Error ->
                CMP     I_EDX, EAX              ; Truncating?
                JNB     EMX_OK                  ; No  -> ignore request, OK
                MOV     EBX, I_EBX              ; File handle
                MOV     EDX, I_EDX              ; Distance
                MOV     AL, 0                   ; SEEK_SET
                MOV     AH, 42H                 ; Move file pointer
                INT     21H
                JC      SHORT SYS25_ERR         ; Error ->
                MOV     EBX, I_EBX              ; File handle
                MOV     ECX, 0                  ; Number of bytes
                MOV     EDX, 0                  ; Buffer
                MOV     AH, 40H                 ; Write handle
                INT     21H
                JNC     EMX_OK
SYS25_ERR:      MOV     I_ECX, EAX
                MOV     I_EAX, -1
                RET

; ----------------------------------------------------------------------------
; AX=7F26H: __clock()
;
; Processor time
;
; Out:  EAX     timer ticks of processor time used, low-order 32 bits
;       EDX     high-order 32 bits
;
; ----------------------------------------------------------------------------
                TALIGN  4
SYS26:          MOV     EAX, CLOCK_HI
                MOV     EBX, CLOCK_LO
                CMP     EAX, CLOCK_HI
                JNE     SHORT SYS26
                MOV     ECX, 500
                MUL     ECX
                XCHG    EAX, EBX
                MUL     ECX                     ; Multiply by 100 / 18.2
                ADD     EDX, EBX
                PUSH    EAX
                MOV     EAX, EDX
                XOR     EDX, EDX
                MOV     ECX, 91
                DIV     ECX
                MOV     I_EDX, EAX
                POP     EAX
                DIV     ECX
                MOV     I_EAX, EAX
                RET

; ----------------------------------------------------------------------------
; AX=7F27H: __ftime()
;
; Get current time
;
; In:  EDX     pointer to structure
;
; ----------------------------------------------------------------------------
                TALIGN  4
SYS27:          MOV     EDI, I_EDX
                MOV     ES, I_DS
                CALL    DO_FTIME
                RET


; ----------------------------------------------------------------------------
; AX=7F28H: __umask()
;
; Set file permission mask
;
; In:  EDX      file permission mask
;
; Out: EAX      previous file permission mask
;
; ----------------------------------------------------------------------------
                TALIGN  4
                ASSUME  DI:PTR PROCESS
SYS28:          MOV     AX, WORD PTR I_EDX
                XCHG    AX, [DI].P_UMASK
                MOVZX   EAX, AX
                MOV     I_EAX, EAX
                RET

                ASSUME  DI:NOTHING

; ----------------------------------------------------------------------------
; AX=7F29H: __getppid()
;
; Get parent process ID
;
; Out: EAX      parent process ID
;
; ----------------------------------------------------------------------------
                TALIGN  4
                ASSUME  DI:PTR PROCESS
SYS29:          MOV     EAX, [DI].P_PPID
                MOV     I_EAX, EAX
                RET

                ASSUME  DI:NOTHING

; ----------------------------------------------------------------------------
; AX=7F2AH: __nls_memupr()
;
; Convert buffer to upper case
;
; In:  EDX      pointer to buffer
;      ECX      size of buffer
;
; ----------------------------------------------------------------------------
                TALIGN  4
SYS2A:          MOV     AX, I_DS
                MOV     ESI, I_EDX
                MOV     EDI, OFFSET_1
                MOV     ECX, I_ECX
                MOV     RM_DX, DI               ; Offset of buffer
                MOV     RM_CX, CX
                CALL    MOVE_TO_RM
                MOV     RM_AX, 7F2AH
                CALL    INT_RM
                MOV     AX, I_DS
                MOV     EDI, I_EDX
                MOV     ECX, I_ECX
                CALL    MOVE_FROM_RM
                RET



; ----------------------------------------------------------------------------
; AX=7F2BH: __open
;
; Open file
;
; In:  EDX      pointer to file name
;      ECX      flags
;
; Out: EAX      handle or -1
;      ECX      errno (0 if successful)
;
; ----------------------------------------------------------------------------
;
; Replace "/dev/null" with "nul"
;
SYS2B_NUL:      MOV     I_EDX, OFFSET $NUL      ; Use "nul" instead
                MOV     SI, DS
                JMP     SHORT SYS2B_1

;
; Replace "/dev/tty" with "con"
;
SYS2B_CON:      MOV     I_EDX, OFFSET $CON      ; Use "con" instead
                MOV     SI, DS
                JMP     SHORT SYS2B_1

;
; __open()
;
                TALIGN  4
SYS2B:          LEA     ESI, $DEVNULL
                MOV     EDI, I_EDX
                MOV     ES, I_DS
                CALL    STRCMP                  ; "/dev/null"?
                JE      SHORT SYS2B_NUL         ; Yes -> use "nul"
                LEA     ESI, $DEVTTY
                MOV     EDI, I_EDX
                CALL    STRCMP                  ; "/dev/tty"?
                JE      SHORT SYS2B_CON         ; Yes -> use "con"
                MOV     SI, I_DS
SYS2B_1:        MOV     EDX, I_EDX              ; File name
                MOV     AL, BYTE PTR I_ECX[0]   ; Open mode
                MOV     AH, 3DH
                PUSH    DS
                MOV     DS, SI
                INT     21H
                POP     DS
                JC      SHORT SYS2B_FAILED
;
; File exists
;
                MOV     EDX, I_ECX
                AND     EDX, 30000H             ; O_CREAT|O_EXCL
                CMP     EDX, 30000H             ; O_CREAT and O_EXCL set?
                JE      SYS2B_EXFAIL            ; Yes -> fail
                MOV     I_EAX, EAX              ; Save the handle
SYS2B_OK:       TEST    I_ECX, 40000H           ; O_TRUNC set?
                JZ      SHORT SYS2B_SUCCESS     ; No  -> don't truncate file
                MOV     EBX, EAX                ; File handle
                XOR     EDX, EDX                ; Distance = 0
                MOV     AL, 0                   ; SEEK_SET
                MOV     AH, 42H                 ; Move file pointer
                INT     21H
                JC      SHORT SYS2B_SUCCESS     ; Ignore error (device!)
                MOV     EBX, I_EAX              ; File handle
                MOV     ECX, 0                  ; Number of bytes
                MOV     EDX, 0                  ; Buffer
                MOV     AH, 40H                 ; Write handle
                INT     21H                     ; Ignore error (device!)
;
; Opening the file succeeded.  Set the handle flags.
;
SYS2B_SUCCESS:  TEST    I_ECX, 80000H           ; O_NOINHERIT?
                JZ      SHORT SYS2B_RET
                MOV     BX, PROCESS_PTR
                CMP     BX, NO_PROCESS
                JE      SHORT SYS2B_RET
                ASSUME  BX:PTR PROCESS
                MOV     SI, WORD PTR I_EAX
                SHL     SI, 1
                OR      [BX].P_HFLAGS[SI], HF_NOINHERIT
                ASSUME  BX:NOTHING
SYS2B_RET:      MOV     I_ECX, 0                ; Success
                RET

;
; Opening an existing file failed.
;
SYS2B_FAILED:   CMP     EAX, ENOENT             ; No such file?
                JNE     SHORT SYS2B_ERR         ; No  -> fail
                TEST    I_ECX, 10000H           ; O_CREAT?
                JZ      SHORT SYS2B_ERR         ; No  -> fail
;
; Try to create the file
;
                MOV     EDX, I_EDX              ; File name
                MOVZX   CX, BYTE PTR I_ECX[1]
                MOV     AH, 3CH                 ; umask is applied by
                PUSH    DS                      ; function 3CH
                MOV     DS, SI
                INT     21H
                POP     DS
                JC      SHORT SYS2B_ERR
                MOV     I_EAX, EAX              ; Save the handle
                JMP     SHORT SYS2B_SUCCESS     ; Done

SYS2B_ERR:      MOV     I_ECX, EAX
                MOV     I_EAX, -1
                RET

SYS2B_EXFAIL:   MOV     EBX, EAX                ; File handle
                MOV     AH, 3EH                 ; Close handle
                INT     21H
                MOV     EAX, EEXIST
                JMP     SHORT SYS2B_ERR

; ----------------------------------------------------------------------------
; AX=7F2CH: __newthread
;
; Notify emx of new thread
;
; In:  EDX      Thread ID
;
; ----------------------------------------------------------------------------

                TALIGN  4
SYS2C:          MOV     I_ECX, ENOSYS
                MOV     I_EAX, -1
                RET

; ----------------------------------------------------------------------------
; AX=7F2DH: __endthread
;
; Notify emx of end of thread
;
; In:  EDX      Thread ID
;
; ----------------------------------------------------------------------------

                TALIGN  4
SYS2D:          MOV     I_ECX, ENOSYS
                MOV     I_EAX, -1
                RET

; ----------------------------------------------------------------------------
; AX=7F2EH: __waitpid()
;
; Wait for child process
;
; In:   EDX     Process ID
;       ECX     Options
;
; Out:  EAX     Process ID of child process (-1 if no children)
;       ECX     errno
;       EDX     Termination status
;
; ----------------------------------------------------------------------------

                TALIGN  4
SYS2E:          MOV     I_ECX, ENOSYS
                MOV     I_EDX, 0
                MOV     I_EAX, -1
                RET

; ----------------------------------------------------------------------------
; AX=7F2FH: __read_kbd()
;
; Keyboard input
;
; In:   EDX     flags (bit 0: echo, bit 1: wait, bit 2:sig)
;
; Out:  EAX     Character (or -1)
;
; ----------------------------------------------------------------------------

RK_ECHO         =       1
RK_WAIT         =       2
RK_SIG          =       4

                TALIGN  4
SYS2F:          TEST    I_EDX, RK_SIG
                JNZ     SHORT RKBD_SIG
                TEST    I_EDX, RK_WAIT
                JNZ     SHORT RKBD_WAIT
                MOV     DL, 0FFH
                MOV     AH, 06H
                INT     21H
                JZ      SHORT RKBD_NOTHING
                MOVZX   EAX, AL
RKBD_ECHO:      TEST    I_EDX, RK_ECHO
                JZ      RKBD_RET
                PUSH    EAX
                MOV     DL, AL
                MOV     AH, 06H
                INT     21H
                POP     EAX
RKBD_RET:       MOV     I_EAX, EAX
                RET

RKBD_WAIT:      MOV     AH, 07H
                INT     21H
                MOVZX   EAX, AL
                JMP     SHORT RKBD_ECHO

RKBD_SIG:       TEST    I_EDX, RK_WAIT
                JNZ     SHORT RKBD_SIG_WAIT
                MOV     AH, 0BH
                INT     21H
                OR      AL, AL
                JZ      SHORT RKBD_NOTHING
RKBD_SIG_WAIT:  TEST    I_EDX, RK_ECHO
                JNZ     SHORT RKBD_SIG_ECHO
                MOV     AH, 08H
                INT     21H
                MOVZX   EAX, AL
                JMP     SHORT RKBD_RET

RKBD_SIG_ECHO:  MOV     AH, 01H
                INT     21H
                MOVZX   EAX, AL
                JMP     SHORT RKBD_RET

RKBD_NOTHING:   MOV     EAX, -1
                JMP     SHORT RKBD_RET


; ----------------------------------------------------------------------------
; AX=7F30H: __sleep2()
;
; Suspend process
;
; In:   EDX     Milliseconds
;
; Out:  EAX     0
;
; ----------------------------------------------------------------------------

                TALIGN  4
SYS30:          MOV     I_EAX, 0                ; Return value
                MOV     EAX, I_EDX
                OR      EAX, EAX
                JZ      SHORT SYS30_RET
                MOV     ECX, 91
                MUL     ECX                     ; Multiply by 0.0182 = 91/5000
                ADD     EAX, 4999
                ADC     EDX, 0
                MOV     ECX, 5000
                DIV     ECX
                MOV     BX, DI
                CALL    SLEEP                   ; Suspend process
SYS30_RET:      RET


; ----------------------------------------------------------------------------
; AX=7F31H: __unwind2()
;
; Unwind signal handlers for longjmp()
;
; Currently not used by the DOS version of emx.
;
; ----------------------------------------------------------------------------

                TALIGN  4
SYS31:          RET


; ----------------------------------------------------------------------------
; AX=7F32H: __pause()
;
; Wait for signal
;
; Currently not implemented
;
; ----------------------------------------------------------------------------

                TALIGN  4
SYS32:          RET


; ----------------------------------------------------------------------------
; AX=7F33H: __execname()
;
; Get the name of the executable file
;
; In:   EDX     Buffer
;       ECX     Buffer size
;
; Out:  EAX     0 if successful, -1 otherwise
;
; ----------------------------------------------------------------------------
                TALIGN  4
SYS33:          MOV     I_EAX, -1
                RET


; ----------------------------------------------------------------------------
; AX=7F34H: __initthread()
;
; Install exception handler in new thread
;
; In:   EDX     Pointer to EXCEPTIONREGISTRATIONRECORD
;
; Out:  EAX     0 if successful, -1 otherwise
;
; ----------------------------------------------------------------------------
                TALIGN  4
SYS34:          MOV     I_EAX, -1
                RET


; ----------------------------------------------------------------------------
; AX=7F35H: __sigaction()
;
; Examine or specify action for a signal
;
; In:   ECX     Signal number
;       EDX     Pointer to sigaction structure (input)
;       EBX     Pointer to sigaction structure (output)
;
; Out:  EAX     0 if successful, -1 otherwise
;       ECX     errno (0 if no error)
;
; ----------------------------------------------------------------------------
                TALIGN  4
                ASSUME  DI:PTR PROCESS
SYS35:          MOV     EAX, I_ECX
                MOV     EDX, I_EDX
                MOV     EBX, I_EBX
                MOV     ES, I_DS
                CALL    DO_SIGACTION
                MOV     I_ECX, EAX
                MOV     I_EAX, 0
                TEST    EAX, EAX
                JZ      SHORT SYS35_RET
                MOV     I_EAX, -1
SYS35_RET:      RET


; ----------------------------------------------------------------------------
; AX=7F36H: __sigpending()
;
; Query set of pending signals
;
; In:   EDX     Pointer to sigset_t (output)
;
; Out:  EAX     0 if successful, -1 otherwise
;       ECX     errno (0 if no error)
;
; ----------------------------------------------------------------------------
                TALIGN  4
                ASSUME  DI:PTR PROCESS
SYS36:          MOV     EAX, [DI].P_SIG_BLOCKED
                AND     EAX, [DI].P_SIG_PENDING
                SHR     EAX, 1
                MOV     EBX, I_EDX
                MOV     ES, I_DS
                MOV     ES:[EBX], EAX
                MOV     I_EAX, 0
                MOV     I_ECX, 0
                RET


; ----------------------------------------------------------------------------
; AX=7F37H: __sigprocmask()
;
; Examine or change the signal mask
;
; In:   ECX     Manner in which to set the mask
;       EDX     Pointer to sigset_t (input)
;       EBX     Pointer to sigset_t (output)
;
; Out:  EAX     0 if successful, -1 otherwise
;       ECX     errno (0 if no error)
;
; ----------------------------------------------------------------------------
                TALIGN  4
                ASSUME  DI:PTR PROCESS
SYS37:          PUSH    [DI].P_SIG_BLOCKED
                MOV     EBX, I_EDX
                TEST    EBX, EBX
                JZ      SHORT SPM_NO_INPUT
                MOV     ES, I_DS
                MOV     EAX, ES:[EBX]
                SHL     EAX, 1
                CMP     I_ECX, SIG_BLOCK
                JE      SHORT SPM_BLOCK
                CMP     I_ECX, SIG_UNBLOCK
                JE      SHORT SPM_UNBLOCK
                CMP     I_ECX, SIG_SETMASK
                JE      SHORT SPM_SETMASK
                POP     EAX
                MOV     I_EAX, -1
                MOV     I_ECX, EINVAL
                RET

SPM_UNBLOCK:    NOT     EAX
                AND     EAX, [DI].P_SIG_BLOCKED
                JMP     SHORT SPM_SETMASK

SPM_BLOCK:      OR      EAX, [DI].P_SIG_BLOCKED
SPM_SETMASK:    AND     EAX, SIG_BLOCK_MASK
                MOV     [DI].P_SIG_BLOCKED, EAX
SPM_NO_INPUT:   POP     EAX
                MOV     EBX, I_EBX
                TEST    EBX, EBX
                JZ      SHORT SPM_NO_OUTPUT
                SHR     EAX, 1
                MOV     ES:[EBX], EAX
SPM_NO_OUTPUT:  MOV     I_EAX, 0
                MOV     I_ECX, 0
                RET


; ----------------------------------------------------------------------------
; AX=7F38H: __sigsuspend()
;
; Replace signal mask and wait for signal
;
; In:   EDX     Pointer to sigset_t (input)
;
; Out:  EAX     0 if successful, -1 otherwise
;       ECX     errno (0 if no error)
;
; Currently not implemented
;
; ----------------------------------------------------------------------------
                TALIGN  4
                ASSUME  DI:PTR PROCESS
SYS38:          MOV     I_EAX, -1
                MOV     I_ECX, ENOSYS
                RET

; ----------------------------------------------------------------------------
; AX=7F39H: __imphandle()
;
; Import a file handle
;
; In:   EDX     File handle
;
; Out:  EAX     Relocated file handle (success) or -1 (failure)
;       ECX     errno (0 if no error)
;
; ----------------------------------------------------------------------------

                TALIGN  4
SYS39:          MOV     I_ECX, ENOSYS
                MOV     I_EAX, -1
                RET

; ----------------------------------------------------------------------------
; AX=7F3AH: __fpuemu()
;
; Interface for floating point emulator
;
; In:   ECX     Command code
;       EDX     Pointer to communication area
;
; Out:  EAX     Relocated file handle (success) or -1 (failure)
;       ECX     errno (0 if no error)
;
; ----------------------------------------------------------------------------
                TALIGN  4
                ASSUME  DI:PTR PROCESS
SYS3A:          CMP     I_ECX, FPUC_INIT
                JE      SHORT FPUEMU_INIT
                CMP     PROCESS_FPUEMU, DI      ; Correct process?
                JNE     FPUEMU_FAIL             ; No  -> fail
                CMP     I_ECX, FPUC_NEXT
                JE      SHORT FPUEMU_NEXT       ; Yes -> wait for notification
                CMP     I_ECX, FPUC_SIGNAL
                JE      FPUEMU_SIGNAL

;
; The FP emulator waits for the next notification; switch back to the
; interrupted process
;
FPUEMU_NEXT:    CMP     FPUEMU_STATE, FES_ON
                JNE     SHORT FPUEMU_NEXT_1
                MOV     AX, 0
                MOV     FS, AX                          ; Avoid trap on POP FS
                MOV     [DI].P_STATUS, PS_FPUEMU_NEXT
                MOV     BX, DI
                CALL    SAVE_PROCESS
                MOV     DI, PROCESS_FPUCLIENT
                ASSUME  DI:PTR PROCESS
                MOV     PROCESS_PTR, DI
                MOV     ES, I_DS
                MOV     ESI, I_EDX
                ASSUME  ESI:NEAR32 PTR FPUEMU_COM
                MOV     CX, I_REG_DWORDS
                XOR     EBX, EBX
FEN_LOOP:       MOV     EAX, ES:[ESI].FEC_FRAME[EBX]
                MOV     DWORD PTR [DI].P_GS[BX], EAX
                ADD     BX, 4
                LOOP    FEN_LOOP
                MOV     BX, DI
                CALL    REST_PROCESS
;;;;            CALL    BREAK_AFTER_IRET
                ASSUME  DI:NOTHING
                RET

FPUEMU_INIT:    CMP     PROCESS_FPUEMU, NO_PROCESS ; FPU emu already installed?
                JNE     FPUEMU_FAIL
                CMP     FPUEMU_STATE, FES_LOADING
                JNE     FPUEMU_FAIL
                MOV     PROCESS_FPUEMU, DI
                MOV     FPUEMU_STATE, FES_INIT1
                JMP     EMX_OK

                ASSUME  DI:PTR PROCESS
FPUEMU_NEXT_1:  CMP     FPUEMU_STATE, FES_ENDPROC
                JE      FPUEMU_ENDPROC_1
                CMP     FPUEMU_STATE, FES_NEWPROC
                JE      SHORT FPUEMU_NEWPROC_1
                CMP     FPUEMU_STATE, FES_INIT1
                JNE     FPUEMU_EXIT               ; TODO
;
; Send FPUN_NEWPROC notifications for all processes
;
                LEA     BX, PROCESS_TABLE
                ASSUME  BX:PTR PROCESS
                MOV     CX, MAX_PROCESSES
NEWPROC_LOOP:   TEST    [BX].P_FLAGS, PF_FPUEMU_INIT
                JNZ     SHORT NEWPROC_NEXT
                CMP     [BX].P_STATUS, PS_NONE
                JE      SHORT NEWPROC_NEXT
                CMP     [BX].P_STATUS, PS_DEFUNCT
                JE      SHORT NEWPROC_NEXT
                CMP     [BX].P_STATUS, PS_INIT
                JE      SHORT NEWPROC_NEXT
                CMP     BX, PROCESS_FPUEMU
                JNE     SHORT NEWPROC_SEND
NEWPROC_NEXT:   ADD     BX, SIZE PROCESS
                LOOP    NEWPROC_LOOP
                ASSUME  BX:NOTHING
;
; All processes have been processed, send FPUN_EMU notification
;
FPUEMU_NEXT_INIT2:
                MOV     FPUEMU_STATE, FES_ON
                MOV     BX, PROCESS_PTR
                CALL    SAVE_PROCESS
                MOV     BX, PROCESS_FPUCLIENT
                MOV     PROCESS_PTR, BX
                CALL    REST_PROCESS
                CALL    FPUEMU_CALL
                JMP     EMX_OK

;
; Send a FPUN_NEWPROC notification
;
                ASSUME  BX:PTR PROCESS
NEWPROC_SEND:   OR      [BX].P_FLAGS, PF_FPUEMU_INIT
                MOV     ES, I_DS
                MOV     EDI, I_EDX
                ASSUME  EDI:NEAR32 PTR FPUEMU_COM
                MOV     ES:[EDI].FEC_NOTIFY, FPUN_NEWPROC
                MOV     EAX, [BX].P_NUMBER
                MOV     ES:[EDI].FEC_PNUM, EAX
                JMP     EMX_OK

                ASSUME  EDI:NOTHING
                ASSUME  BX:NOTHING

;
; FPUN_ENDPROC and FPUN_NEWPROC handled
;
                ASSUME  DI:PTR PROCESS
FPUEMU_ENDPROC_1:
FPUEMU_NEWPROC_1:
                MOV     FPUEMU_STATE, FES_ON
                MOV     [DI].P_STATUS, PS_FPUEMU_NEXT
                MOV     BX, DI
                CALL    SAVE_PROCESS
                MOV     BX, PROCESS_FPUCLIENT
                MOV     PROCESS_PTR, BX
                CALL    REST_PROCESS
                RET

;
; Generate a signal.  Faking an exception would be better, but this
; will do for now.
;
FPUEMU_SIGNAL:  MOV     ES, I_DS
                MOV     ESI, I_EDX
                ASSUME  ESI:NEAR32 PTR FPUEMU_COM
                MOV     EAX, ES:[ESI].FEC_SIGNAL
                MOV     BX, PROCESS_FPUCLIENT
                BTS     (PROCESS PTR [BX]).P_SIG_PENDING, EAX ; Generate signal
                JMP     EMX_OK
                ASSUME  ESI:NOTHING

;
; TODO: Error message
;
FPUEMU_EXIT:    MOV     AX, 4CFFH
                INT     21H

FPUEMU_FAIL:    MOV     I_ECX, EINVAL
                MOV     I_EAX, -1
                RET

                ASSUME  DI:NOTHING

; ----------------------------------------------------------------------------
; AX=7F58H: __settime()
;
; Set system time
;
; In:   EDX     Pointer to struct timeval
;
; Out:  EAX     0 (success) or -1 (failure)
;       ECX     errno (0 if no error)
;
; ----------------------------------------------------------------------------
                TALIGN  4
SYS58:          MOV     ESI, I_EDX
                MOV     ES, I_DS
                CALL    DO_SETTIME
                MOV     I_ECX, EAX
                MOV     I_EAX, 0
                TEST    EAX, EAX
                JZ      SHORT SYS58_RET
                MOV     I_EAX, -1
SYS58_RET:      RET

; ----------------------------------------------------------------------------
; AX=7F59H: __profil()
;
; Sampling profiler
;
; In:   EDX     Pointer to struct _profil
;
; Out:  EAX     0 (success) or -1 (failure)
;       ECX     errno (0 if no error)
;
; ----------------------------------------------------------------------------
                TALIGN  4
                ASSUME  DI:PTR PROCESS
SYS59:          MOV     ES, I_DS
                MOV     ESI, I_EDX
                CALL    DO_PROFIL
                MOV     I_ECX, EAX
                TEST    EAX, EAX
                JZ      SHORT SYS59_1
                MOV     EAX, -1
SYS59_1:        MOV     I_EAX, EAX
                RET

; ----------------------------------------------------------------------------
; AX=7F5AH: __nls_ctype()
;
; Mark DBCS lead bytes
;
; In:   EDX     pointer to buffer
;
; Out:  EAX     0 (success) or -1 (failure)
;       ECX     errno (0 if no error)
;
; ----------------------------------------------------------------------------
                TALIGN  4
SYS5A:          CALL    GET_DBCS_LEAD
                TEST    AX, AX
                JZ      SHORT NLS_CT_COPY
                JMP     SYSCALL_ENOSYS

;
; Copy the DBCS lead byte bits of our table to the target buffer
;
NLS_CT_COPY:    CMP     I_DS, 0                 ; See INIT_DBCS
                JE      SHORT NLS_CT_EFAULT
                MOV     ES, I_DS
                MOV     EDI, I_EDX
                MOV     ECX, 0
                TALIGN  4
NLS_CT_COPY_BIT:BT      DBCS_LEAD_TAB, CX
                JC      SHORT NLS_CT_COPY_SET
                AND     BYTE PTR ES:[EDI], 0FEH ; Clear _NLS_DBCS_LEAD
NLS_CT_COPY_NXT:INC     EDI
                INC     CX
                CMP     CX, 256
                JB      SHORT NLS_CT_COPY_BIT
                JMP     EMX_OK

                TALIGN  4
NLS_CT_COPY_SET:OR      BYTE PTR ES:[EDI], 01H  ; Set _NLS_DBCS_LEAD
                JMP     SHORT NLS_CT_COPY_NXT

NLS_CT_EFAULT:  MOV     I_EAX, -1
                MOV     I_ECX, EFAULT
                RET

                ASSUME  BP:NOTHING

; ----------------------------------------------------------------------------
;
; Convert a MY_DATETIME structure to Unix time format
;
; MDT must be in the stack!
;
; ----------------------------------------------------------------------------
                TALIGN  4
TIME_2_UNIX     PROC    PASCAL USES EBX ESI, MDT:PTR MY_DATETIME
                MOV     BX, MDT
                ASSUME  BX:PTR MY_DATETIME
                MOV     EAX, SS:[BX].YEAR
                MOV     EDX, 365
                MUL     EDX
                MOV     ESI, EAX
                MOV     EAX, SS:[BX].MONTH
                DEC     EAX
                MOV     EDX, 31
                MUL     EDX
                ADD     ESI, EAX
                ADD     ESI, SS:[BX].DAY        ; 365*year + day + 31*(month-1)
                DEC     SS:[BX].YEAR
                CMP     SS:[BX].MONTH, 2        ; Jan or Feb?
                JBE     SHORT T2U_1             ; Yes -> skip

                INC     SS:[BX].YEAR
                MOV     EAX, SS:[BX].MONTH
                SHL     EAX, 2
                ADD     EAX, 23
                XOR     EDX, EDX
                MOV     ECX, 10
                DIV     ECX
                SUB     ESI, EAX                ; - ((4*month)+23)/10

                TALIGN  4
T2U_1:          MOV     EAX, SS:[BX].YEAR
                SHR     EAX, 2
                ADD     ESI, EAX                ; + (year{-1})/4
                MOV     EAX, SS:[BX].YEAR
                XOR     EDX, EDX
                MOV     ECX, 100
                DIV     ECX
                INC     EAX
                MOV     EDX, EAX
                ADD     EAX, EAX
                ADD     EAX, EDX
                SHR     EAX, 2
                SUB     ESI, EAX                ; - (3*((year{-1})/100+1)) / 4
                MOV     EAX, ESI
                SUB     EAX, 719528             ; 01-Jan-1970
                MOV     EDX, 24*60*60
                MUL     EDX
;
; Add time of day
;
                ADD     EAX, SS:[BX].SECONDS
                MOV     ESI, EAX
                MOV     EAX, SS:[BX].MINUTES
                MOV     EDX, 60
                MUL     EDX
                ADD     ESI, EAX
                MOV     EAX, SS:[BX].HOURS
                MOV     EDX, 60*60
                MUL     EDX
                ADD     EAX, ESI
                RET
                ASSUME  BX:NOTHING
TIME_2_UNIX     ENDP

; ----------------------------------------------------------------------------
; Convert a packed time/date value to Unix time value (GMT)
;
; In:  CX       Time
;      DX       Date
;
; Out: EAX      Unix time
;
; ----------------------------------------------------------------------------
;
                TALIGN  4
PACKED_2_UNIX   PROC
                LOCAL   MDT:MY_DATETIME
                MOV     AX, DX                  ; Day of month
                AND     EAX, 1FH
                MOV     MDT.DAY, EAX
                MOV     AX, DX                  ; Month
                SHR     AX, 5
                AND     EAX, 0FH
                MOV     MDT.MONTH, EAX
                MOV     AX, DX                  ; Year - 1980
                SHR     AX, 9
                AND     EAX, 7FH
                ADD     EAX, 1980
                MOV     MDT.YEAR, EAX
                MOV     AX, CX                  ; Seconds / 2
                AND     EAX, 1FH
                SHL     EAX, 1
                MOV     MDT.SECONDS, EAX
                MOV     AX, CX                  ; Minutes
                SHR     AX, 5
                AND     EAX, 3FH
                MOV     MDT.MINUTES, EAX
                MOV     AX, CX                  ; Hours
                SHR     AX, 11
                AND     EAX, 1FH
                MOV     MDT.HOURS, EAX
                INVOKE  TIME_2_UNIX, ADDR MDT
                RET
PACKED_2_UNIX   ENDP


;
; In:  EAX      Year
;
; Out: EAX      1 if leap year, 0 otherwise
;
                TALIGN  4
LEAP_YEAR       PROC    NEAR
                PUSH    ECX
                PUSH    EDX
                TEST    EAX, 3                  ; year = 4n?
                JNZ     SHORT LEAP_NO           ; No -> not a leap year
                MOV     ECX, 100
                XOR     EDX, EDX
                DIV     ECX
                OR      EDX, EDX                ; year = 100n?
                JNZ     SHORT LEAP_YES          ; No  -> it's a leap year
                TEST    EAX, 3                  ; year = 400n?
                JNZ     SHORT LEAP_NO           ; No  -> not a leap year
LEAP_YES:       XOR     EAX, EAX
                INC     EAX
                JMP     SHORT LEAP_RET

                TALIGN  4
LEAP_NO:        XOR     EAX, EAX
LEAP_RET:       POP     EDX
                POP     ECX
                RET
LEAP_YEAR       ENDP

; ----------------------------------------------------------------------------
;
; Convert a Unix time value to a MY_DATETIME structure
;
; MDT must be in the stack!
;
; ----------------------------------------------------------------------------
                TALIGN  4
UNIX_2_TIME     PROC    PASCAL USES EBX ESI EDI,
                        TIME:DWORD, MDT:PTR MY_DATETIME
                MOV     BX, MDT
                ASSUME  BX:PTR MY_DATETIME
                MOV     EAX, TIME
                MOV     ECX, 60
                XOR     EDX, EDX
                DIV     ECX
                MOV     SS:[BX].SECONDS, EDX
                XOR     EDX, EDX
                DIV     ECX
                MOV     SS:[BX].MINUTES, EDX
                MOV     ECX, 24
                XOR     EDX, EDX
                DIV     ECX
                MOV     SS:[BX].HOURS, EDX
                MOV     ESI, 1970
                MOV     EDI, EAX
                TALIGN  4
U2T_Y_LOOP:     MOV     EAX, ESI
                CALL    LEAP_YEAR
                ADD     EAX, 365
                MOV     ECX, EAX
                INC     ESI
                SUB     EDI, ECX
                JNC     SHORT U2T_Y_LOOP
                ADD     EDI, ECX
                DEC     ESI
                MOV     SS:[BX].YEAR, ESI
                MOV     ESI, 0
                TALIGN  4
U2T_M_LOOP:     CMP     ESI, 1                  ; February?
                JE      SHORT U2T_M_FEB
                MOVZX   ECX, MONTH_LEN[ESI]
                JMP     SHORT U2T_M_CONT
                TALIGN  4
U2T_M_FEB:      MOV     EAX, SS:[BX].YEAR
                CALL    LEAP_YEAR
                ADD     EAX, 28
                MOV     ECX, EAX
                TALIGN  4
U2T_M_CONT:     INC     ESI
                SUB     EDI, ECX
                JNC     SHORT U2T_M_LOOP
                ADD     EDI, ECX
                MOV     SS:[BX].MONTH, ESI
                INC     EDI
                MOV     SS:[BX].DAY, EDI
                RET
UNIX_2_TIME     ENDP


;
; Convert Unix time value (GMT) to a packed time/date value
;
; In:  EAX      Unix time
;
; Out: CX       Time
;      DX       Date
;
UNIX_2_PACKED   PROC
                LOCAL   MDT:MY_DATETIME
                INVOKE  UNIX_2_TIME, EAX, ADDR MDT

                MOV     ECX, MDT.SECONDS
                SHR     ECX, 1
                MOV     EAX, MDT.MINUTES
                SHL     EAX, 5
                OR      ECX, EAX
                MOV     EAX, MDT.HOURS
                SHL     EAX, 11
                OR      ECX, EAX

                MOV     EDX, MDT.DAY
                MOV     EAX, MDT.MONTH
                SHL     EAX, 5
                OR      EDX, EAX
                MOV     EAX, MDT.YEAR
                SUB     EAX, 1980
                SHL     EAX, 9
                OR      EDX, EAX
                RET
UNIX_2_PACKED   ENDP


;
; In:  ES:EDI   destination
;
                TALIGN  4
                ASSUME  EDI:NEAR32 PTR TIMEB
DO_FTIME        PROC
                LOCAL   DATE_CX:WORD, DATE_DX:WORD
                LOCAL   TIME_CX:WORD, TIME_DX:WORD
                LOCAL   MDT:MY_DATETIME

RETRY:          MOV     AH, 2AH
                INT     21H
                MOV     DATE_CX, CX
                MOV     DATE_DX, DX
                MOV     AH, 2CH
                INT     21H
                MOV     TIME_CX, CX
                MOV     TIME_DX, DX
                MOV     AH, 2AH
                INT     21H
                CMP     CX, DATE_CX
                JNE     SHORT RETRY
                CMP     DX, DATE_DX
                JNE     SHORT RETRY

                MOVZX   EAX, BYTE PTR TIME_DX[1]
                MOV     MDT.SECONDS, EAX
                MOVZX   EAX, BYTE PTR TIME_CX[0]
                MOV     MDT.MINUTES, EAX
                MOVZX   EAX, BYTE PTR TIME_CX[1]
                MOV     MDT.HOURS, EAX
                MOVZX   EAX, BYTE PTR DATE_DX[0]
                MOV     MDT.DAY, EAX
                MOVZX   EAX, BYTE PTR DATE_DX[1]
                MOV     MDT.MONTH, EAX
                MOVZX   EAX, DATE_CX
                MOV     MDT.YEAR, EAX

                INVOKE  TIME_2_UNIX, ADDR MDT

                MOV     ES:[EDI].DSTFLAG, 0
                MOV     ES:[EDI].TIME, EAX
                MOVZX   EAX, BYTE PTR TIME_DX[0]
                MOV     ECX, 10
                MUL     ECX
                MOV     ES:[EDI].MILLITM, EAX
                MOV     ES:[EDI].TIMEZONE, 0
                ASSUME  EDI:NOTHING
                RET
DO_FTIME        ENDP


;
; In: ES:ESI    Source
;
                TALIGN  4
                ASSUME  ESI:NEAR32 PTR TIMEVAL
DO_SETTIME      PROC
                LOCAL   MDT:MY_DATETIME
                MOV     EAX, ES:[ESI].TV_USEC
                XOR     EDX, EDX
                MOV     ECX, 1000000
                DIV     ECX
                PUSH    EDX                     ; Save microseconds
                ADD     EAX, ES:[ESI].TV_SEC
                INVOKE  UNIX_2_TIME, EAX, ADDR MDT
                POP     EAX                     ; Microseconds
                XOR     EDX, EDX
                MOV     ECX, 10000
                DIV     ECX                     ; 1/100 seconds
                MOV     DL, CL
                MOV     DH, BYTE PTR MDT.SECONDS
                MOV     CL, BYTE PTR MDT.MINUTES
                MOV     CH, BYTE PTR MDT.HOURS
                MOV     AH, 2DH                 ; Set system time
                INT     21H
                TEST    AL, AL
                JNZ     SHORT ERROR
; TODO: Time window
                MOV     DL, BYTE PTR MDT.DAY
                MOV     DH, BYTE PTR MDT.MONTH
                MOV     CX, WORD PTR MDT.YEAR
                MOV     AH, 2BH                 ; Set system date
                INT     21H
                TEST    AL, AL
                JNZ     SHORT ERROR
                XOR     EAX, EAX
                RET

ERROR:          MOV     EAX, EINVAL
                RET
DO_SETTIME      ENDP

                ASSUME  ESI:NOTHING


;
; Compare two strings
;
; In:   DS:ESI  First string
;       ES:EDI  Second string
;
; Out:  ZR      Strings are equal
;       ESI     Modified
;       EDI     Modified
;
                TALIGN  4
STRCMP          PROC    NEAR
SC_1:           LODS    BYTE PTR DS:[ESI]
                SCAS    BYTE PTR ES:[EDI]
                JNE     SHORT SC_RET
                OR      AL, AL
                JNZ     SHORT SC_1
SC_RET:         RET
STRCMP          ENDP
       
;
; Compare two strings, disregarding letter case
;
; In:   DS:SI   First string
;       ES:DI   Second string
;
; Out:  ZR      Strings are equal
;       SI      Modified
;
                TALIGN  4
STRICMP         PROC    NEAR
                PUSH    DI
SIC_1:          LODSB
                CALL    UPPER
                MOV     AH, AL
                MOV     AL, ES:[DI]
                INC     DI
                CALL    UPPER
                CMP     AL, AH
                JNE     SHORT SIC_RET
                OR      AL, AL
                JNZ     SHORT SIC_1
SIC_RET:        POP     DI
                RET
STRICMP         ENDP
       


SV_CODE         ENDS

                END
