;
; ERRORS.ASM -- Translate error codes
;
; Copyright (c) 1991-1995 by Eberhard Mattes
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

__ERRORS        =       1
                INCLUDE EMX.INC
                INCLUDE ERRORS.INC
                INCLUDE SIGNAL.INC
                INCLUDE PROCESS.INC
                INCLUDE PMINT.INC

                PUBLIC  DOS_ERROR_TO_ERRNO, XLATE_ERRNO

SV_DATA         SEGMENT

ERRNO_TAB       LABEL   BYTE
                DB      EINVAL, EINVAL, ENOENT, ENOENT, EMFILE  ; 0..4
                DB      EACCES, EBADF,  EIO,    ENOMEM, EIO     ; 5..9
                DB      EINVAL, ENOEXEC,EINVAL, EINVAL, EINVAL  ; 10..14
                DB      EACCES, EBUSY,  EXDEV,  ENOENT, EIO     ; 15..19
                DB      EIO,    EIO,    EIO,    EIO,    EIO     ; 20..24
                DB      EIO,    EIO,    EIO,    ENOSPC, EIO     ; 25..29
                DB      EIO,    EIO,    EACCES, EACCES, EIO     ; 30..34
                DB      EIO,    EIO,    EIO,    EIO,    ENOSPC  ; 35..39
                DB      EIO,    EIO,    EIO,    EIO,    EIO     ; 40..44
                DB      EIO,    EIO,    EIO,    EIO,    EIO     ; 45..49
                DB      EIO,    EIO,    EIO,    EIO,    EBUSY   ; 50..54
                DB      EIO,    EIO,    EIO,    EIO,    EIO     ; 55..59
                DB      EIO,    ENOSPC, ENOSPC, EIO,    EIO     ; 60..64
                DB      EACCES, EIO,    EIO,    EIO,    EIO     ; 65..69
                DB      EIO,    EIO,    EIO,    EACCES, EIO     ; 70..74
                DB      EIO,    EIO,    EIO,    EIO,    EIO     ; 75..79
                DB      EEXIST, EIO,    ENOENT, EIO,    EIO     ; 80..84
                DB      EIO,    EIO,    EINVAL, EIO             ; 85..88
ERRNO_TAB_LEN   =       THIS BYTE - ERRNO_TAB
                .ERRE   ERRNO_TAB_LEN-1 EQ 88

; This table translates errno values for `interface 0'.  Changed
; entries are marked (*).

ERRNO_INTERFACE_0 LABEL   BYTE
                DB      0               ; 0:  0
                DB      EACCES          ; 1:  EPERM (*)
                DB      ENOENT          ; 2:  ENOENT
                DB      ESRCH           ; 3:  ESRCH
                DB      EINTR           ; 4:  EINTR
                DB      EIO             ; 5:  EIO
                DB      EINVAL          ; 6:  ENXIO (*)
                DB      E2BIG           ; 7:  E2BIG
                DB      ENOEXEC         ; 8:  ENOEXEC
                DB      EBADF           ; 9:  EBADF
                DB      ECHILD          ; 10: ECHILD
                DB      EAGAIN          ; 11: EAGAIN
                DB      ENOMEM          ; 12: ENOMEM
                DB      EACCES          ; 13: EACCES
                DB      EINVAL          ; 14: EFAULT (*)
                DB      EINVAL          ; 15: ENOLCK (*)
                DB      EACCES          ; 16: EBUSY (*)
                DB      EEXIST          ; 17: EEXIST
                DB      EXDEV           ; 18: EXDEV
                DB      EINVAL          ; 19: ENODEV (*)
                DB      EACCES          ; 20: ENOTDIR (*)
                DB      EACCES          ; 21: EISDIR (*)
                DB      EINVAL          ; 22: EINVAL
                DB      EMFILE          ; 23: ENFILE (*)
                DB      EMFILE          ; 24: EMFILE
                DB      EINVAL          ; 25: ENOTTY (*)
                DB      EINVAL          ; 26: EDEADLK (*)
                DB      EINVAL          ; 27: EFBIG (*)
                DB      ENOSPC          ; 28: ENOSPC
                DB      EINVAL          ; 29: ESPIPE (*)
                DB      EACCES          ; 30: EROFS (*)
                DB      EINVAL          ; 31: EMLINK (*)
                DB      EPIPE           ; 32: EPIPE
                DB      EINVAL          ; 33: EDOM (*)
                DB      ERANGE          ; 34: ERANGE
                DB      EACCES          ; 35: ENOTEMPTY (*)
                DB      EINVAL          ; 36: EINPROGRESS (*)
                DB      ENOSYS          ; 37: ENOSYS
                DB      EINVAL          ; 38: ENAMETOOLONG (*)

; This table translates errno values for `interface 1' (up to and
; including 0.8h).  Changed entries are marked (*).

ERRNO_INTERFACE_1 LABEL   BYTE
                DB      0               ; 0:  0
                DB      EPERM           ; 1:  EPERM
                DB      ENOENT          ; 2:  ENOENT
                DB      ESRCH           ; 3:  ESRCH
                DB      EINTR           ; 4:  EINTR
                DB      EIO             ; 5:  EIO
                DB      EINVAL          ; 6:  ENXIO (*)
                DB      E2BIG           ; 7:  E2BIG
                DB      ENOEXEC         ; 8:  ENOEXEC
                DB      EBADF           ; 9:  EBADF
                DB      ECHILD          ; 10: ECHILD
                DB      EAGAIN          ; 11: EAGAIN
                DB      ENOMEM          ; 12: ENOMEM
                DB      EACCES          ; 13: EACCES
                DB      EINVAL          ; 14: EFAULT (*)
                DB      EINVAL          ; 15: ENOLCK (*)
                DB      EACCES          ; 16: EBUSY (*)
                DB      EEXIST          ; 17: EEXIST
                DB      EXDEV           ; 18: EXDEV
                DB      EINVAL          ; 19: ENODEV (*)
                DB      ENOTDIR         ; 20: ENOTDIR
                DB      EISDIR          ; 21: EISDIR
                DB      EINVAL          ; 22: EINVAL
                DB      EMFILE          ; 23: ENFILE (*)
                DB      EMFILE          ; 24: EMFILE
                DB      EINVAL          ; 25: ENOTTY (*)
                DB      EINVAL          ; 26: EDEADLK (*)
                DB      EINVAL          ; 27: EFBIG (*)
                DB      ENOSPC          ; 28: ENOSPC
                DB      ESPIPE          ; 29: ESPIPE
                DB      EROFS           ; 30: EROFS
                DB      EINVAL          ; 31: EMLINK (*)
                DB      EPIPE           ; 32: EPIPE
                DB      EDOM            ; 33: EDOM
                DB      ERANGE          ; 34: ERANGE
                DB      EACCES          ; 35: ENOTEMPTY (*)
                DB      EINVAL          ; 36: EINPROGRESS (*)
                DB      ENOSYS          ; 37: ENOSYS
                DB      ENAMETOOLONG    ; 38: ENAMETOOLONG

SV_DATA         ENDS


SV_CODE         SEGMENT

                ASSUME  CS:SV_CODE, DS:NOTHING

;
; Translate DOS error code to errno number
;
; In:   AX      DOS error code
;
; Out:  EAX     errno value
;       BX      modified
;
                ASSUME  DS:SV_DATA
                TALIGN  4
DOS_ERROR_TO_ERRNO PROC NEAR
                CMP     AX, ERRNO_TAB_LEN       ; My table ends here
                JAE     SHORT ERRNO_1
                LEA     BX, ERRNO_TAB
                ADD     BX, AX
                MOVZX   EAX, BYTE PTR [BX]
                CALL    XLATE_ERRNO
                RET

ERRNO_1:        MOV     EAX, EINVAL             ; Invalid argument
                RET
DOS_ERROR_TO_ERRNO ENDP

;
; Translate an errno value according to the interface.  This function
; should be called when returning an errno value marked (*) in the
; ERRNO_INTERFACE_0 table.
;
; In:   AX      errno value
;       PROCESS_PTR
;
; Out:  EAX     errno value
;       BX      modified
;
                ASSUME  DS:SV_DATA
                TALIGN  4
XLATE_ERRNO     PROC    NEAR
                MOV     BX, PROCESS_PTR
                CMP     BX, NO_PROCESS
                JE      SHORT FIN
                ASSUME  BX:PTR PROCESS
                CMP     [BX].P_INTERFACE, 0
                JE      SHORT XE_0
                CMP     [BX].P_INTERFACE, 1
                ASSUME  BX:NOTHING
                JNE     SHORT FIN
XE_1:           LEA     BX, ERRNO_INTERFACE_1
                JMP     SHORT XE_TRANS
XE_0:           LEA     BX, ERRNO_INTERFACE_0
XE_TRANS:       ADD     BX, AX
                MOVZX   EAX, BYTE PTR [BX]
FIN:            RET
XLATE_ERRNO     ENDP


SV_CODE         ENDS

                END
