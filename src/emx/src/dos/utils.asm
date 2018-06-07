;
; UTILS.ASM -- Utility functions
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

                PUBLIC  NSTRCPY, UPPER

SV_CODE         SEGMENT

                ASSUME  CS:SV_CODE, DS:NOTHING

;
; Copy a null-terminated string
;
; In:  DS:SI    Pointer to source string
;      DS:DI    Pointer to destination array
;
; Out: AX       Points to terminating null byte of the destination string
;
NSTRCPY         PROC    NEAR
                PUSH    SI
                PUSH    DI
                TALIGN  4
NSC_LOOP:       LODS    BYTE PTR DS:[SI]
                MOV     [DI], AL
                INC     DI
                TEST    AL, AL
                JNZ     SHORT NSC_LOOP
                LEA     AX, [DI-1]
                POP     DI
                POP     SI
                RET
NSTRCPY         ENDP

;
; Convert a letter (a..z) to upper case
;
; In:   AL      Character
;
; Out:  AL      Character
;
                TALIGN  4
UPPER           PROC    NEAR
                CMP     AL, "a"
                JB      SHORT UPPER_RET
                CMP     AL, "z"
                JA      SHORT UPPER_RET
                SUB     AL, "a" - "A"
UPPER_RET:      RET
UPPER           ENDP

SV_CODE         ENDS

                END
