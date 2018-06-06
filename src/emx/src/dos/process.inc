;
; PROCESS.INC -- Manage processes
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

;
; Requires: SIGNAL.INC (for SIGNALS)
;

;
; Capacity
;
MAX_FILES       =       40              ; Max number of open files per process
MAX_PROCESSES   =       6               ; Max number of processes
MAX_TIMERS      =       2 * MAX_PROCESSES       ; Max number of timers

;
; Special process pointers
;
NO_PROCESS      =       0               ; Used for PROCESS_PTR, PROCESS_SIG,
                                        ; FIND_PROCESS, etc.
NO_WPROCESS     =       1               ; Used for WAIT_PROCESS

;
; P_FLAGS bits
;
PF_DEBUG        =       0001H           ; Process is being debugged
PF_WAIT_WAIT    =       0002H           ; Process waits for being waited for
PF_387_USED     =       0004H           ; 387 used, don't FNINIT
PF_PRELOADING   =       0008H           ; Preloading in progress (for swapper)
PF_TERMIO_TIME  =       0010H           ; VTIME expired
PF_SLEEP_FLAG   =       0020H           ; sleep interval expired
PF_PSEUDO_ASYNC =       0040H           ; wait() can be done for pseudo async
PF_REDIR_STDERR =       0200H           ; Redirect stderr to stdout
PF_QUOTE        =       0400H           ; Quote arguments for children
PF_COMMIT       =       0800H           ; Commit memory
PF_DONT_ZERO    =       1000H           ; Don't zero unitialized data
PF_PRELOAD      =       4000H           ; Disable preloading (-L)
PF_NO_CORE      =       8000H           ; Disable core dumps (-c)
PF_FPUEMU_INIT  =      10000H           ; FPU emu initialized for this process

;
; P_UFLAGS bits
;
UF_SIG_MODEL    =       00003H          ; Mask for signal() semantics
UF_SIG_EMX      =       00000H          ; signal() semantics: emx (SIG_ACK)
UF_SIG_SYSV     =       00001H          ; signal() semantics: System V
UF_SIG_BSD      =       00002H          ; signal() semantics: BSD

;
; P_STATUS values
;
PS_NONE         =       0               ; empty slot
PS_DEFUNCT      =       1               ; terminated, zombie (til wait)
PS_INIT         =       2               ; initialized but not started
PS_RUN          =       3               ; running
PS_STOP         =       4               ; stopped (debugging)
PS_WAIT_SPAWN   =       5               ; executes synchronous spawn
PS_WAIT_CHILD   =       6               ; executes wait()
PS_WAIT_PTRACE  =       7               ; executes ptrace()
PS_FPUEMU_CLIENT =      8               ; application being serviced by FPU emu
PS_FPUEMU_NEXT  =       9               ; FPU emulator waiting for notification

;
; Bits in the P_HW_ACCESS byte
;
HW_ACCESS_MEM   =       01H             ; Memory read access allowed
HW_ACCESS_IO    =       02H             ; Port access allowed
HW_ACCESS_WRITE =       04H             ; Memory write access allowed
HW_ACCESS_CODE  =       08H             ; Data segment and stack executable

;
; Bits in the P_HFLAGS array, cf. <sys/fcntl.h>
;
HF_APPEND       =       0008H           ; O_APPEND
HF_NDELAY       =       0004H           ; O_NONBLOCK and O_NDELAY
HF_NOINHERIT    =       1000H           ; O_NOINHERIT and FD_CLOEXEC

;
; Process table entry
;
; Note: P_PID must come first to simplify initialization
;
PROCESS         STRUC
P_PID           DD      ?               ; Process ID (NO_PID: slot not used)
P_LINEAR        DD      ?               ; Linear address
P_DSEG_SIZE     DD      ?               ; Size of data segment
P_LIN_SIZE      DD      ?               ; Allocated linear addresses
P_BRK           DD      ?               ; Current break value
P_INIT_BRK      DD      ?               ; Initial break value
P_STACK_ADDR    DD      ?               ; Top of stack
P_STACK_SIZE    DD      ?               ; Stack size
P_TEXT_OFF      DD      ?               ; Start of code area
P_TEXT_SIZE     DD      ?               ; Size of code area
P_DATA_OFF      DD      ?               ; Start of data area
P_DATA_SIZE     DD      ?               ; Size of data area
P_BSS_OFF       DD      ?               ; Start of bss area
P_BSS_SIZE      DD      ?               ; Size of bss area
P_ENTRY_POINT   DD      ?               ; Entry point
P_NUMBER        DD      ?               ; Process number for __fpuemu()
P_SYMBOLS       DD      ?               ; Number of symbols
P_STATUS        DW      ?               ; Process status (PS_*)
P_EXEC_HANDLE   DW      ?               ; Handle of executable file
P_LDT           DW      ?               ; Selector of local descriptor table
P_LDT_PTR       DW      ?               ; Pointer to LDT
P_UMASK         DW      ?               ; umask
P_UMASK1        DW      ?               ; umask (old semantics)
P_LAST_BYTES    DW      ?               ; Number of bytes in last page
P_PIDX          DB      ?               ; Process index for paging
P_RC            DB      ?               ; Return code (P_STATUS = PS_DEFUNCT)
P_SIG_NO        DB      ?               ; Signal number (P_STATUS = PS_STOP)
P_HW_ACCESS     DB      ?               ; -a option bits
P_DRIVE         DB      ?               ; -r option
P_INTERFACE     DB      ?               ; Interface version, from layout table
P_PAD           DB      0 DUP (?)       ; Padding
P_COMMIT_SIZE   DD      ?               ; Additional pages
P_SPAWN_RC      DD      ?               ; Return code of child process (note 1)
P_PAGE_FAULTS   DD      ?               ; Number of page faults
P_PPID          DD      ?               ; Parent process id
P_STR_OFF       DD      ?               ; Offset of strings in symbol segment
P_EXEC_OFFSET   DD      ?               ; Offset of first page in exec file
P_LAST_PAGE     DD      ?               ; Last page of exec file (0=ignore)
P_SYM_PAGE      DD      ?               ; Page number for symbols
P_RELOC_SIZE    DD      ?               ; Size of relocation information
P_FLAGS         DD      ?               ; Flags (PF_*)
P_TRUNC         DD      ?               ; Bits set by -t option
P_SIG_HANDLERS  DD      SIGNALS DUP (?) ; Signal handlers
P_SA_MASK       DD      SIGNALS DUP (?) ; sa_mask of struct sigaction
P_SA_FLAGS      DD      SIGNALS DUP (?) ; sa_flags of struct sigaction
P_SIG_PENDING   DD      ?               ; Pending signals (bitwise)
P_SIG_BLOCKED   DD      ?               ; Blocked signals (bitwise)
P_UFLAGS        DD      ?               ; These flags can be changed by process
P_VESAINFO_PTR  DD      ?               ; Pointer to VESA info data area
P_PRF_BUFF      DD      ?               ; profil(): Pointer to counters
P_PRF_BUFSIZ    DD      ?               ; profil(): Size of buffer
P_PRF_OFFSET    DD      ?               ; profil(): First address
P_PRF_SCALE     DD      ?               ; profil(): Scaling factor
P_HANDLES       DW      MAX_FILES DUP (?)   ; DOS file handles
P_HFLAGS        DW      MAX_FILES DUP (?)   ; Handle flags (note 2)
;
; Cf. PMINT.INC!!! (sequence, padding!)
;
P_GS            DW      ?
P_FS            DW      ?
P_ES            DW      ?
P_DS            DW      ?
P_EDI           DD      ?
P_ESI           DD      ?
P_EBP           DD      ?
P_ESP_1         DD      ?
P_EBX           DD      ?
P_EDX           DD      ?
P_ECX           DD      ?
P_EAX           DD      ?
P_ERRCD         DD      ?
P_EIP           DD      ?
P_CS            DW      ?, 0
P_EFLAGS        DD      ?
P_ESP           DD      ?
P_SS            DW      ?, 0
;
; Floating point status
;
              IF FLOATING_POINT
P_CW            DW      ?, 0            ; Control-word register
P_SW            DW      ?, 0            ; Status-word register
P_TW            DW      ?, 0            ; Tag-word register
P_FIP           DD      ?               ; Last instruction (EIP)
P_FCS           DW      ?               ; Last instruction (CS)
P_FOP           DW      ?               ; Last instruction (opcode)
P_FOO           DD      ?               ; Memory operand offset
P_FOS           DW      ?, 0            ; Memory operand segment
P_FST           DT      8 DUP (?)       ; Stack
              ENDIF
PROCESS         ENDS
;
; Note 1: The P_SPAWN_RC field contains the process ID of the child,
;         if a child has been spawned for debugging (NP_DEBUG)
;         or if an asynchronous child process has ended.
;
; Note 2: P_HFLAGS is 16 bits wide to simplify things -- everytime we
;         access P_HFLAGS, we also access P_HANDLES, therefore we already
;         have computed the offset (2 * handle)
;

;
; Cf. <sys/process.h>
;
NP_SPAWN_SYNC   =       0                       ; P_WAIT
NP_SPAWN_ASYNC  =       1                       ; P_NOWAIT
NP_EXEC         =       2                       ; P_OVERLAY
NP_DEBUG        =       3                       ; P_DEBUG

NP2_QUOTE       =       0001H                   ; P_QUOTE >> 16

;
; Cf. struct _new_proc in <sys/emx.h>
;
; Note: Fields sorted by size to avoid problems with alignment (gcc!)
;
NEW_PROC        STRUC
NP_ARG_OFF      DD      ?               ; Pointer to arguments (offset)
NP_ENV_OFF      DD      ?               ; Pointer to environment (offset)
NP_FNAME_OFF    DD      ?               ; Pointer to file name (offset)
NP_ARG_SEL      DW      ?               ; Pointer to arguments (selector)
NP_ENV_SEL      DW      ?               ; Pointer to environment (selector)
NP_FNAME_SEL    DW      ?               ; Pointer to file name (selector)
NP_ARG_COUNT    DW      ?               ; Number of arguments
NP_ARG_SIZE     DW      ?               ; Size of arguments
NP_ENV_COUNT    DW      ?               ; Number of environment strings
NP_ENV_SIZE     DW      ?               ; Size of environment
NP_MODE1        DW      ?               ; Lower 16 bits of mode (see above)
NP_MODE2        DW      ?               ; Upper 16 bits of mode (note 2)
; The fields below are in addition to the structure passed to function 7F06H
NP_PARENT       DW      ?               ; Pointer to parent process (note 3)
NP_FPROC        DW      ?               ; Pointer to files process (note 4)
NEW_PROC        ENDS

NP_USER_SIZE    =        NEW_PROC.NP_MODE2      ; Size of structure passed to
                                                ; function 7F06H
;
; Note 2: The NP_MODE2 field of the NEW_PROC structure is ignored unless
;         Bit 15 of NP_MODE1 is set.
;
; Note 3: The NP_PARENT field of the NEW_PROC structure points to the
;         process table entry of the process starting the new process.
;         If the new process is exec'ed (not spawned), this is *not*
;         the parent process.
;
; Note 4: The NP_FPROC field of the NEW_PROC structure points to the
;         process table entry of the process from which the file handles
;         and signal settings are to be inherited.
;

;
; Timers
;
TT_NONE         =       0
TT_EXPIRED      =       1
TT_ALARM        =       2
TT_SLEEP        =       3
TT_TERMIO_TIME  =       4

TT_RUNNING      =       TT_ALARM        ; T_TYPE >= TT_RUNNING -> running

TIMER           STRUC
T_TYPE          DW      ?               ; Type of timer
T_PROCESS       DW      ?               ; Pointer to process table entry
T_TICKS         DD      ?               ; Remaining timer ticks
TIMER           ENDS

;
; Status of floating point unit emulator.  Some of the status information
; should be kept in P_STATUS of the PTE for the client process instead.
;
FES_OFF         =       0               ; FPU emu not loaded
FES_NONE        =       1               ; Failed to load the FPU emu
FES_LOADING     =       2               ; Loading the FPU emu
FES_INIT1       =       3               ; Initialization, sending FPUN_NEWPROC
FES_ON          =       4               ; Normal state of the FPU emu
FES_NEWPROC     =       5               ; New process started
FES_ENDPROC     =       6               ; Process ended

SV_DATA         SEGMENT
                EXTERNDEF CLOCK_LO:DWORD
                EXTERNDEF CLOCK_HI:DWORD
SV_DATA         ENDS

              IFNDEF __PROCESS

SV_DATA         SEGMENT

                EXTRN   PROCESS_TABLE:DWORD
                EXTRN   PROC0:PROCESS
                EXTRN   PROCESS_PTR:WORD
                EXTRN   PROCESS_SIG:WORD
                EXTRN   PROCESS_FP:WORD
                EXTRN   PROCESS_FPUEMU:WORD
                EXTRN   PROCESS_FPUCLIENT:WORD
                EXTRN   VIDEO_LIN:DWORD
                EXTRN   TIMER_TABLE:DWORD
                EXTRN   TIMER_MIN:DWORD
                EXTRN   TIMER_TICKS:DWORD
                EXTRN   FPUEMU_STATE:BYTE
                EXTRN   NP1:NEW_PROC
                EXTRN   EMX_DIR:BYTE

SV_DATA         ENDS


SV_CODE         SEGMENT

                EXTRN   NEW_PROCESS:NEAR
                EXTRN   FIND_PROCESS:NEAR
                EXTRN   REMOVE_PROCESS:NEAR
                EXTRN   ZOMBIE_PROCESS:NEAR
                EXTRN   KILL_PROCESS:NEAR
                EXTRN   SAVE_PROCESS:NEAR
                EXTRN   REST_PROCESS:NEAR
                EXTRN   WAIT_PROCESS:NEAR
                EXTRN   SET_TIMER:NEAR
                EXTRN   SLEEP:NEAR
                EXTRN   SET_COMMIT:NEAR
                EXTRN   CREATE_PID:NEAR
                EXTRN   FIND_EXEC:NEAR
                EXTRN   FPUEMU_LOAD:NEAR
                EXTRN   FPUEMU_CALL:NEAR
                EXTRN   FPUEMU_ENDPROC:NEAR
                EXTRN   FPUEMU_NEWPROC:NEAR

SV_CODE         ENDS


INIT_CODE       SEGMENT

                EXTRN   PROCESS_INIT:NEAR

INIT_CODE       ENDS

              ENDIF
