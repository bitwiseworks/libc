;
; NDEBUG.ASM -- No debugger
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

                INCLUDE EMX.INC
                INCLUDE PMINT.INC

                PUBLIC  STEP_FLAG, DEBUG_AVAIL, DEBUG_SER_FLAG, DEBUG_SER_PORT
                PUBLIC  DEBUG_EXCEPTION, SET_BREAKPOINT, INS_BREAKPOINTS
                PUBLIC  DEBUG_RESUME, DEBUG_STEP, DEBUG_QUIT, DEBUG_INIT

SV_DATA         SEGMENT

STEP_FLAG       DB      FALSE
DEBUG_AVAIL     DB      FALSE
DEBUG_SER_FLAG  DB      FALSE
DEBUG_SER_PORT  DW      0

SV_DATA         ENDS

SV_CODE         SEGMENT

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

DEBUG_EXCEPTION PROC    NEAR
                RET                             ; Return and dump registers
DEBUG_EXCEPTION ENDP

SET_BREAKPOINT  PROC    NEAR
                RET
SET_BREAKPOINT  ENDP

INS_BREAKPOINTS PROC    NEAR
                RET
INS_BREAKPOINTS ENDP

DEBUG_INIT      PROC    NEAR
                RET
DEBUG_INIT      ENDP

DEBUG_QUIT      PROC    NEAR
                RET
DEBUG_QUIT      ENDP

SV_CODE         ENDS

                END
