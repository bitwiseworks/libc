;
; VCPI.INC -- Virtual Control Program Interface
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

                EXTRN   VCPI_FLAG:BYTE, DV_FLAG:BYTE
                EXTRN   V2P_LIN:DWORD, V2P_CR3:DWORD
                EXTRN   V2P_GDTR:DWORD, V2P_IDTR:DWORD

SV_DATA         ENDS


SV_CODE         SEGMENT

                EXTRN   INT_VCPI:NEAR
                EXTRN   VCPI_ALLOC:NEAR
                EXTRN   VCPI_AVAIL:NEAR

SV_CODE         ENDS


INIT_CODE       SEGMENT

                EXTRN   START_VCPI:NEAR, INT_BACK_VCPI:NEAR
                EXTRN   INIT_VCPI:NEAR, CHECK_VCPI:NEAR, CLEANUP_VCPI:NEAR
                EXTRN   GET_INT_MAP:NEAR, SET_INT_MAP:NEAR, CHECK_VM:NEAR
                EXTRN   VCPI_LIN_TO_PHYS:NEAR

INIT_CODE       ENDS
