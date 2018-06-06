;
; PROFIL.INC -- Implement profil()
;
; Copyright (c) 1995 by Eberhard Mattes
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
; Cf. struct _profil in <sys/emx.h>
;
PROFIL          STRUC
PRF_CB          DWORD   ?               ; Size of structure
PRF_BUFF        DWORD   ?               ; Pointer to buffer
PRF_BUFSIZ      DWORD   ?               ; Size of buffer
PRF_OFFSET      DWORD   ?               ; Start address
PRF_SCALE       DWORD   ?               ; Scaling factor
PROFIL          ENDS

              IFNDEF __PROFIL

SV_DATA         SEGMENT
                EXTRN   PROFIL_COUNT:WORD
SV_DATA         ENDS

SV_CODE         SEGMENT

                EXTRN   PROFIL_STOP:NEAR
                EXTRN   PROFIL_SUSPEND:NEAR
                EXTRN   PROFIL_RESUME:NEAR
                EXTRN   PROFIL_TICK:NEAR
                EXTRN   DO_PROFIL:NEAR

SV_CODE         ENDS

              ENDIF
