;
; FILEIO.INC -- Handle files and file handles
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

SV_CODE         SEGMENT

                EXTRN   DO_DUP:NEAR
                EXTRN   DO_DUP2:NEAR
                EXTRN   DO_CLOSE:NEAR
                EXTRN   GET_HANDLE:NEAR
                EXTRN   NEW_HANDLE:NEAR
                EXTRN   MAP_HANDLE:NEAR
                EXTRN   MAP_ALL_HANDLES:NEAR
                EXTRN   INHERIT_HANDLES:NEAR
                EXTRN   CLOSE_HANDLES:NEAR
                EXTRN   CHECK_HANDLES:NEAR
                EXTRN   TRUNCATE:NEAR           ; Truncate path name

SV_CODE         ENDS

INIT_CODE       SEGMENT

                EXTRN   INIT_FILEIO:NEAR        ; Initialize file table

INIT_CODE       ENDS
