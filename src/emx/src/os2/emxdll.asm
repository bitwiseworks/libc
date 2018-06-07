;
; EMXDLL.ASM
;
; Copyright (c) 1992-1996 by Eberhard Mattes
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
; As special exception, emx.dll can be distributed without source code
; unless it has been changed.  If you modify emx.dll, this exception
; no longer applies and you must remove this paragraph from all source
; files for emx.dll.
;

                .386

                INCLUDE EMXDLL.INC

;
; IBM TCP/IP's socket handles are < 2048.  See also emxdll.h.
;
MAX_SOCKETS     =       2048

; ----------------------------------------------------------------------------
; Structures
; ----------------------------------------------------------------------------

;
; emx_syscall puts all the registers of the client process into the stack
; using PUSHAD.  This structure matches the layout of the registers in
; the stack.
;
SYSCALL_FRAME   STRUCT
SC_EDI          DWORD   ?
SC_ESI          DWORD   ?
SC_EBP          DWORD   ?
SC_ESP          DWORD   ?
SC_EBX          DWORD   ?
SC_EDX          DWORD   ?
SC_ECX          DWORD   ?
SC_EAX          DWORD   ?
SC_EFLAGS       DWORD   ?
SC_EIP          DWORD   ?
SYSCALL_FRAME   ENDS

;
; Thread data block.  Fields at the end of the structure are omitted.
; In consequence, the size of the structure is wrong and must not be
; used.
;
THREAD_DATA     STRUCT
LAST_SYS_ERRNO  DWORD   ?                       ; Error code for last syscall
PREV_SYS_ERRNO  DWORD   ?                       ; Previous value of the above
; ...and other fields, which are omitted here
THREAD_DATA     ENDS

; ----------------------------------------------------------------------------
; Data
; ----------------------------------------------------------------------------

BSS32           SEGMENT

;
; The following variables are imported from other modules
;
                EXTRN   stack_end:DWORD
                EXTRN   layout:DWORD
                EXTRN   threads:DWORD
                EXTRN   env_count:DWORD
                EXTRN   arg_size:DWORD
                EXTRN   arg_count:DWORD
                EXTRN   fork_flag:BYTE
                EXTRN   fork_stack_page:DWORD
                EXTRN   fork_ebp:DWORD
                EXTRN   exc_reg_ptr:DWORD
                EXTRN   errout_handle:DWORD

;
; Private data of this module
;
RET_ADDR        DWORD   ?                       ; Return address

BSS32           ENDS

; ----------------------------------------------------------------------------
; Solo data
; ----------------------------------------------------------------------------

COMMON32        SEGMENT
pipe_number     DWORD   ?
queue_number    DWORD   ?
sock_proc_count DWORD   MAX_SOCKETS DUP (?)
COMMON32        ENDS


; ----------------------------------------------------------------------------
; The code segment
; ----------------------------------------------------------------------------

TEXT32          SEGMENT

                ASSUME  DS:FLAT, ES:FLAT

;
; emx.dll exports the following functions of this module:
;
                PUBLIC  emx_init
                PUBLIC  emx_syscall

;
; The following functions are exported to other modules
;
                PUBLIC  get_thread
                PUBLIC  get_tid
                PUBLIC  init_signal16
                PUBLIC  sig16_handler
                PUBLIC  touch
                PUBLIC  unwind

;
; The following variables are exported to other modules
;
                PUBLIC  pipe_number
                PUBLIC  queue_number
                PUBLIC  sock_proc_count
                PUBLIC  old_sig16_handler

;
; The following functions are imported from other modules
;
                EXTRN   build_arg_env:PROC
                EXTRN   copy_fork:PROC
                EXTRN   count_arg_env:PROC
                EXTRN   dos_call:PROC
                EXTRN   initialize1:PROC
                EXTRN   initialize2:PROC
                EXTRN   raise_from_16:PROC
                EXTRN   sys_call:PROC
                EXTRN   dll_init:PROC
                EXTRN   _DLL_InitTerm:PROC
                EXTRN   new_thread:PROC

;
; Import OS/2 API
;
                EXTRN   DosUnwindException:PROC


; ----------------------------------------------------------------------------
; Macros
; ----------------------------------------------------------------------------

;
; REG := pointer to thread data block
;
GET_THREAD      MACRO   REG
                MOV     REG, (TIB PTR FS:[0]).TIB_SYS_PTR
                MOV     REG, (TIB2 PTR [REG]).TID
                MOV     REG, threads[REG*4]
                ENDM


; ----------------------------------------------------------------------------
; Code
; ----------------------------------------------------------------------------

;
; This procedure is called by the startup code, see crt0.s.  A pointer
; to the layout table is passed on the stack.
;
; Doing this in C would be hard because we play dirty tricks with the stack.
;
emx_init:       POP     RET_ADDR                ; Remove return address
                POP     EAX                     ; Pointer to layout table
                MOV     errout_handle, 2        ; Use stderr unless -!1 given
                TEST    (LAYOUT_TABLE PTR [EAX]).L_FLAGS, L_FLAG_DLL
                JNZ     DLL_INIT_1              ; Called by DLL startup code
                MOV     layout, EAX
                CALL    initialize1             ; First part of initialization
              .IF fork_flag
                MOV     ESP, fork_stack_page
                CALL    copy_fork
              .ENDIF
                CALL    USE_STACK               ; Switch to new stack
                CALL    initialize2             ; Second part of initialization
              .IF fork_flag
                MOV     EBP, fork_ebp
                MOV     I_EAX, 0                ; Make fork() return 0
                MOV     I_ECX, 0                ; Keep errno
                JMP     SYSCALL_RET
              .ENDIF
;
; Compute number of bytes to be allocated for argv[] and envp[] and strings
;
                CALL    count_arg_env
;
; Allocate space in the stack for the environment pointers and the arguments
;
                MOV     ESI, ESP
                SUB     ESI, arg_size           ; Argument strings are put here
                MOV     EDI, ESI
                MOV     EAX, arg_count
                ADD     EAX, env_count
                ADD     EAX, 2                  ; Two NULL pointers
                SHL     EAX, 2
                SUB     EDI, EAX                ; Tables are put here
                AND     EDI, NOT 3              ; Alignment
                MOV     BYTE PTR [EDI], 0       ; Stack probe
                MOV     ESP, EDI
;
; Build envp[] and argv[]
;
                PUSH    EDI                     ; vec
                PUSH    ESI                     ; str
                CALL    build_arg_env           ; build_arg_env (str, vec)
                ADD     ESP, 2 * 4
;
; Return to startup code
;
                JMP     RET_ADDR

;
; We have been called by the DLL startup code, see dll0.s
;
                TALIGN  4
DLL_INIT_1:     CMP     DWORD PTR [ESP+8], 0    ; Initialization?
                JNE     SHORT DLL_INIT_2        ; No  -> skip
                PUSH    EAX
                CALL    dll_init
                ADD     ESP, 4
DLL_INIT_2:     JMP     RET_ADDR


;
; Allocate space for the exception registration record on the stack.
; If this is a forked process, adjust the stack pointer.
;
                TALIGN  4
USE_STACK       PROC
                POP     ECX                     ; Retrieve the return address
                MOV     ESI, stack_end          ; EXC_REGISTER buffer must be
                SUB     ESI, SIZE EXC_REGISTER  ; be in the stack
                AND     ESI, NOT 3              ; Align the stack pointer
                MOV     exc_reg_ptr, ESI
                MOV     AL, [ESI]               ; Stack probe
              .IF fork_flag
                MOV     ESI, fork_ebp
              .ENDIF
                MOV     ESP, ESI                ; Allocate space
                JMP     ECX
                RET                             ; Avoid warning
USE_STACK       ENDP

;
; emx_syscall puts all the registers of the client process into the stack
; using PUSHAD.  The registers in the stack can then be accessed using the
; following names.  This can be used both for input and output.
;
I_EDI           EQU     (SYSCALL_FRAME PTR [EBP]).SC_EDI
I_ESI           EQU     (SYSCALL_FRAME PTR [EBP]).SC_ESI
I_EBP           EQU     (SYSCALL_FRAME PTR [EBP]).SC_EBP
I_ESP           EQU     (SYSCALL_FRAME PTR [EBP]).SC_ESP
I_EBX           EQU     (SYSCALL_FRAME PTR [EBP]).SC_EBX
I_EDX           EQU     (SYSCALL_FRAME PTR [EBP]).SC_EDX
I_ECX           EQU     (SYSCALL_FRAME PTR [EBP]).SC_ECX
I_EAX           EQU     (SYSCALL_FRAME PTR [EBP]).SC_EAX
I_EFLAGS        EQU     (SYSCALL_FRAME PTR [EBP]).SC_EFLAGS
I_EIP           EQU     (SYSCALL_FRAME PTR [EBP]).SC_EIP

I_AL            EQU     (BYTE PTR I_EAX+0)
I_AH            EQU     (BYTE PTR I_EAX+1)
I_CL            EQU     (BYTE PTR I_ECX+0)
I_CH            EQU     (BYTE PTR I_ECX+1)
I_DL            EQU     (BYTE PTR I_EDX+0)
I_DH            EQU     (BYTE PTR I_EDX+1)

;
; syscall entry point.  AH contains the function number, the other registers
; contain arguments.  Usually, CY will be set on return though not required
; for all functions.  The function numbers are somewhat similar to PC-DOS
; function numbers -- it's simpler to use those numbers than inventing and
; *remembering* new numbers (or using the(?) Unix syscall function numbers).
; Using function names would be much more complex.  In fact, even OS/2 uses
; numbers (import by ordinal).  See system.doc for details.
;
; As arguments are passed in registers (which may have been a bad idea),
; this cannot be done in C.
;
                TALIGN  4
emx_syscall:    PUSHFD
                PUSHAD                          ; Save all the registers
                MOV     EBP, ESP                ; Make stack frame
                CLD                             ; Don't trust anyone
                GET_THREAD ESI                  ; ESI := pointer to thread data
                TEST    ESI, ESI                ; Pointer valid?
                JZ      SHORT SYSCALL_ERRNO_1   ; No  -> skip
                ASSUME  ESI:PTR THREAD_DATA
                MOV     EAX, [ESI].LAST_SYS_ERRNO;  Get old error code
                MOV     [ESI].PREV_SYS_ERRNO, EAX ; Save it for __syserrno()
                MOV     [ESI].LAST_SYS_ERRNO, 0 ; Clear error code
                ASSUME  ESI:NOTHING
SYSCALL_ERRNO_1:MOVZX   EAX, I_AH               ; Get function number
                CMP     AL, 7FH                 ; Special function?
                JE      SHORT SPECIAL           ; Yes -> function code in AL
                PUSH    EBP                     ; f
                CALL    dos_call                ; dos_call (f)
                ADD     ESP, 1 * 4
SYSCALL_RET:    POPAD                           ; Restore registers
                POPFD
                RET                             ; Return to user program

                TALIGN  4
SPECIAL:        PUSH    EBP                     ; f
                CALL    sys_call                ; sys_call (f)
                ADD     ESP, 1 * 4
                JMP     SYSCALL_RET             ; Restore registers and return

;
; Unwind exception handlers.  Doing this in C would be hairy due to
; DosUnwindException not returning and stack pointer adjustments done
; or not done by the C compiler.
;
; void unwind (EXCEPTIONREGISTRATIONRECORD *registration,
;              EXCEPTIONREPORTRECORD *report);
;
;
REGISTRATION    EQU     (DWORD PTR [EBP+2*4])
REPORT          EQU     (DWORD PTR [EBP+3*4])
                TALIGN  4
unwind          PROC
                PUSH    EBP                     ; Set up stack frame
                MOV     EBP, ESP
                PUSH    EBX                     ; Save registers
                PUSH    ESI
                PUSH    EDI
                PUSH    REPORT                  ; Report
                PUSH    OFFSET FLAT:UNWIND_1    ; Target address (jump there)
                PUSH    REGISTRATION            ; Handler
                MOV     AL, 3
                CALL    DosUnwindException      ; Does not return
                TALIGN  4
UNWIND_1:       ADD     ESP, 3 * 4              ; Remove arguments
                POP     EDI                     ; Restore registers
                POP     ESI
                POP     EBX
                POP     EBP
                RET
unwind          ENDP


;
; Get pointer to thread data block.  In assembler, we can do much better
; (by using FS:0) than in C (using DosGetInfoBlocks).
;
                TALIGN  4
get_thread      PROC
                GET_THREAD EAX
                TEST    EAX, EAX
                JZ      SHORT gt_new
                RET

; new_thread (tid, NULL)
gt_new:         CALL    get_tid
                PUSH    0                       ; errnop
                PUSH    EAX                     ; tid
                CALL    new_thread
                ADD     ESP, 2*4
                GET_THREAD EAX
                RET
get_thread      ENDP


;
; Return the thread ID of the current thread.
;
                TALIGN  4
get_tid         PROC
                MOV     EAX, (TIB PTR FS:[0]).TIB_SYS_PTR
                MOV     EAX, (TIB2 PTR [EAX]).TID
                RET
get_tid         ENDP


;
; Initialize variables for the 16-bit signal handler.
;
                TALIGN  4
init_signal16   PROC
                MOV     SS32, SS
                MOV     FS32, FS
                MOV     DS32, DS
                MOV     ES32, ES
                RET
init_signal16   ENDP


;
; Touch each page in a range of addresses
;
; void touch (void *base, ULONG count);
;
; In: BASE      Start address
;     COUNT     Number of bytes
;
; This is required because DosRead seems not to be reentrant enough --
; if it's used to read data into a page to be loaded from the .EXE file
; (dumped heap), it's recursively called by the guard pageexception handler.
; This call seems to disturb the first call, which will return a strange
; `error code' (ESP plus some constant).
;

BASE            EQU     (DWORD PTR [EBP+2*4])
COUNT           EQU     (DWORD PTR [EBP+3*4])
                TALIGN  4
touch           PROC
                PUSH    EBP                     ; Make stack frame
                MOV     EBP, ESP
                PUSH    EBX                     ; Save register
                MOV     ECX, COUNT
                JECXZ   TOUCH_RET
                MOV     EBX, BASE
                MOV     AL, [EBX]               ; Touch first page
                MOV     EAX, EBX                ; Save start address
                OR      EBX, 0FFFH              ; Move pointer to
                INC     EBX                     ; start of next page
                SUB     EBX, EAX                ; Number of bytes skipped
                SUB     ECX, EBX                ; Adjust counter
                JBE     SHORT TOUCH_RET         ; Done -> return
                ADD     EBX, EAX                ; Recompute pointer
                TALIGN  4
TOUCH_1:        MOV     AL, [EBX]               ; Touch a page
                ADD     EBX, 1000H              ; Move pointer to next page
                SUB     ECX, 1000H              ; Adjust counter
                JA      SHORT TOUCH_1           ; More -> repeat
TOUCH_RET:      POP     EBX                     ; Restore register
                POP     EBP                     ; Remove stack frame
                RET                             ; Done
touch           ENDP


;
; 32-bit code called from 16-bit code: Raise signal CX
;
                TALIGN  4
CALL32:         MOVZX   EAX, CX                 ; Signal number to EAX
                PUSH    EAX                     ; signo
;
; Raise signal (by calling)
;
                CALL    raise_from_16           ; raise_from_16 (signo)
                ADD     ESP, 1 * 4
;
; Return to 16-bit code
;
                LSS     SP, [ESP]               ; Restore 16-bit stack
                MOVZX   ESP, SP                 ; Don't trust
                JMP     FAR PTR TEXT16:MODE32_RET

TEXT32          ENDS

; ----------------------------------------------------------------------------
; 16-bit section
; ----------------------------------------------------------------------------

;
; Import OS/2 API
;

                EXTRN   _16_Dos16SetSigHandler:FAR16

; ----------------------------------------------------------------------------
; 16-bit data
; ----------------------------------------------------------------------------

BSS16           SEGMENT

old_sig16_handler DWORD ?

OLD_SIG_ACTION  DWORD   ?

;
; Saved segment registers of 32-bit mode, used for switching from
; a 16-bit signal handler to 32-bit mode.
;
DS32            WORD    ?               ; The DS register of 32-bit mode
ES32            WORD    ?               ; The ES register of 32-bit mode
SS32            WORD    ?               ; The SS register of 32-bit mode
FS32            WORD    ?               ; The FS register of 32-bit mode

BSS16           ENDS

; ----------------------------------------------------------------------------
; 16-bit code
; ----------------------------------------------------------------------------

TEXT16          SEGMENT

                ASSUME  CS:TEXT16, DS:NOTHING, ES:NOTHING

;
; The 16-bit signal handler
;
SH_SIGNUM       EQU     (WORD PTR [BP+6])       ; Signal number
SH_SIGARG       EQU     (WORD PTR [BP+8])       ; User-defined argument

                ASSUME  DS:NOTHING
                TALIGN  4
sig16_handler   PROC    FAR
                PUSH    BP                      ; Make stack frame
                MOV     BP, SP
                PUSH    DS                      ; Save DS
                MOV     AX, BSS16
                MOV     DS, AX                  ; Access our data segment
                ASSUME  DS:BSS16
                PUSH    CS                      ; Signal handler function
                PUSH    OFFSET TEXT16:sig16_handler
                PUSH    BSS16                   ; Old signal handler function
                PUSH    OFFSET BSS16:old_sig16_handler
                PUSH    BSS16                   ; Old signal handler action
                PUSH    OFFSET BSS16:OLD_SIG_ACTION
                PUSH    SIGA_ACKNOWLEDGE        ; Action
                PUSH    SH_SIGNUM               ; Signal
                CALL    _16_Dos16SetSigHandler  ; Acknowledge signal
                MOV     CX, SH_SIGARG
                CALL    MODE32                  ; Raise 32-bit signal
                POP     DS                      ; Restore DS
                ASSUME  DS:NOTHING
                POP     BP                      ; Remove stack frame
                RET     4                       ; Remove arguments
sig16_handler   ENDP

                ASSUME  DS:BSS16
;
; Call 32-bit code from 16-bit code
;
                TALIGN  4
MODE32          PROC    NEAR
                PUSH    FS
                PUSH    BP
                MOV     BP, SP
                SUB     SP, 4
                AND     SP, NOT 3               ; Align SP on a DWORD
                MOV     [BP-4], EAX
                MOV     AX, SP
                PUSH    SS
                PUSH    AX
                MOV     AX, SS
                SHR     AX, 3
                SHL     EAX, 16
                MOV     AX, SP
                PUSH    SS32
                PUSH    EAX
                MOVZX   ESP, SP
;
; Restore the FS register of 32-bit mode.  FS:0 points to the TIB.
; Fortunately, the selector seems to be the same for all the threads.
; If each thread had its own selector, we would have a problem.  The
; only solution for this case I can think of is to store the FS register
; in the thread-specific data block.
;
                MOV     FS, FS32
;
; Restore the DS and ES registers of 32-bit mode.
;
                MOV     ES, ES32
                MOV     DS, DS32
                ASSUME  DS:NOTHING
                MOV     EAX, [BP-4]
                LSS     ESP, [ESP]
                JMP     FAR PTR FLAT:CALL32
;
; Return here from CALL32.
;
                TALIGN  4
MODE32_RET::    PUSH    AX
                MOV     AX, BSS16
                MOV     DS, AX
                ASSUME  DS:BSS16
                POP     AX
                MOV     SP, BP
                POP     BP
                POP     FS
                RET
MODE32          ENDP

TEXT16          ENDS

                END     _DLL_InitTerm
