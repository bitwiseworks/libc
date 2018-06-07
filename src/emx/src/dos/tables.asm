;
; TABLES.ASM -- Protected mode tables
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

;
; Global descriptor table
; Local descriptor tables
; Task state segment
;

__TABLES        =       1
                INCLUDE EMX.INC
                INCLUDE SIGNAL.INC
                INCLUDE PROCESS.INC
                INCLUDE PMINT.INC
                INCLUDE TABLES.INC
                INCLUDE HEADERS.INC
                INCLUDE EXCEPT.INC
                INCLUDE MISC.INC


                PUBLIC  GDT_PTR, GDT_LIN
                PUBLIC  IDT_PTR, IDT_LIN
                PUBLIC  REAL_IDT_PTR
                PUBLIC  GDT, LDTS, IDT, BASE_TABLE
                PUBLIC  G_VCPI_DESC, G_ENV_DESC, G_PHYS_DESC, G_LIN_MAP_DESC
                PUBLIC  G_SWAP_BMP_DESC, G_PAGEDIR_DESC, G_VIDEO_DESC
                PUBLIC  G_PAGE_BMP_DESC, G_TSS_DESC, G_TSS_MEM_DESC
                PUBLIC  G_TMP1_DESC, G_TMP2_DESC
                PUBLIC  G_BUF1_DESC, G_BUF2_DESC
                PUBLIC  TSS_BUSY
                PUBLIC  TSS_EX8_CR3, TSS_EX10_CR3

SV_DATA         SEGMENT

GDT_PTR         LABEL   FWORD
                WORD    END_GDT - GDT - 1       ; Limit of GDT
GDT_LIN         LABEL   DWORD
                WORD    GDT, 0                  ; Linear address of GDT
                
IDT_PTR         LABEL   FWORD
                WORD    END_IDT - IDT - 1       ; Limit of IDT
IDT_LIN         LABEL   DWORD
                WORD    IDT, 0                  ; Linear address of IDT

REAL_IDT_PTR    LABEL   FWORD
                WORD    03FFH                   ; limit of real-mode IDT
                DWORD   0                       ; Linear address


                DALIGN  8

GDT             LABEL   QWORD
;
; The null selector (G_NULL_SEL)
;
                QWORD   0
;
; The GDT as memory segment (G_GDT_MEM_SEL)
;
BTD1            WORD    END_GDT-GDT-1   ; Limit (0..15)
                WORD    GDT             ; Base (0..15)
                BYTE    0               ; Base (16..23)
                BYTE    93H             ; Access
                BYTE    0               ; Granularity
                BYTE    0               ; Base (24..31)
;
; The IDT (G_IDT_SEL), not used, but convenient for debugging
;
                QWORD   0
;
; Accessing physical memory by physical addresses (G_PHYS_SEL, see note 1)
;
G_PHYS_DESC     WORD    0FFFFH          ; Limit (0..15)
                WORD    0               ; Base (0..15)
                BYTE    0               ; Base (16..23)
                BYTE    93H             ; Access
                BYTE    0CFH            ; Granularity
                BYTE    0               ; Base (24..31)
;
; Supervisor code (G_SV_CODE_SEL)
;
BTC1            WORD    0FFFFH          ; Limit (0..15)
                WORD    0               ; Base (0..15)
                BYTE    0               ; Base (16..23)
                BYTE    9BH             ; Access
                BYTE    0               ; Granularity
                BYTE    0               ; Base (24..31)
;
; Used for switch to real mode (G_REAL_SEL)
;
                WORD    0FFFFH          ; Limit (0..15)
                WORD    0               ; Base (0..15)
                BYTE    0               ; Base (16..23)
                BYTE    93H             ; Access
                BYTE    0               ; Granularity
                BYTE    0               ; Base (24..31)
;
; Initialization/termination code (G_INIT_SEL)
;
BTI1            WORD    0FFFFH          ; Limit (0..15)  !!! (return to RM)
                WORD    0               ; Base (0..15)
                BYTE    0               ; Base (16..23)
                BYTE    9BH             ; Access
                BYTE    0               ; Granularity
                BYTE    0               ; Base (24..31)
;
; Supervisor data (G_SV_DATA_SEL)
;
BTD3            WORD    0FFFFH          ; Limit (0..15)
                WORD    0               ; Base (0..15)
                BYTE    0               ; Base (16..23)
                BYTE    93H             ; Access
                BYTE    0               ; Granularity
                BYTE    0               ; Base (24..31)
;
; Supervisor stack (G_SV_STACK_SEL)
;
BTS1            WORD    OFFSET SV_TOS-1 ; Limit (0..15)
                WORD    0               ; Base (0..15)
                BYTE    0               ; Base (16..23)
                BYTE    93H             ; Access
                BYTE    40H             ; Granularity   (see note 2)
                BYTE    0               ; Base (24..31)
;
; Video memory (G_VIDEO_SEL)
;
G_VIDEO_DESC    WORD    4000 - 1        ; Limit (0..15)
                WORD    0               ; Base (0..15)
                BYTE    0BH             ; Base (16..23)
                BYTE    93H             ; Access
                BYTE    0               ; Granularity
                BYTE    0               ; Base (24..31)
;
; The TSS (G_TSS_SEL)
;
G_TSS_DESC      WORD    END_TSS-TSS-1   ; Limit (0..15)
                WORD    TSS             ; Base (0..15)
                BYTE    0               ; Base (16..23)
TSS_BUSY        BYTE    89H             ; Access
                BYTE    0               ; Granularity
                BYTE    0               ; Base (24..31)
;
; Three segments for VCPI server (G_SERVER_SEL)
;
G_VCPI_DESC     QWORD   3 DUP (0)
;
; The DOS environment (G_ENV_SEL)
;
G_ENV_DESC      QWORD   0
;
; The HDR_SEG segment (G_HDR_SEL)
;
BTH1            WORD    SIZE BIND_HEADER - 1
                WORD    0               ; Base (0..15)
                BYTE    0               ; Base (16..23)
                BYTE    91H             ; Access
                BYTE    0               ; Granularity
                BYTE    0               ; Base (24..31)
;
; Two temporary entries (G_TMP1_SEL, G_TMP2_SEL)
;
G_TMP1_DESC     QWORD   0               ; G_TMP1_SEL
G_TMP2_DESC     QWORD   0               ; G_TMP2_SEL
;
; Zero based segment (4G bytes) for returning from protected mode
; to real mode under VCPI (G_ZERO_SEL)
;
                WORD    0FFFFH          ; Limit (0..15)
                WORD    0               ; Base (0..15)
                BYTE    0               ; Base (16..23)
                BYTE    93H             ; Access
                BYTE    0CFH            ; Granularity
                BYTE    0               ; Base (24..31)
;
; 1st communication buffer (G_BUF1_SEL)
;
G_BUF1_DESC     WORD    0FFFFH          ; Limit 0..15
                WORD    0               ; Base (0..15)
                BYTE    0               ; Base (16..23)
                BYTE    93H             ; Access
                BYTE    0               ; Granularity
                BYTE    0               ; Base (24..31)
;
; 2nd communication buffer (G_BUF2_SEL)
;
G_BUF2_DESC     WORD    0FFFH+OFFSET_1  ; Limit 0..15
                WORD    0               ; Base (0..15)
                BYTE    0               ; Base (16..23)
                BYTE    93H             ; Access
                BYTE    0               ; Granularity
                BYTE    0               ; Base (24..31)
;
; The page directory (and swapper info directory, G_PAGEDIR_SEL)
;
G_PAGEDIR_DESC  WORD    2*4096-1        ; Limit 0..15
                WORD    0               ; Base (0..15)
                BYTE    0               ; Base (16..23)
                BYTE    93H             ; Access
                BYTE    0               ; Granularity
                BYTE    0               ; Base (24..31)
;
; The swapper file bitmap (G_SWAP_BMP_SEL)
;
G_SWAP_BMP_DESC QWORD   0
;
; The linear address map (G_LIN_MAP_SEL)
;
G_LIN_MAP_DESC  WORD    4096-1          ; Limit 0..15
                WORD    0               ; Base (0..15)
                BYTE    0               ; Base (16..23)
                BYTE    93H             ; Access
                BYTE    0               ; Granularity
                BYTE    0               ; Base (24..31)
;
; The free page bitmap (G_PAGE_BMP_SEL)
;
G_PAGE_BMP_DESC QWORD   0
;
; The TSS for exception 8 (double fault, G_TSS_EX8_SEL)
;
BTD5            WORD    END_TSS_EX8 - TSS_EX8 - 1       ; Limit (0..15)
                WORD    TSS_EX8         ; Base (0..15)
                BYTE    0               ; Base (16..23)
                BYTE    89H             ; Access
                BYTE    0               ; Granularity
                BYTE    0               ; Base (24..31)
;
; The TSS for exception 10 (invalid TSS, G_TSS_EX10_SEL)
;
BTD6            WORD    END_TSS_EX10 - TSS_EX10 - 1     ; Limit (0..15)
                WORD    TSS_EX10        ; Base (0..15)
                BYTE    0               ; Base (16..23)
                BYTE    89H             ; Access
                BYTE    0               ; Granularity
                BYTE    0               ; Base (24..31)
;
; Stack for exception 8 task (G_EX8_STACK_SEL)
;
BTS8            WORD    OFFSET EX8_TOS-1        ; Limit (0..15)
                WORD    0               ; Base (0..15)
                BYTE    0               ; Base (16..23)
                BYTE    93H             ; Access
                BYTE    40H             ; Granularity
                BYTE    0               ; Base (24..31)
;
; Stack for exception 10 task (G_EX10_STACK_SEL)
;
BTS10           WORD    OFFSET EX10_TOS-1       ; Limit (0..15)
                WORD    0               ; Base (0..15)
                BYTE    0               ; Base (16..23)
                BYTE    93H             ; Access
                BYTE    40H             ; Granularity
                BYTE    0               ; Base (24..31)
;
; The TSS as memory segment (G_TSS_MEM_SEL)
;
G_TSS_MEM_DESC  WORD    END_TSS-TSS-1   ; Limit (0..15)
                WORD    TSS             ; Base (0..15)
                BYTE    0               ; Base (16..23)
                BYTE    93H             ; Access
                BYTE    0               ; Granularity
                BYTE    0               ; Base (24..31)
;
; The lower 1MB (+64KB)
;
                WORD    010FH           ; Limit (0..15)
                WORD    0               ; Base (0..15)
                BYTE    0               ; Base (16..23)
                BYTE    91H             ; Access (read only)
                BYTE    0C0H            ; Granularity
                BYTE    0               ; Base (24..31)
;
; One LDT per process (G_LDT_SEL, the first entry, is already initialized,
; because it is used for protected mode startup -- only its existence is
; required)
;
BTD8            WORD    8*LDT_ENTRIES-1 ; Limit (0..15)
                WORD    LDTS            ; Base (0..15)
                BYTE    0               ; Base (16..23)
                BYTE    82H             ; Access
                BYTE    0               ; Granularity
                BYTE    0               ; Base (24..31)
                
                QWORD   (MAX_PROCESSES-1) DUP (0)

END_GDT         LABEL   QWORD


;
; Note 1: When running under VCPI, the base address of the physical
;         memory segment will be changed. See INIT_VCPI for details.
;         The limit will be also changed if not running under VCPI,
;         see INIT_PAGING.
;
; Note 2: Setting the B bit is required to make a stack-switch to
;         an inner privilege level load ESP instead of SP from the
;         TSS. With non-zero upper word of ESP the VCPI server doesn't
;         work (tested with CEMM.SYS: crash!).
;
LDTS            QWORD   (MAX_PROCESSES * LDT_ENTRIES) DUP (0)

;
; The IDT will be filled in by this program. See RMINT module.
;
IDT             LABEL   QWORD
                QWORD   256 DUP (?)
END_IDT         LABEL   QWORD


; This TSS is used only for holding the ring 0 SS:ESP.  After
; switching to protected mode, it is replaced with a dynamically
; allocated TSS which adds an I/O permission bitmap.
;
TSS             LABEL   BYTE
                DWORD   0                       ; Back pointer
                WORD    OFFSET SV_TOS, 0        ; STK0
                WORD    G_SV_STACK_SEL, 0
                QWORD   0                       ; STK1
                QWORD   0                       ; STK2
                DWORD   0                       ; CR3
                DWORD   0                       ; EIP
                DWORD   0                       ; EFLAGS
                DWORD   0                       ; EAX
                DWORD   0                       ; ECX
                DWORD   0                       ; EDX
                DWORD   0                       ; EBX
                DWORD   0                       ; ESP
                DWORD   0                       ; EBP
                DWORD   0                       ; ESI
                DWORD   0                       ; EDI
                DWORD   0                       ; ES
                DWORD   0                       ; CS
                DWORD   0                       ; SS
                DWORD   0                       ; DS
                DWORD   0                       ; FS
                DWORD   0                       ; GS
                DWORD   0                       ; LDT
                WORD    0                       ; DTB
                WORD    $ + 2 - TSS             ; I/O bitmap
                BYTE    0FFH                    ; End of bitmap
END_TSS         LABEL   BYTE


;
; This TSS is used for exception 8 (double fault)
;
TSS_EX8         LABEL   BYTE
                DWORD   0                       ; Back pointer
                QWORD   0                       ; STK0 (no priv. level chg!)
                QWORD   0                       ; STK1
                QWORD   0                       ; STK2
TSS_EX8_CR3     DWORD   0                       ; CR3
                DWORD   OFFSET EXCEPT_TASK      ; EIP
                DWORD   0                       ; EFLAGS
                DWORD   8                       ; EAX (exception #)
                DWORD   0                       ; ECX
                DWORD   0                       ; EDX
                DWORD   0                       ; EBX
                DWORD   OFFSET EX8_TOS          ; ESP
                DWORD   0                       ; EBP
                DWORD   0                       ; ESI
                DWORD   0                       ; EDI
                DWORD   G_SV_DATA_SEL           ; ES
                DWORD   G_SV_CODE_SEL           ; CS
                DWORD   G_EX8_STACK_SEL         ; SS
                DWORD   G_SV_DATA_SEL           ; DS
                DWORD   G_SV_DATA_SEL           ; FS
                DWORD   G_SV_DATA_SEL           ; GS
                DWORD   0                       ; LDT
                WORD    0                       ; DTB
                WORD    $ + 2 - TSS_EX8         ; I/O bitmap
                BYTE    0FFH                    ; End of bitmap
END_TSS_EX8     LABEL   BYTE


;
; This TSS is used for exception 10 (invalid TSS)
;
TSS_EX10        LABEL   BYTE
                DWORD   0                       ; Back pointer
                QWORD   0                       ; STK0 (no priv. level chg!)
                QWORD   0                       ; STK1
                QWORD   0                       ; STK2
TSS_EX10_CR3    DWORD   0                       ; CR3
                DWORD   OFFSET EXCEPT_TASK      ; EIP
                DWORD   0                       ; EFLAGS
                DWORD   10                      ; EAX (exception #)
                DWORD   0                       ; ECX
                DWORD   0                       ; EDX
                DWORD   0                       ; EBX
                DWORD   OFFSET EX10_TOS         ; ESP
                DWORD   0                       ; EBP
                DWORD   0                       ; ESI
                DWORD   0                       ; EDI
                DWORD   G_SV_DATA_SEL           ; ES
                DWORD   G_SV_CODE_SEL           ; CS
                DWORD   G_EX10_STACK_SEL        ; SS
                DWORD   G_SV_DATA_SEL           ; DS
                DWORD   G_SV_DATA_SEL           ; FS
                DWORD   G_SV_DATA_SEL           ; GS
                DWORD   0                       ; LDT
                WORD    0                       ; DTB
                WORD    $ + 2 - TSS_EX10        ; I/O bitmap
                BYTE    0FFH                    ; End of bitmap
END_TSS_EX10    LABEL   BYTE


;
; This table is used for converting segment/offset pairs to linear addresses
; in descriptor tables. The first word of a pair is the real-mode segment,
; the second word points to the descriptor table entry (ASSUME DS:SV_DATA),
; which already contains the offset in the base 0..15 field.
;
BASE_TABLE      LABEL   WORD
                WORD    SV_DATA, BTD1
                WORD    SV_DATA, BTD3
                WORD    SV_DATA, G_TSS_DESC
                WORD    SV_DATA, BTD5
                WORD    SV_DATA, BTD6
                WORD    SV_DATA, G_TSS_MEM_DESC
                WORD    SV_DATA, BTD8
                WORD    SV_STACK, BTS1
                WORD    SV_CODE, BTC1
                WORD    INIT_CODE, BTI1
                WORD    HDR_SEG, BTH1
                WORD    EX8_STACK, BTS8
                WORD    EX10_STACK, BTS10
                WORD    0, 0                    ; End of table


SV_DATA         ENDS


EX8_STACK       SEGMENT
                WORD    256 DUP (?)
EX8_TOS         LABEL   WORD
EX8_STACK       ENDS


EX10_STACK      SEGMENT
                WORD    256 DUP (?)
EX10_TOS        LABEL   WORD
EX10_STACK      ENDS


                END
