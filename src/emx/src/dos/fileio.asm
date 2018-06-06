;
; FILEIO.ASM -- Handle files and file handles
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
                INCLUDE SIGNAL.INC
                INCLUDE PROCESS.INC
                INCLUDE SWAPPER.INC
                INCLUDE ERRORS.INC
                INCLUDE OPTIONS.INC
                INCLUDE OPRINT.INC
                INCLUDE PMINT.INC
                INCLUDE UTILS.INC

TOTAL_FILES     =       128

SV_DATA         SEGMENT

;
; For each DOS file handle, this array contains the number of times
; the handle is referenced.
;
DOS_REF_COUNT   DW      TOTAL_FILES DUP (?)

$CHECK_HANDLES  DB      "Internal error: wrong handle reference count", 0

$DIR_DEV        BYTE    "DEV", 0
$DIR_PIPE       BYTE    "PIPE", 0

SV_DATA         ENDS


SV_CODE         SEGMENT

                ASSUME  CS:SV_CODE, DS:NOTHING

;
; Implementation of __dup()
;
; In:   AX      File handle
;       BX      Pointer to process table entry
;
; Out:  AX      errno if CY set
;       AX      File handle if CY clear
;       CY      Error
;
; Find an available file handle, then use common code for
; DO_DUP and DO_DUP2.
;
                TALIGN  4
                ASSUME  DS:SV_DATA
                ASSUME  BX:PTR PROCESS
DO_DUP          PROC    NEAR
;
; Check the file handle
;
                CMP     AX, MAX_FILES
                JAE     SHORT DUP_EBADF
                MOV     SI, AX
                SHL     SI, 1
                CMP     [BX].P_HANDLES[SI], NO_FILE_HANDLE
                JE      SHORT DUP_EBADF
;
; Find an available file handle
;
                MOV     SI, 0
                MOV     CX, 0
DUP_LOOP:       CMP     [BX].P_HANDLES[SI], NO_FILE_HANDLE
                JE      DUP_FOUND
                ADD     SI, 2
                INC     CX
                CMP     CX, MAX_FILES
                JB      SHORT DUP_LOOP
                MOV     AX, EMFILE              ; Too many open files
                STC
                RET

DUP_EBADF:      MOV     AX, EBADF
                STC
                RET

;
; File handle found, call common code
;
DUP_FOUND:      CALL    DUP_COMMON
                CLC
                RET
                ASSUME  BX:NOTHING
DO_DUP          ENDP


;
; Implementation of __dup2()
;
; In:   AX      Source handle
;       CX      Target handle
;       BX      Pointer to process table entry
;
; Out:  AX      errno if CY set
;       AX      File handle if CY clear
;
; Check the file handles, then use common code for
; DO_DUP and DO_DUP2.
;
                TALIGN  4
                ASSUME  DS:SV_DATA
                ASSUME  BX:PTR PROCESS
DO_DUP2         PROC    NEAR
;
; Check the source file handle
;
                CMP     AX, MAX_FILES
                JAE     SHORT DUP2_EBADF
                MOV     SI, AX
                SHL     SI, 1
                CMP     [BX].P_HANDLES[SI], NO_FILE_HANDLE
                JE      SHORT DUP2_EBADF
;
; Do nothing if the two handles are identical
;
                CMP     AX, CX                  ; Handles identical?
                JE      SHORT DUP2_OK           ; Yes -> return the handle
;
; Check the target file handle.  Close the target handle if it is open
;
                CMP     CX, MAX_FILES
                JAE     SHORT DUP2_EBADF
                MOV     SI, CX
                SHL     SI, 1
                CMP     [BX].P_HANDLES[SI], NO_FILE_HANDLE
                JE      SHORT DUP2_CONT
                PUSH    AX
                PUSH    CX
                MOV     AX, CX
                CALL    DO_CLOSE
                POP     CX
                POP     DX
                JC      SHORT DUP2_RET          ; Error
                MOV     AX, DX
;
; Call common code
;
DUP2_CONT:      CALL    DUP_COMMON
DUP2_OK:        CLC
DUP2_RET:       RET

DUP2_EBADF:     MOV     AX, EBADF
                STC
                RET

                ASSUME  BX:NOTHING
DO_DUP2         ENDP


;
; Common code for DO_DUP and DO_DUP2, both handles have been checked,
; no error is possible.  The target handle is available.
;
; In:   AX      Source handle
;       CX      Target handle
;       BX      Pointer to process table entry
;
; Out:  AX      File handle
;
                TALIGN  4
                ASSUME  DS:SV_DATA
                ASSUME  BX:PTR PROCESS
DUP_COMMON      PROC    NEAR
                MOV     SI, AX
                SHL     SI, 1
                MOV     AX, [BX].P_HFLAGS[SI]
                MOV     DX, [BX].P_HANDLES[SI]
                MOV     SI, CX
                SHL     SI, 1
                MOV     [BX].P_HANDLES[SI], DX
                AND     AX, NOT HF_NOINHERIT    ; Clear FD_CLOEXEC
                MOV     [BX].P_HFLAGS[SI], AX
                MOV     SI, DX
                SHL     SI, 1
                INC     DOS_REF_COUNT[SI]
                MOV     AX, CX
                CALL    MAYBE_CHECK_HANDLES
                RET
                ASSUME  BX:NOTHING
DUP_COMMON      ENDP

;
; Close a file handle
;
; In:   AX      File handle
;       BX      Pointer to process table entry
;
; Out:  AX      errno if CY set
;       CY      Error
;
                TALIGN  4
                ASSUME  DS:SV_DATA
                ASSUME  BX:PTR PROCESS
DO_CLOSE        PROC    NEAR
                PUSH    DX
                PUSH    SI
                CMP     AX, MAX_FILES
                JAE     SHORT CLOSE_EBADF
                MOV     SI, AX
                SHL     SI, 1
                MOV     DX, [BX].P_HANDLES[SI]
                CMP     DX, NO_FILE_HANDLE
                JE      SHORT CLOSE_EBADF
                MOV     [BX].P_HANDLES[SI], NO_FILE_HANDLE
                MOV     [BX].P_HFLAGS[SI], 0
                MOV     SI, DX
                SHL     SI, 1
;
; The reference count should not be zero here.  Handle that case, anyway.
;
                CMP     DOS_REF_COUNT[SI], 0
                JE      SHORT CLOSE_CLOSE
                DEC     DOS_REF_COUNT[SI]
                JNZ     SHORT CLOSE_OK
;
; Close the DOS file handle
;
CLOSE_CLOSE:    PUSH    BX
                MOV     BX, DX
                MOV     AH, 3EH
                PUSH    PROCESS_PTR
                MOV     PROCESS_PTR, NO_PROCESS
                INT     21H
                POP     PROCESS_PTR
                POP     BX
                JMP     SHORT CLOSE_RET

CLOSE_OK:       CALL    MAYBE_CHECK_HANDLES
                XOR     AX, AX
CLOSE_RET:      POP     SI
                POP     DX
                RET

CLOSE_EBADF:    MOV     AX, EBADF
                STC
                JMP     SHORT CLOSE_RET
                ASSUME  BX:NOTHING
DO_CLOSE        ENDP


;
; Translate file handle (process PROCESS_PTR) to DOS file handle
;
; In:   AX      File handle
;       PROCESS_PTR
;
; Out:  AX      DOS file handle (NO_FILE_HANDLE if file not open)
;       CY      Not a valid handle
;
                TALIGN  4
                ASSUME  DS:SV_DATA
GET_HANDLE      PROC    NEAR
                CMP     AX, MAX_FILES
                JAE     SHORT GET_HANDLE_ERR
                PUSH    BX
                PUSH    SI
                MOV     SI, AX
                SHL     SI, 1
                MOV     BX, PROCESS_PTR
                MOV     AX, (PROCESS PTR [BX]).P_HANDLES[SI]
                POP     SI
                POP     BX
                CLC
                RET
GET_HANDLE_ERR: MOV     AX, NO_FILE_HANDLE
                STC
                RET
GET_HANDLE      ENDP


;
; Put new handle into handle translation table
;
; In:   AX      DOS file handle
;       PROCESS_PTR
;
; Out:  CY      No empty slot found
;       EAX     emx file handle
;
; Note:
;       When called with AX=NO_FILE_HANDLE, this function just looks for an
;       empty slot, but does not use that slot.
;

                TALIGN  4
                ASSUME  DS:SV_DATA
NEW_HANDLE      PROC    NEAR
                PUSH    BX
                PUSH    DX
                PUSH    SI
                MOV     BX, PROCESS_PTR
                MOV     SI, 0
                MOV     DX, 0
                ASSUME  BX:PTR PROCESS
                TALIGN  4
NH_LOOP:        CMP     [BX].P_HANDLES[SI], NO_FILE_HANDLE ; Empty slot?
                JE      SHORT NH_FOUND          ; Yes -> use it
                ADD     SI, 2                   ; Next slot
                INC     DX
                CMP     DX, MAX_FILES
                JB      SHORT NH_LOOP
NH_ERROR:       STC                             ; Out of file handles
                JMP     SHORT NH_RET

                TALIGN  4
NH_FOUND:       CMP     AX, NO_FILE_HANDLE
                JE      SHORT NH_OK
                CMP     AX, TOTAL_FILES
                JAE     SHORT NH_ERROR
                MOV     [BX].P_HANDLES[SI], AX  ; Store DOS file handle
                MOV     [BX].P_HFLAGS[SI], 0    ; Clear handle flags
                MOV     SI, AX
                SHL     SI, 1
                MOV     DOS_REF_COUNT[SI], 1
NH_OK:          MOVZX   EAX, DX
                CALL    MAYBE_CHECK_HANDLES
                CLC
NH_RET:         POP     SI
                POP     DX
                POP     BX
                RET
                ASSUME  BX:NOTHING
NEW_HANDLE      ENDP

;
; Remap file handles to make the DOS handle identical to the user process'
; handle.
;
; In:   AX      File handle
;       PROCESS_PTR
;
; Out:  AX      errno if CY is set
;       CY      Error
;
; TODO: Handle HF_NOINHERIT.
;
MH_HANDLE       EQU     (WORD PTR [BP-1*2])
MH_RELOC        EQU     (WORD PTR [BP-2*2])
MH_DOSHANDLE    EQu     (WORD PTR [BP-3*2])

                TALIGN  4
                ASSUME  DS:SV_DATA
MAP_HANDLE      PROC    NEAR
                PUSH    BX
                PUSH    CX
                PUSH    DX
                PUSH    SI
                PUSH    BP
                MOV     BP, SP
                SUB     SP, 3 * 2
                MOV     MH_HANDLE, AX
                MOV     BX, PROCESS_PTR
                ASSUME  BX:PTR PROCESS
                MOV     SI, AX
                SHL     SI, 1
                MOV     DX, [BX].P_HANDLES[SI]  ; Fetch curent DOS handle
                CMP     AX, DX                  ; Mapping required?
                JE      MH_OK                   ; No -> done
                MOV     MH_DOSHANDLE, DX
;
; Check whether we have to relocate the DOS handle.  It must be relocated
; if it is used.
;
                CMP     DOS_REF_COUNT[SI], 0    ; DOS handle used at all?
                JNE     SHORT MH_RELOCATE       ; Yes -> relocate
                CMP     AX, SWAP_HANDLE         ; Used for swap file?
                JE      SHORT MH_RELOCATE       ; Yes -> relocate
;
; Check whether the handle is used for an executable file
;
                LEA     BX, PROCESS_TABLE
                MOV     CX, MAX_PROCESSES
MH_EXEC_LOOP:   CMP     AX, [BX].P_EXEC_HANDLE
                JE      SHORT MH_RELOCATE
                ADD     BX, SIZE PROCESS
                LOOP    MH_EXEC_LOOP
                JMP     SHORT MH_NO_RELOC
;
; Relocate the DOS handle to make it available, updating the translation
; tables of all processes
;
MH_RELOCATE:    MOV     BX, MH_HANDLE
                ASSUME  BX:NOTHING
                MOV     AH, 45H
                PUSH    PROCESS_PTR
                MOV     PROCESS_PTR, NO_PROCESS
                INT     21H
                POP     PROCESS_PTR
                JC      MH_RET
                CMP     AX, TOTAL_FILES
                JAE     MH_ERROR
                MOV     MH_RELOC, AX            ; Relocated handle

                MOV     AH, 3EH                 ; Close the original handle
                PUSH    PROCESS_PTR
                MOV     PROCESS_PTR, NO_PROCESS
                INT     21H
                POP     PROCESS_PTR
                MOV     AX, MH_HANDLE
                MOV     DX, MH_RELOC
                CALL    MAP_TRANSLATE
;
; Move the DOS handle previously associated with MH_HANDLE to DOS handle
; MH_HANDLE.
;
MH_NO_RELOC:    MOV     BX, MH_DOSHANDLE        ; Is the handle associated
                CMP     BX, NO_FILE_HANDLE      ; with a DOS handle?
                JE      SHORT MH_OK             ; No  -> done (it's closed now)
                CMP     BX, MH_HANDLE           ; Same handle?
                JE      SHORT MH_OK             ; Yes -> done
                MOV     CX, MH_HANDLE
                MOV     AH, 46H
                PUSH    PROCESS_PTR
                MOV     PROCESS_PTR, NO_PROCESS
                INT     21H
                POP     PROCESS_PTR
                JC      SHORT MH_RET
;
; Update the P_HANDLES entry for this one handle, leaving the other entries
; of this process and other processes point to the duplicate made above.
; If there are no other entries, we can close the handle.
;
                MOV     BX, PROCESS_PTR
                ASSUME  BX:PTR PROCESS
                MOV     AX, MH_HANDLE
                MOV     SI, AX
                SHL     SI, 1
                MOV     [BX].P_HANDLES[SI], AX
                ASSUME  BX:NOTHING
                MOV     SI, AX
                SHL     SI, 1
                INC     DOS_REF_COUNT[SI]
;
; Update the reference count for the handle and check whether we can close it.
;
                MOV     SI, MH_DOSHANDLE
                SHL     SI, 1
                CMP     DOS_REF_COUNT[SI], 0
                JE      SHORT MH_CLOSE
                DEC     DOS_REF_COUNT[SI]
                JNZ     SHORT MH_OK
;
; The duplicate is no longer referenced, close it now.  Note that the
; handle can only be referenced by P_HANDLES.
;
MH_CLOSE:       MOV     BX, MH_DOSHANDLE
                MOV     AH, 3EH
                PUSH    PROCESS_PTR
                MOV     PROCESS_PTR, NO_PROCESS
                INT     21H
                POP     PROCESS_PTR

MH_OK:          CALL    MAYBE_CHECK_HANDLES
                XOR     AX, AX
MH_RET:         MOV     SP, BP
                POP     BP
                POP     SI
                POP     DX
                POP     CX
                POP     BX
                ASSUME  BX:NOTHING
                RET

MH_ERROR:       MOV     AX, EMFILE              ; ENFILE is not implemented
                STC
                JMP     SHORT MH_RET

MAP_HANDLE      ENDP


;
; Update handle translation tables of all processes and other file
; handles.  Also update DOS_REF_COUNT.
;
; In:   AX      Old DOS handle
;       DX      New DOS handle
;
                TALIGN  4
                ASSUME  DS:SV_DATA
MAP_TRANSLATE   PROC    NEAR
                LEA     BX, PROCESS_TABLE
                ASSUME  BX:PTR PROCESS
                MOV     DI, 0                   ; Number of changes
                MOV     CX, MAX_PROCESSES + 1   ; Include PROC0 (for OCHAR)!
MT_LOOP_PROC:   MOV     SI, 0
MT_LOOP_HANDLE: CMP     [BX].P_HANDLES[SI], AX
                JNE     SHORT MT_NO_UPDATE_1
                MOV     [BX].P_HANDLES[SI], DX
                INC     DI
MT_NO_UPDATE_1: ADD     SI, 2
                CMP     SI, 2 * MAX_FILES
                JB      SHORT MT_LOOP_HANDLE
                CMP     AX, [BX].P_EXEC_HANDLE  ; Executable file?
                JNE     SHORT MT_NO_UPDATE_2
                MOV     [BX].P_EXEC_HANDLE, DX
MT_NO_UPDATE_2: ADD     BX, SIZE PROCESS
                LOOP    MT_LOOP_PROC
;
; Update DOS_REF_COUNT
;
                MOV     SI, AX
                SHL     SI, 1
                SUB     DOS_REF_COUNT[SI], DI
                MOV     SI, DX
                SHL     SI, 1
                ADD     DOS_REF_COUNT[SI], DI
;
; Update other file handles
;
                CMP     AX, SWAP_HANDLE         ; Swap file?
                JNE     SHORT MT_NO_UPDATE_3
                MOV     SWAP_HANDLE, DX
MT_NO_UPDATE_3:
                RET
                ASSUME  BX:NOTHING
MAP_TRANSLATE   ENDP


;
; Remap file handles 0 through 2 of the current process to make the
; DOS handles identical to the current process' handle.  We don't
; remap all file handles because that would certainly run out of
; DOS handles: To pass closed handles as closed to the child process,
; all used handles must be moved out of 0..MAX_FILES-1, leaving no
; space for non-user handles.
;
; In:   PROCESS_PTR
;
; Out:  AX      errno if CY is set
;       CY      Error
;       
                TALIGN  4
                ASSUME  DS:SV_DATA
MAP_ALL_HANDLES PROC    NEAR
                PUSH    BX
                MOV     BX, 0
MAH_LOOP:       MOV     AX, BX
                CALL    MAP_HANDLE
                JC      SHORT MAH_RET
                INC     BX
                CMP     BX, 3
                JB      SHORT MAH_LOOP
                XOR     AX, AX
MAH_RET:        POP     BX
                RET
MAP_ALL_HANDLES ENDP


;
; Duplicate inheritable file handles for a child process
;
; In:   SI      Pointer to source process table entry (parent)
;       DI      Pointer to destination process table entry (child)
;
                ASSUME  DS:SV_DATA
                ASSUME  SI:PTR PROCESS
                ASSUME  DI:PTR PROCESS
INHERIT_HANDLES PROC    NEAR
                MOV     BX, 0
IH_LOOP:        MOV     AX, [SI].P_HANDLES[BX]  ; Get DOS handle
                MOV     DX, [SI].P_HFLAGS[BX]   ; Get handle flags
                TEST    DX, HF_NOINHERIT        ; Inherit?
                JNZ     SHORT IH_NOINHERIT      ; No  -> skip
                CMP     AX, NO_FILE_HANDLE      ; Valid handle?
                JE      SHORT IH_SET            ; No  -> don't update ref count
;
; Update the reference count
;
                PUSH    BX
                MOV     BX, AX
                SHL     BX, 1
                INC     DOS_REF_COUNT[BX]
                POP     BX
;
; Update the process table entry of the child
;
IH_SET:         MOV     [DI].P_HANDLES[BX], AX
                MOV     [DI].P_HFLAGS[BX], DX
                JMP     SHORT IH_NEXT

;
; Don't inherit the file handle
;
IH_NOINHERIT:   MOV     AX, NO_FILE_HANDLE
                MOV     DX, 0
                JMP     SHORT IH_SET

;
; Process next file handle
;
IH_NEXT:        ADD     BX, 2
                CMP     BX, 2 * MAX_FILES
                JB      IH_LOOP
;
; Redirect stderr to stdout if the -e option is given
;
                TEST    [DI].P_FLAGS, PF_REDIR_STDERR ; Redirect stderr?
                JE      SHORT IH_DONT_REDIR     ; No -> skip
                MOV     AX, 1                   ; stdout
                MOV     CX, 2                   ; stderr
                MOV     BX, DI
                CALL    DO_DUP2
IH_DONT_REDIR:  RET
                ASSUME  SI:NOTHING
                ASSUME  DI:NOTHING
INHERIT_HANDLES ENDP

;
; Close all open file handles of a process
;
; In:   BX      Pointer to process table entry
;
                ASSUME  DS:SV_DATA
                ASSUME  BX:PTR PROCESS
CLOSE_HANDLES   PROC    NEAR
                PUSH    SI
                MOV     SI, 0
CH_LOOP:        MOV     AX, SI
                CALL    DO_CLOSE
                INC     SI
                CMP     SI, MAX_FILES
                JB      CH_LOOP
                POP     SI
                RET
                ASSUME  BX:NOTHING
CLOSE_HANDLES   ENDP

;
; Dump handle tables for debugging
;
              IF FALSE
                ASSUME  DS:SV_DATA
DUMP_HANDLES    PROC    NEAR
                PUSHA
                LEA     BX, PROCESS_TABLE
                ASSUME  BX:PTR PROCESS
                MOV     CL, 0
DH_LOOP_PROC:   MOV     SI, 0
                MOV     AL, "0"
                CMP     CL, MAX_PROCESSES
                JE      SHORT DH_PROC0
                MOV     AL, CL
                ADD     AL, "1"
DH_PROC0:       CALL    OCHAR
                MOV     Al, ":"
                CALL    OCHAR
                MOV     AL, " "
                CALL    OCHAR
DH_LOOP_HANDLE: MOV     AX, [BX].P_HANDLES[SI]
                CMP     AX, NO_FILE_HANDLE
                JE      SHORT DH_NO_HANDLE
                AAM
                XCHG    AL, AH
                ADD     AL, "0"
                CALL    OCHAR
                XCHG    AL, AH
                ADD     AL, "0"
                CALL    OCHAR
                JMP     SHORT DH_NEXT_HANDLE

DH_NO_HANDLE:   MOV     AL, "-"
                CALL    OCHAR
                CALL    OCHAR
DH_NEXT_HANDLE: MOV     AL, " "
                CALL    OCHAR
                ADD     SI, 2
                CMP     SI, 2 * 24
                JB      SHORT DH_LOOP_HANDLE
                CALL    OCRLF
                ADD     BX, SIZE PROCESS
                INC     CL
                CMP     CL, MAX_PROCESSES + 1   ; Include PROC0
                JB      SHORT DH_LOOP_PROC
                POPA
                ASSUME  BX:NOTHING
                RET
DUMP_HANDLES    ENDP

              ENDIF

;
; Check reference counts
;
                ASSUME  DS:SV_DATA
CHECK_HANDLES   PROC    NEAR
                PUSHF
                PUSHA
                MOV     AX, 0
CH_LOOP_REF:    MOV     DX, 0
                MOV     CX, MAX_PROCESSES + 1   ; Include PROC0
                LEA     BX, PROCESS_TABLE
                ASSUME  BX:PTR PROCESS
CH_LOOP_PROC:   MOV     SI, 0
CH_LOOP_HANDLE: CMP     AX, [BX].P_HANDLES[SI]
                JNE     SHORT CH_NEXT_HANDLE
                INC     DX
CH_NEXT_HANDLE: ADD     SI, 2
                CMP     SI, 2 * MAX_FILES
                JB      SHORT CH_LOOP_HANDLE
                ADD     BX, SIZE PROCESS
                LOOP    CH_LOOP_PROC
                MOV     DI, AX
                SHL     DI, 1
                CMP     DOS_REF_COUNT[DI], DX
                JNE     SHORT CH_FAILURE
                INC     AX
                CMP     AX, TOTAL_FILES
                JB      CH_LOOP_REF
                ASSUME  BX:NOTHING
CH_RET:         POPA
                POPF
                RET
CH_FAILURE:     LEA     EDX, $CHECK_HANDLES
                CALL    OTEXT
                CALL    OCRLF
                JMP     SHORT CH_RET
CHECK_HANDLES   ENDP

;
; Call CHECK_HANDLES for -!1
;
                TALIGN  4
                ASSUME  DS:SV_DATA
MAYBE_CHECK_HANDLES PROC NEAR
                TEST    TEST_FLAGS, TEST_CHECK_HANDLES
                JZ      SHORT FIN
                CALL    CHECK_HANDLES
FIN:            RET
MAYBE_CHECK_HANDLES ENDP


;
; Truncate a path name
;
; In:   ES:EDI          Pathname
;       PROCESS_PTR     Pointer to process table entry
;

                ASSUME  DS:SV_DATA
                TALIGN  4
TRUNCATE        PROC    NEAR
                PUSHAD
                PUSH    FS
                MOV     BX, PROCESS_PTR         ; Get PTE of current process
                CMP     BX, NO_PROCESS          ; Is there a current process?
                JE      TRUNC_NO                ; No  -> do nothing
                MOV     ECX, -1                 ; Drive letter unknown
                MOV     AL, ES:[EDI]            ; Does the pathname start
                CMP     AL, "/"                 ; with / or \ ?
                JE      SHORT TRUNC_1           ; Yes -> look for special name
                CMP     AL, "\"
                JNE     SHORT TRUNC_CHECK       ; No  -> skip
TRUNC_1:        MOV     AL, ES:[EDI+1]          ; Check for UNC pathname
                CMP     AL, "/"
                JE      SHORT TRUNC_UNC         ; Yes -> UNC
                CMP     AL, "\"
                JE      SHORT TRUNC_UNC         ; Yes -> UNC
                LEA     BX, $DIR_DEV
                CALL    DIR_CHECK               ; \dev\ ?
                JZ      TRUNC_NO                ; Yes -> do nothing
                LEA     BX, $DIR_PIPE           ; (TODO: This is DOS, not OS/2)
                CALL    DIR_CHECK               ; \pipe\ ?
                JZ      TRUNC_NO                ; Yes -> do nothing
;
; We have a pathname that starts with / or \ and is not special.  Prepend
; the drive letter defined by the -r option, if -r is given.
;
                MOV     BX, PROCESS_PTR         ; Get PTE of current process
                ASSUME  BX:PTR PROCESS
                CMP     [BX].P_DRIVE, 0         ; Prepend drive?
                JE      SHORT TRUNC_CHECK       ; No ->
;
; Prepend drive letter
;
                MOV     DL, [BX].P_DRIVE        ; Get drive letter (A-Z)
                MOV     CL, DL
                AND     ECX, 1FH                ; Bit for -t option
                ASSUME  BX:NOTHING
                MOV     DH, ":"                 ; Insert drive and colon
                PUSH    EDI
DRIVE_MOVE:     MOV     AL, DL                  ; Insert two bytes
                XCHG    DL, ES:[EDI]
                INC     EDI
                XCHG    DL, DH
                TEST    AL, AL
                JNZ     SHORT DRIVE_MOVE
                POP     EDI
                JMP     SHORT TRUNC_CHECK

;
; It's a UNC pathname
;
TRUNC_UNC:      MOV     ECX, 0                  ; Check bit 0
                JMP     SHORT TRUNC_CHECK

;
; Check if we should truncate the pathname.
;
TRUNC_CHECK:    MOV     BX, PROCESS_PTR         ; Get PTE of current process
                ASSUME  BX:PTR PROCESS
                CMP     [BX].P_TRUNC, 0         ; Never truncate?
                JE      TRUNC_NO                ; Yes -> do nothing
                CMP     [BX].P_TRUNC, -1        ; Always truncate?
                JE      SHORT TRUNC_YES         ; Yes -> truncate
                CMP     ECX, -1                 ; Drive known?
                JNE     SHORT TRUNC_CHECK_2     ; Yes -> skip
;
; Try to get the drive letter from the pathname
;
                MOV     AL, ES:[EDI]
                SUB     AL, "A"
                CMP     AL, 26
                JB      SHORT TRUNC_CHECK_1
                MOV     AL, ES:[EDI]
                SUB     AL, "a"
                CMP     AL, 26
                JAE     SHORT TRUNC_CURDISK
TRUNC_CHECK_1:  CMP     BYTE PTR ES:[EDI+1], ":"
                JE      SHORT TRUNC_PLUS1
;
; We don't know the drive letter -- use the current disk
;
TRUNC_CURDISK:  MOV     AH, 19H
                INT     21H
TRUNC_PLUS1:    MOV     CL, AL
                INC     CL
                AND     ECX, 1FH
;
; Check the bit specified by ECX
;
TRUNC_CHECK_2:  BT      [BX].P_TRUNC, ECX       ; Truncate for this drive?
                JNC     SHORT TRUNC_NO          ; No  -> do nothing
                ASSUME  BX:NOTHING
TRUNC_YES:      PUSH    DS
                MOV     ESI, EDI                ; Setup source pointer
                MOV_FS_DS                       ; Access DBCS_LEAD_TAB with FS
                ASSUME  FS:SV_DATA
                MOV_DS_ES
                ASSUME  DS:NOTHING
                JMP     SHORT TRUNC_NAME        ; At start of name

                TALIGN  4
TRUNC_LOOP:     LODS    BYTE PTR DS:[ESI]       ; Fetch character
                TEST    AL, AL                  ; End?
                JE      SHORT TRUNC_END         ; Yes -> done
                CMP     AL, ":"
                JE      SHORT TRUNC_DIR
                CMP     AL, "\"
                JE      SHORT TRUNC_DIR
                CMP     AL, "/"
                JE      SHORT TRUNC_DIR
                CMP     AL, "."
                JE      SHORT TRUNC_EXT
                MOV     AH, 0
                BT      DBCS_LEAD_TAB, AX       ; DBCS lead byte?
                JC      SHORT TRUNC_DBCS
TRUNC_STORE:    TEST    CL, CL                  ; Beyond maximum length?
                JZ      SHORT TRUNC_LOOP        ; Yes -> don't store
                STOS    BYTE PTR ES:[EDI]       ; Store character
                DEC     CL                      ; Adjust length counter
                JMP     SHORT TRUNC_LOOP        ; Next character

TRUNC_DBCS:     CMP     BYTE PTR DS:[ESI], 0    ; Invalid DBCS char?
                JE      SHORT TRUNC_STORE       ; Yes -> store anyway
                INC     ESI                     ; Skip 2nd byte
                CMP     CL, 2                   ; Can we store 2 bytes?
                JB      SHORT TRUNC_LOOP        ; No -> drop both bytes
                STOS    BYTE PTR ES:[EDI]       ; Store 1st byte
                MOV     AL, DS:[ESI-1]
                STOS    BYTE PTR ES:[EDI]       ; Store 2nd byte
                SUB     CL, 2                   ; Adjust length counter
                JMP     SHORT TRUNC_LOOP        ; Next character

TRUNC_DIR:      STOS    BYTE PTR ES:[EDI]       ; Store character
TRUNC_NAME:     MOV     CX, 0008H               ; Extension not seen, 8 chars
                JMP     SHORT TRUNC_LOOP        ; Next character

TRUNC_EXT:      CMP     CX, 0103H               ; Previous character a dot?
                JE      SHORT TRUNC_DOTS        ; Yes -> keep ".."
                TEST    CH, CH                  ; Extension seen?
                JNZ     SHORT TRUNC_STOP        ; Yes -> stop storing
TRUNC_DOTS:     STOS    BYTE PTR ES:[EDI]       ; Store dot character
                MOV     CX, 0103H               ; Extension seen, 3 characters
                JMP     SHORT TRUNC_LOOP        ; Next character

TRUNC_STOP:     MOV     CL, 0                   ; Don't store characters
                JMP     SHORT TRUNC_LOOP        ; Next character

TRUNC_END:      STOS    BYTE PTR ES:[EDI]
                POP     DS
TRUNC_NO:       POP     FS
                POPAD
                RET
TRUNCATE        ENDP

                ASSUME  FS:NOTHING

                TALIGN  4
DIR_CHECK       PROC    NEAR
                PUSH    EDI
DIR_CHECK_1:    INC     EDI
                MOV     AL, ES:[EDI]
                CMP     BYTE PTR [BX], 0
                JE      SHORT DIR_CHECK_2
                CALL    UPPER
                CMP     AL, [BX]
                JNE     SHORT DIR_CHECK_8
                INC     BX
                JMP     SHORT DIR_CHECK_1

DIR_CHECK_2:    CMP     AL, "/"
                JE      SHORT DIR_CHECK_8
                CMP     AL, "\"
DIR_CHECK_8:    POP     EDI
                RET
DIR_CHECK       ENDP

SV_CODE         ENDS


INIT_CODE       SEGMENT

                ASSUME  CS:INIT_CODE, DS:NOTHING

;
; Initialize the DOS_REF_COUNT table and the file handles of PROC0.
;
; To speed things up, assume that at most handles 0, 1, and 2 are
; inherited.
;
                ASSUME  DS:SV_DATA
INIT_FILEIO     PROC    NEAR
;
; Initially, no file is referenced.
;
                MOV     BX, 0
IF_ZERO_REF:    MOV     DOS_REF_COUNT[BX], 0
                ADD     BX, 2
                CMP     BX, 2 * TOTAL_FILES
                JB      IF_ZERO_REF
;
; Initialize the P_HANDLES array of PROC0, setting reference counts
; as required.
;
                MOV     BX, 0                   ; File handle
                MOV     DI, 0                   ; Index
IF_LOOP:        MOV     PROC0.P_HFLAGS[DI], 0   ; Clear handle flags
                MOV     AX, 4400H               ; Get device data
                INT     21H
                JC      SHORT IF_NOT_OPEN
                MOV     PROC0.P_HANDLES[DI], BX
                INC     DOS_REF_COUNT[DI]
                JMP     SHORT IF_NEXT

IF_NOT_OPEN:    MOV     PROC0.P_HANDLES[DI], NO_FILE_HANDLE
IF_NEXT:        ADD     DI, 2
                INC     BX
                CMP     BX, MAX_FILES           ; File handles per process!
                JB      SHORT IF_LOOP
                RET
INIT_FILEIO     ENDP

INIT_CODE       ENDS

                END
