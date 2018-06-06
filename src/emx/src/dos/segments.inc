;
; SEGMENTS.INC -- Manage segments
;
; Copyright (c) 1991-1999 by Eberhard Mattes
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

A_DATA32        =   4093H   ; Accessed, writable, expand up, big
A_READ32        =   4091H   ; Accessed, expand up, big
A_CODE32        =   409BH   ; Accessed, readable, non-conforming, default=32
A_LDT           =   0082H   ; LDT
A_TSS           =   0089H   ; TSS

DPL_0           =   0000H
DPL_1           =   0020H
DPL_2           =   0040H
DPL_3           =   0060H


              IFNDEF __SEGMENTS

SV_DATA         SEGMENT

                EXTRN   NEW_TSS:BYTE
                EXTRN   G_PHYS_BASE:DWORD

SV_DATA         ENDS

SV_CODE         SEGMENT

                EXTRN   CREATE_SEG:NEAR, SV_SEGMENT:NEAR
                EXTRN   SEG_SIZE:NEAR, SEG_BASE:NEAR, SEG_ATTR:NEAR
                EXTRN   GET_BASE:NEAR, NULL_SEG:NEAR, GET_LIN:NEAR
                EXTRN   GET_DESC:NEAR, INIT_TSS:NEAR, ADD_PAGES:NEAR
                EXTRN   ACCESS_LOWMEM:NEAR, MAP_PHYS:NEAR

SV_CODE         ENDS

INIT_CODE       SEGMENT

                EXTRN   INIT_DESC:NEAR
                EXTRN   RM_SEG_BASE:NEAR
                EXTRN   RM_SEG_SIZE:NEAR

INIT_CODE       ENDS

              ENDIF
