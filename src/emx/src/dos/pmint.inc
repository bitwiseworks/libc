;
; PMINT.INC -- Handle interrupts
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

; 
; Stack (at PMINT_STACK_SNAP_SHOT):
;
;  GS FS ES DS EDI ESI EBP ESP EBX EDX ECX EAX ERRCD EIP CS EFLAGS ESP(U) SS(U)
;  0  2  4  6  8   12  16  20  24  28  32  36  40    44  48 52     56     60
;  ^                                           -----               ------------
;  |                                           simulated           only for
;  EBP                                         for interrupts      user program
;                                              and exc. 0..7, 16
;
; See also PROCESS.INC, SPECIAL.ASM, /emx/include/sys/reg.h.
;

ISTACKFRAME     STRUC
IS_GS           DW      ?                       ; 0
IS_FS           DW      ?                       ; 2
IS_ES           DW      ?                       ; 4
IS_DS           DW      ?                       ; 6
IS_EDI          DD      ?                       ; 8
IS_ESI          DD      ?                       ; 12
IS_EBP          DD      ?                       ; 16
IS_ESP_1        DD      ?                       ; 20
IS_EBX          DD      ?                       ; 24
IS_EDX          DD      ?                       ; 28
IS_ECX          DD      ?                       ; 32
IS_EAX          DD      ?                       ; 36
IS_ERRCD        DD      ?                       ; 40
IS_EIP          DD      ?                       ; 44
IS_CS           DW      ?                       ; 48
IS_CS_PAD       DW      ?                       ; 50
IS_EFLAGS       DD      ?                       ; 52
IS_ESP          DD      ?                       ; 56
IS_SS           DW      ?                       ; 60
IS_SS_PAD       DW      ?                       ; 62
ISTACKFRAME     ENDS

I_GS            EQU     [BP].IS_GS
I_FS            EQU     [BP].IS_FS
I_ES            EQU     [BP].IS_ES
I_DS            EQU     [BP].IS_DS
I_EDI           EQU     [BP].IS_EDI
I_ESI           EQU     [BP].IS_ESI
I_EBP           EQU     [BP].IS_EBP
I_ESP_1         EQU     [BP].IS_ESP_1
I_EBX           EQU     [BP].IS_EBX
I_EDX           EQU     [BP].IS_EDX
I_ECX           EQU     [BP].IS_ECX
I_EAX           EQU     [BP].IS_EAX
I_ERRCD         EQU     [BP].IS_ERRCD
I_EIP           EQU     [BP].IS_EIP
I_CS            EQU     [BP].IS_CS
I_EFLAGS        EQU     [BP].IS_EFLAGS
I_ESP           EQU     [BP].IS_ESP
I_SS            EQU     [BP].IS_SS

I_REG_DWORDS    =       16                              ; Cf. PROCESS.ASM

;
; Size of local variables of exception/interrupt handler
;
FRAME_SIZE      =       38

;
; Local variables for exceptions
;
VOUTPUT         EQU     (BYTE PTR  [BP-2])
SWAP_PIDX       EQU     (BYTE PTR  [BP-3])
EXCEPT_NO       EQU     (BYTE PTR  [BP-4])
FAULT_ADDR      EQU     (DWORD PTR [BP-8])
ETEXT           EQU     (WORD PTR  [BP-10])
EBYTE           EQU     (WORD PTR  [BP-12])
EWORD           EQU     (WORD PTR  [BP-14])
EDWORD          EQU     (WORD PTR  [BP-16])
ECRLF           EQU     (WORD PTR  [BP-18])
ECHAR           EQU     (WORD PTR  [BP-20])
EXC_UNUSED_1    EQU     (DWORD PTR [BP-24])
FAULT_PROC      EQU     (WORD PTR [BP-26])

;
; Local variables for interrupts
;
INT_NO          EQU     (BYTE PTR [BP-1])
HW_INT_VEC      EQU     (BYTE PTR [BP-2])
;unused         EQU     (BYTE PTR [BP-3])
RM_FLAGS        EQU     (BYTE PTR [BP-4])
INP_HANDLE      EQU     (WORD PTR [BP-6])
RM_AX           EQU     (WORD PTR [BP-8])
RM_BX           EQU     (WORD PTR [BP-10])
RM_CX           EQU     (WORD PTR [BP-12])
RM_DX           EQU     (WORD PTR [BP-14])
RM_SI           EQU     (WORD PTR [BP-16])
RM_DI           EQU     (WORD PTR [BP-18])
RM_BP           EQU     (WORD PTR [BP-20])
RM_DS           EQU     (WORD PTR [BP-22])
RM_ES           EQU     (WORD PTR [BP-24])
NREQ            EQU     (DWORD PTR [BP-28])
NPTR            EQU     (DWORD PTR [BP-32])
N_TMP           EQU     (WORD PTR [BP-34])
N_AX            EQU     (WORD PTR [BP-36])
CONV_FLAG       EQU     (WORD PTR [BP-38])

;
; Communication area for __fpuemu()
;
FPUEMU_COM      STRUCT
FEC_NOTIFY      DD      ?
FEC_PNUM        DD      ?
FEC_SIGNAL      DD      ?
FEC_FRAME       DD      I_REG_DWORDS DUP (?)
FPUEMU_COM      ENDS

FPUC_INIT       =       0
FPUC_NEXT       =       1
FPUC_SIGNAL     =       2

FPUN_NEWPROC    =       0
FPUN_ENDPROC    =       1
FPUN_EMU        =       2

;
; These are the offsets of the PM/RM communication buffer
;
; Note: There's a bug in DOS function 5AH (COMPAQ DOS 3.31):
;       It stores a backslash DS:[DX+0FFFFH] if DX=0 !!!
;       0 < OFFSET_1 < 16 (cf. D_READ, D_WRITE, INIT_BUFFER)
;
OFFSET_1        =       0004H           ; Used if one buffer is required
OFFSET_2        =       8000H           ; Used for second buffer if two
                                        ; buffers are required
              IFNDEF __PMINT

SV_DATA         SEGMENT

                EXTRN   BUF_SEG:WORD
                EXTRN   BUF_SEL:WORD
                EXTRN   BUF1_SEG:WORD
                EXTRN   BUF2_SEG:WORD
                EXTRN   PMINT0_TAB:WORD
                EXTRN   PMINT8_TAB:WORD
                EXTRN   RM_CR0:DWORD
                EXTRN   PM_CR0:DWORD
                EXTRN   DBCS_LEAD_TAB:WORD

SV_DATA         ENDS

SV_CODE         SEGMENT

                EXTRN   IRQ0_ADD:BYTE
                EXTRN   IRQ8_ADD:BYTE

                EXTRN   INTERRUPT:NEAR          ; Unexpected interrupt
                EXTRN   V2P_CONT:NEAR           ; Return from VCPI server
                EXTRN   DOSF_ERROR:NEAR         ; Invalid function code
                EXTRN   BREAK_AFTER_IRET:NEAR   ; Set breakpoint to ret addr
                EXTRN   NMI:NEAR                ; NMI handler
                EXTRN   PMINT_10H:NEAR          ; VIO interface interrupt
                EXTRN   PMINT_11H:NEAR          ; Equipment interrupt
                EXTRN   PMINT_14H:NEAR          ; ASY interface interrupt
                EXTRN   PMINT_16H:NEAR          ; KBD interface interrupt
                EXTRN   PMINT_17H:NEAR          ; PRN interface interrupt
                EXTRN   PMINT_21H:NEAR          ; DOS interface interrupt
                EXTRN   PMINT_31H:NEAR          ; EXT interface interrupt
                EXTRN   PMINT_33H:NEAR          ; MOU interface interrupt
                EXTRN   INT_RM:NEAR             ; Call RM interrupt routine
                EXTRN   GET_DBCS_LEAD:NEAR      ; Get DBCS lead bytes

MOVE_TO_RM      PROTO   NEAR
MOVE_FROM_RM    PROTO   NEAR

SV_CODE         ENDS


INIT_CODE       SEGMENT

                EXTRN   V2V_CONT:NEAR
                EXTRN   INIT_BUFFER:NEAR

INIT_CODE       ENDS

              ENDIF
