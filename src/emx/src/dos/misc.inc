;
; MISC.INC -- Miscellaneous definitions (for emx.asm)
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

SV_STACK        SEGMENT

                EXTRN   SV_TOS:WORD

SV_STACK        ENDS


RM_STACK        SEGMENT

                EXTRN   RM_TOS:WORD

RM_STACK        ENDS


SV_DATA         SEGMENT

                EXTRN   FP_FLAG:BYTE
                EXTRN   FP_TMP:WORD
                EXTRN   DOS_MAJOR:BYTE
                EXTRN   DOS_MINOR:BYTE
                EXTRN   ENV_PATH:WORD
                EXTRN   ENV_EMXPATH:WORD
                EXTRN   ENV_EMXOPT:WORD
                EXTRN   CMDL_OPTIONS:BYTE

SV_DATA         ENDS


SV_CODE         SEGMENT

                EXTRN   PROT1:NEAR

SV_CODE         ENDS


INIT_CODE       SEGMENT

                EXTRN   EXIT:NEAR               ; Cleanup and exit
                EXTRN   CLEANUP:NEAR
                EXTRN   RM_OUT_OF_MEM:NEAR

INIT_CODE       ENDS
