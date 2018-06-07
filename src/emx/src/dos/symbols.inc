;
; SYMBOLS.INC -- Manage symbol tables
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

NLIST           STRUCT
N_STRING        DD      ?
N_TYPE          DB      ?
N_OTHER         DB      ?
N_DESC          DW      ?
N_VALUE         DD      ?
NLIST           ENDS

N_EXT           =       01H
N_TEXT          =       04H
N_DATA          =       06H
N_BSS           =       08H
N_SLINE         =       44H

SYM_ANY         =       0               ; SYM_TEXT or SYM_DATA
SYM_TEXT        =       1
SYM_DATA        =       2
SYM_LINE        =       3
SYM_NONE        =       0FFH                    ; for disassembler

              IFNDEF __SYMBOLS

SV_CODE         SEGMENT

                EXTRN   SYM_BY_ADDR:NEAR
                EXTRN   SYM_BY_NAME:NEAR
                EXTRN   SYM_MODULE:NEAR
                EXTRN   SYM_BEFORE:NEAR

SV_CODE         ENDS

              ENDIF
