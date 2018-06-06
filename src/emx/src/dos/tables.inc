;
; TABLES.INC -- Protected mode tables
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

LDT_ENTRIES     =       6


;
; Task State Segment
;
; Without I/O bitmap, at least one bytes (0FFH) required after TSS
;
TSS_STRUC       STRUC
TSS_BACK        DD      ?                       ; Back pointer
TSS_STK0_OFF    DD      ?                       ; STK0: offset
TSS_STK0_SEL    DW      ?, 0                    ; STK0: selector
TSS_STK1_OFF    DD      ?                       ; STK1: offset
TSS_STK1_SEL    DW      ?, 0                    ; STK1: selector
TSS_STK2_OFF    DD      ?                       ; STK2: offset
TSS_STK2_SEL    DW      ?, 0                    ; STK2: selector
TSS_CR3         DD      ?                       ; CR3
TSS_EIP         DD      ?                       ; EIP
TSS_EFLAGS      DD      ?                       ; EFLAGS
TSS_EAX         DD      ?                       ; EAX
TSS_ECX         DD      ?                       ; ECX
TSS_EDX         DD      ?                       ; EDX
TSS_EBX         DD      ?                       ; EBX
TSS_ESP         DD      ?                       ; ESP
TSS_EBP         DD      ?                       ; EBP
TSS_ESI         DD      ?                       ; ESI
TSS_EDI         DD      ?                       ; EDI
TSS_ES          DW      ?, 0                    ; ES
TSS_CS          DW      ?, 0                    ; CS
TSS_SS          DW      ?, 0                    ; SS
TSS_DS          DW      ?, 0                    ; DS
TSS_FS          DW      ?, 0                    ; FS
TSS_GS          DW      ?, 0                    ; GS
TSS_LDT         DD      ?                       ; LDT
TSS_TRAP        DW      ?                       ; Bit 0: trap flag
TSS_BIT_MAP_OFF DW      ?                       ; I/O bitmap
TSS_STRUC       ENDS



              IFNDEF __TABLES

SV_DATA         SEGMENT

                EXTRN   IDT:QWORD, GDT:QWORD, LDTS:QWORD
                EXTRN   BASE_TABLE:WORD
                EXTRN   GDT_PTR:FWORD, IDT_PTR:FWORD, REAL_IDT_PTR:FWORD
                EXTRN   GDT_LIN:DWORD, IDT_LIN:DWORD
                EXTRN   G_VCPI_DESC:QWORD, G_ENV_DESC:QWORD
                EXTRN   G_TMP1_DESC:QWORD, G_TMP2_DESC:QWORD
                EXTRN   G_BUF1_DESC:QWORD, G_BUF2_DESC:QWORD
                EXTRN   G_SWAP_BMP_DESC:QWORD, G_VIDEO_DESC:QWORD
                EXTRN   G_LIN_MAP_DESC:QWORD, G_PAGE_BMP_DESC:QWORD
                EXTRN   G_TSS_DESC:QWORD, G_TSS_MEM_DESC:QWORD
                EXTRN   TSS_BUSY:BYTE
                EXTRN   TSS_EX8_CR3:DWORD, TSS_EX10_CR3:DWORD
                EXTRN   G_PHYS_DESC:DESCRIPTOR
                EXTRN   G_PAGEDIR_DESC:DESCRIPTOR
SV_DATA         ENDS

              ENDIF
