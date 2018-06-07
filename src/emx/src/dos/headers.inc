;
; HEADERS.INC -- Header structures
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

;
; emxbind header
;
BIND_HEADER     STRUC
BND_SIGNATURE   BYTE    16 DUP (?)
BND_BIND_FLAG   BYTE    FALSE
BND_FILL_1      BYTE    ?
BND_HDR_LOC     DWORD   ?
BND_OPTIONS     BYTE    64 DUP (0)
BIND_HEADER     ENDS

;
; EXE header
;
EXE1_HEADER     STRUC
EXE_MAGIC       DW      ?
EXE_LAST_PAGE   DW      ?
EXE_PAGES       DW      ?
EXE_RELOC_SIZE  DW      ?
EXE_HDR_SIZE    DW      ?
EXE_MIN_ALLOC   DW      ?
EXE_MAX_ALLOC   DW      ?
EXE_SS          DW      ?
EXE_SP          DW      ?
EXE_CHKSUM      DW      ?
EXE_IP          DW      ?
EXE_CS          DW      ?
EXE_RELOC_PTR   DW      ?
EXE_OVL         DW      ?
EXE1_HEADER     ENDS

;
; Additional data of EXE header
;
EXE2_HEADER     STRUC
EXE_RES1        DW      16 DUP (?)
EXE_NEW_LO      DW      ?
EXE_NEW_HI      DW      ?
EXE2_HEADER     ENDS

;
; a.out header
;
A_OUT_HEADER    STRUC
A_MAGIC         DW      ?
A_MACHTYPE      DB      ?
A_FLAGS         DB      ?
A_TEXT_SIZE     DD      ?
A_DATA_SIZE     DD      ?
A_BSS_SIZE      DD      ?
A_SYM_SIZE      DD      ?
A_ENTRY         DD      ?
A_TRSIZE        DD      ?
A_DRSIZE        DD      ?
A_OUT_HEADER    ENDS

A_OUT_MAGIC     =       010BH
A_OUT_OFFSET    =       0400H                   ; First page of code
UMAGIC          =       010FH
U_OFFSET        =       0400H                   ; First page of data
KERNEL_U_ADDR   =       0E0000000H              ; Subtract this from U_AR0

;
; This structure that starts a core file
;
USER_AREA       STRUC
U_MAGIC         DW      ?
U_RESERVED1     DW      ?
U_DATA_BASE     DD      ?
U_DATA_END      DD      ?
U_DATA_OFF      DD      ?
U_HEAP_BASE     DD      ?
U_HEAP_END      DD      ?
U_HEAP_OFF      DD      ?
U_HEAP_BRK      DD      ?
U_STACK_BASE    DD      ?
U_STACK_END     DD      ?
U_STACK_OFF     DD      ?
U_STACK_LOW     DD      ?
U_AR0           DD      ?
U_FPVALID       DB      ?
U_RESERVED2     DB      3 DUP (?)
U_FPSTATE       DD      27 DUP (?)
U_FPSTATUS      DD      ?
U_RESERVED3     DD      23 DUP (?)
U_GS            DW      ?, 0
U_FS            DW      ?, 0
U_ES            DW      ?, 0
U_DS            DW      ?, 0
U_EDI           DD      ?
U_ESI           DD      ?
U_EBP           DD      ?
U_ESP           DD      ?
U_EBX           DD      ?
U_EDX           DD      ?
U_ECX           DD      ?
U_EAX           DD      ?
U_TRAPNO        DD      ?
U_ERRCD         DD      ?
U_EIP           DD      ?
U_CS            DW      ?, 0
U_EFL           DD      ?
U_UESP          DD      ?
U_SS            DW      ?, 0
USER_AREA       ENDS

U_REGS          EQU     U_GS

L_FLAG_DLL      =       0001H                   ; Executable is a DLL
L_FLAG_LINK386  =       0002H                   ; Linked by LINK386

;
; This table is at the beginning of the data section
;
LAYOUT_TABLE    STRUC
L_TEXT_BASE     DD      ?
L_TEXT_END      DD      ?
L_DATA_BASE     DD      ?
L_DATA_END      DD      ?
L_BSS_BASE      DD      ?
L_BSS_END       DD      ?
L_HEAP_BASE     DD      ?
L_HEAP_END      DD      ?
L_HEAP_BRK      DD      ?
L_HEAP_OFF      DD      ?
L_OS2_DLL       DD      ?
L_STACK_BASE    DD      ?
L_STACK_END     DD      ?
L_FLAGS         DD      ?
L_RESERVED      DD      2 DUP (?)
L_OPTIONS       DB      64 DUP (?)
LAYOUT_TABLE    ENDS
