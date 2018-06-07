;
; SWAPPER.INC -- Swap memory pages in and out
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

SV_DATA         SEGMENT

                EXTRN   SWAP_HANDLE:WORD
                EXTRN   SWAP_FLAG:BYTE
                EXTRN   SWAP_FAULTS:DWORD
                EXTRN   SWAP_READS:DWORD
                EXTRN   SWAP_WRITES:DWORD
                EXTRN   SWAP_SIZE:DWORD
                EXTRN   SNATCH_COUNT:DWORD

SV_DATA         ENDS


SV_CODE         SEGMENT

                EXTRN   PAGE_FAULT:NEAR         ; Page fault exception
                EXTRN   SWAP_OUT:NEAR           ; Swap a page out
                EXTRN   INIT_SWAP:NEAR          ; Initialize swapper
                EXTRN   ALLOC_SWAP:NEAR         ; Allocate swap space

SV_CODE         ENDS


INIT_CODE       SEGMENT

                EXTRN   CLEANUP_SWAP:NEAR
                EXTRN   SET_TMP_DIR:NEAR

INIT_CODE       ENDS
