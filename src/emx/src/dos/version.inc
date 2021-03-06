;
; VERSION.INC -- Version number and revision index
;
; Copyright (c) 1991-2000 by Eberhard Mattes
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

VERSION         EQU     <"0.9d">                ; No comment
REV_INDEX_TXT   EQU     <"61">                  ; Revision index (as text)
REV_INDEX_BIN   =       61                      ; Revision index (as number)

HDR_EMX_LEN     =       4                       ; "emx "
HDR_VERSION_LEN =       9                       ; "emx 0.9d" plus null char
