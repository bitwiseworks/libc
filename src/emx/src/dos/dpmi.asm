;
; DPMI.ASM -- Do not support DPMI
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
                INCLUDE RPRINT.INC
                INCLUDE VCPI.INC
                INCLUDE MISC.INC

                PUBLIC  CHECK_DPMI

SV_DATA         SEGMENT

$DPMI_NOT_SUPP  DB      "DPMI not supported", CR, LF, 0

SV_DATA         ENDS

INIT_CODE       SEGMENT

                ASSUME  CS:INIT_CODE, DS:NOTHING

                ASSUME  DS:SV_DATA
CHECK_DPMI      PROC    NEAR
                CMP     VCPI_FLAG, FALSE
                JNE     SHORT NO_DPMI
                MOV     AX, 1687H
                INT     2FH
                OR      AX, AX
                JNZ     SHORT NO_DPMI
                LEA     DX, $DPMI_NOT_SUPP
                CALL    RTEXT
                MOV     AL, 0FFH
                JMP     EXIT

NO_DPMI:        RET
CHECK_DPMI      ENDP


INIT_CODE       ENDS

                END
