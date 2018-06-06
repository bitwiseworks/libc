;
; DISASM.ASM -- Disassembler
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

DISASM_FLAG     =       1               ; Include disassembler?
                                        ; Not yet implemented

                INCLUDE EMX.INC
                INCLUDE SYMBOLS.INC

                PUBLIC  DISASM

              IF DISASM_FLAG

SV_DATA         SEGMENT

SIZE_ON         =       01H
SIZE_OFF        =       00H

F_SIGNED        =       01H
F_EMPTY         =       02H
F_NO_PTR        =       04H

V_REPE          =       1
V_REPNE         =       2

RELOCATION      DD      ?               ; Add this to EIP
MODE_32         DB      ?               ; Operand/address size (default)
PFX_ASIZE       DB      ?               ; Address size prefix
PFX_OSIZE       DB      ?               ; Operand size prefix
PFX_SEG         DB      ?               ; Segment override
PFX_REP         DB      ?               ; Repeat prefix
PFX_LOCK        DB      ?
FLAGS           DB      ?
OPCODE          DB      ?
OPSIZE          DB      ?
MOD_REG_RM      DB      ?
SIB             DB      ?
ACCU            DB      ?
SYM_FLAG        DB      ?
SYM_ADDR        DD      ?

$REG16          DB      "AXCXDXBXSPBPSIDI"
$REG8           DB      "ALCLDLBLAHCHDHBH"

$BYTE           DB      "byte", 0
$DWORD          DB      "d"
$WORD           DB      "word", 0
$PTR            DB      " ptr ",0

                DW      32 DUP (?)
DISASM_STACK    LABEL   WORD
DISASM_SP       DW      ?
                DB      16 DUP (?)
ACCU_STACK      LABEL   BYTE
ACCU_SP         DW      ?

NOT_FLAG        DB      ?

RM_BX           =       01H
RM_BP           =       02H
RM_SI           =       04H
RM_DI           =       08H

RM_TAB          DB      RM_BX+RM_SI
                DB      RM_BX+RM_DI
                DB      RM_BP+RM_SI
                DB      RM_BP+RM_DI
                DB      RM_SI
                DB      RM_DI
                DB      RM_BP
                DB      RM_BX

DISASM_JMP      DW      Q_END, Q_DWORD, Q_JUMP, Q_DISP
                DW      Q_NOT, Q_OPCODE, Q_SWITCH, Q_MASK_EQ
                DW      Q_LD_MOD_REG_RM, Q_TEST, Q_BITS, Q_CMP
                DW      Q_REG, Q_REGA, Q_MEM, Q_FLAGS
                DW      Q_NIBBLE, Q_ACCUM, Q_MORE, Q_O16
                DW      Q_A16, Q_BYTE, Q_WORD, Q_TABLE
                DW      Q_RET, Q_PREFIX, Q_EXT, Q_OP_SIZE
                DW      Q_JMP_DST, Q_IMMED, Q_MOD_REG_RM, Q_OP

EXT_JMP         DW      Q_PUSH, Q_POP, Q_XCHG, Q_BACK

Z_END           =       00H
Z_DWORD         =       01H
Z_JUMP          =       02H
Z_DISP          =       03H
Z_NOT           =       04H
Z_OPCODE        =       05H
Z_SWITCH        =       06H
Z_MASK_EQ       =       07H
Z_LD_MOD_REG_RM =       08H
Z_TEST          =       09H
Z_BITS          =       0AH
Z_CMP           =       0BH
Z_REG           =       0CH
Z_REGA          =       0DH
Z_MEM           =       0EH
Z_FLAGS         =       0FH
Z_NIBBLE        =       10H
Z_ACCUM         =       11H
Z_MORE          =       12H
Z_O16           =       13H
Z_A16           =       14H
Z_BYTE          =       15H
Z_WORD          =       16H
Z_TABLE         =       17H
Z_RET           =       18H
Z_PREFIX        =       19H
Z_EXT           =       1AH                     ; Currently not used
Z_OP_SIZE       =       1BH
Z_JMP_DST       =       1CH
Z_IMMED         =       1DH
Z_MOD_REG_RM    =       1EH
Z_OP            =       1FH

ZE_PUSH         =       00H
ZE_POP          =       01H
ZE_XCHG         =       02H
ZE_BACK         =       03H

; ============================================================================
; Disassembler language
; ============================================================================

;
; Utility macros (required for expansion of numeric variables (N)
;
VSET            MACRO   V, N, X
V&N             =       X
                ENDM
VINC            MACRO   V, N
V&N             =       V&N + 1
                ENDM
VDB             MACRO   V, N
                DB      V&N
                ENDM
VDW             MACRO   V, N
                DW      V&N
                ENDM

;
; End of program
;
X_END           MACRO
                DB      Z_END
                ENDM

;
; Fetch DWORD from input and insert into output
;
X_DWORD         MACRO
                DB      Z_DWORD
                ENDM

;
; Jump to another location
;
X_JUMP          MACRO   DST
                DB      Z_JUMP
                DW      DST
                ENDM

;
; Call subroutine (note: no checking for stack overflow)
;
X_CALL          MACRO   DST
                DB      CALL_&DST
                ENDM
;
; Define subroutine
;
CALLCNT         =       0
X_SUB           MACRO   NAME
CALL_&NAME      =       80H + CALLCNT
                VSET    CALLV,%CALLCNT, <THIS BYTE>
CALLCNT         =       CALLCNT + 1
                ENDM
;
; Negate condition of next conditional jump instruction
;
X_NOT           MACRO
                DB      Z_NOT
                ENDM

;
; X_OPCODE
;
; Reload opcode into accu
;
X_OPCODE        MACRO
                DB      Z_OPCODE
                ENDM


SWCNT           =       0

;
; Switch statement (accu)
;
X_SWITCH        MACRO
SWCNT           =       SWCNT+1
                DB      Z_SWITCH
                VDB     SWLEN, %SWCNT
                VSET    SWLEN, %SWCNT, 0
                ENDM

;
; One case of a switch statement: jump to DST, if accu==VALUE
;
X_CASE          MACRO   VALUE, DST
                VINC    SWLEN, %SWCNT
                DB      VALUE
                DW      DST
                ENDM

;
; Jump to DST, if (accu & MASK) == VALUE
;
X_MASK_EQ       MACRO   MASK, VALUE, DST
                DB      Z_MASK_EQ, MASK, VALUE
                DW      DST
                ENDM

;
; Load accu from mod,reg,r/m byte
;
X_LD_MOD_REG_RM MACRO
                DB      Z_LD_MOD_REG_RM
                ENDM

;
; Load accu from reg field of mod,reg,r/m byte
;
X_LOAD_REG      MACRO
                X_CALL  P_LOAD_REG
                ENDM

;
; Jump to DST, if (accu & MASK) != 0
;
; (If we run out of instruction codes, this can be done with Z_MASK_EQ.)
;
X_TEST          MACRO   MASK, DST
                DB      Z_TEST, MASK
                DW      DST
                ENDM

;
; Extract a bit field of accu: accu := (accu AND MASK) SHR b,
; with b = min {i | (MASK AND (1 SHL i)) != 0}
;
; Example:
;       accu = 01101101
;       MASK = 00111000
;     result = 00000101 (bits 3..5 of accu moved to bits 0..2)
;
X_BITS          MACRO   MASK
                DB      Z_BITS, MASK
                ENDM

;
; Jump to DST, if variable SRC contains VALUE
;
X_CMP           MACRO   SRC, VALUE, DST
                DB      Z_CMP
                DW      SRC
                DB      VALUE
                DW      DST
                ENDM

;
; Insert name of register specified by reg field of reg,mod,r/m byte
;
X_REG           MACRO
                DB      Z_REG
                ENDM

;
; Insert name of register specified by bits 0..2 of accu
;
X_REGA          MACRO
                DB      Z_REGA
                ENDM

;
; Insert operand specified by mod & r/m fields of reg,mod,r/m byte
;
X_MEM           MACRO
                DB      Z_MEM
                ENDM

;
; Insert destination address of a short jump instruction
;
X_SHORT         MACRO
                DB      Z_JMP_DST, 0
                ENDM

;
; Insert destination address of a near jump instruction
;
X_NEAR          MACRO
                DB      Z_JMP_DST, 1
                ENDM

;
; Insert destination address of a far jump instruction
;
X_FAR           MACRO
                DB      Z_JMP_DST, 2
                ENDM

;
; Insert name of 8/16/32 bit accumulator (AL/AX/EAX)
;
X_ACCUM         MACRO
                DB      Z_ACCUM
                ENDM

;
; Jump to DST, if operand size = 16 bit
;
X_O16           MACRO   DST
                DB      Z_O16
                DW      DST
                ENDM

;
; Jump to DST, if address size = 16 bit
;
X_A16           MACRO   DST
                DB      Z_A16
                DW      DST
                ENDM

;
; Fetch mod,reg,r/m byte
;
X_MOD_REG_RM    MACRO
                DB      Z_MOD_REG_RM
                ENDM

;
; Insert opcode / operand delimiter into output
;
X_OP            MACRO
                DB      Z_OP
                ENDM

;
; Fetch BYTE from input and insert into output
;
X_BYTE          MACRO
                DB      Z_BYTE
                ENDM

;
; Fetch WORD from input and insert into output
;
X_WORD          MACRO
                DB      Z_WORD
                ENDM

;
; Insert [xxxx] (WORD/DWORD, depending on address size)
;
X_DISP          MACRO
                DB      Z_DISP
                ENDM

;
; Table jump: load accu'th (0..255) word following this instruction
;             into instruction pointer
; Attention:  be sure that there's a table entry for all possible values
;             of accu!
;
X_TABLE         MACRO
                DB      Z_TABLE
                ENDM

;
; Return from subroutine
;
X_RET           MACRO
                DB      Z_RET
                ENDM

;
; Handle prefix instruction: store VALUE to variable PREFIX,
; fetch next opcode byte, and restart program
;
X_PREFIX        MACRO   PREFIX, VALUE
                DB      Z_PREFIX
                DW      PREFIX
                DB      VALUE
                ENDM

;
; Insert operand / operand delimiter into output
;
X_COMMA         MACRO
                X_CALL  P_COMMA
                ENDM

;
; Switch operand size to BYTE (note 1)
;
X_BYTE_OP       MACRO
                DB      Z_OP_SIZE, 00H
                ENDM

;
; Switch operand size to WORD (note 1)
;
X_WORD_OP       MACRO
                DB      Z_OP_SIZE, 01H
                ENDM

;
; Switch operand size to FWORD (note 1)
;
X_FAR_OP        MACRO
                DB      Z_OP_SIZE, 02H
                ENDM

;
; Insert immediate operand into output (uses F_SIGNED flag)
;
X_IMMED         MACRO
                DB      Z_IMMED
                ENDM

;
; Fetch next opcode byte
;
X_MORE          MACRO
                DB      Z_MORE
                ENDM

;
; Set F_SIGNED flag (for immediate operands)
;
X_SIGNED        MACRO
                DB      Z_FLAGS, F_SIGNED
                ENDM

;
; Set F_EMPTY flag (after JMP/RET/... instructions)
;
X_EMPTY         MACRO
                DB      Z_FLAGS, F_EMPTY
                ENDM

;
; Set F_NO_PTR flag (omit `byte ptr' etc.)
;
X_NO_PTR        MACRO
                DB      Z_FLAGS, F_NO_PTR
                ENDM

;
; Insert accu into output (bits 0..3 in hexadecimal representation)
;
X_NIBBLE        MACRO
                DB      Z_NIBBLE
                ENDM

;
; Push accu on stack
;
X_PUSH          MACRO
                DB      Z_EXT, ZE_PUSH
                ENDM


;
; Pop accu from stack
;
X_POP           MACRO
                DB      Z_EXT, ZE_POP
                ENDM


;
; Exchange accu with top of stack
;
X_XCHG          MACRO
                DB      Z_EXT, ZE_XCHG
                ENDM


;
; Backup one byte
;
X_BACK          MACRO
                DB      Z_EXT, ZE_BACK
                ENDM


;
; Note 1: The default operand size is determined by bit 0 (w) of the
;         current opcode byte.
;

; ============================================================================
; This is the 80386 disassembler, written in disassembler language
; ============================================================================
;
; If I had time, I would create a compiler which translates a C like
; language to this object code. Example:
;
;       if (op & 0xf8 == 0x50)
;           {
;           "PUSH\t"; word_op; reg_a; end;
;           }
;
; But of course I don't have time. Therefore, this disassembler is
; somewhat cryptic.
;
;
DISASM_PROGRAM  LABEL   BYTE
                X_SWITCH
                X_CASE   0FH, P_0F
                X_CASE   26H, P_ES
                X_CASE   27H, P_DAA
                X_CASE   2EH, P_CS
                X_CASE   2FH, P_DAS
                X_CASE   36H, P_SS
                X_CASE   37H, P_AAA
                X_CASE   3EH, P_DS
                X_CASE   3FH, P_AAS
                X_CASE   60H, P_PUSHA
                X_CASE   61H, P_POPA
                X_CASE   62H, P_BOUND
                X_CASE   63H, P_ARPL
                X_CASE   64H, P_FS
                X_CASE   65H, P_GS
                X_CASE   66H, P_O_PFX
                X_CASE   67H, P_A_PFX
                X_CASE   8DH, P_LEA
                X_CASE   8FH, P_POP_MEM
                X_CASE   90H, P_NOP
                X_CASE   98H, P_CBW
                X_CASE   99H, P_CDQ
                X_CASE   9AH, P_CALLF
                X_CASE   9BH, P_WAIT
                X_CASE   9CH, P_PUSHF
                X_CASE   9DH, P_POPF
                X_CASE   9EH, P_SAHF
                X_CASE   9FH, P_LAHF
                X_CASE  0C4H, P_LES
                X_CASE  0C5H, P_LDS
                X_CASE  0C8H, P_ENTER
                X_CASE  0C9H, P_LEAVE
                X_CASE  0CCH, P_INT3
                X_CASE  0CDH, P_INT
                X_CASE  0CEH, P_INTO
                X_CASE  0CFH, P_IRET
                X_CASE  0D4H, P_AAM
                X_CASE  0D5H, P_AAD
                X_CASE  0D7H, P_XLAT
                X_CASE  0E0H, P_LOOPNE
                X_CASE  0E1H, P_LOOPE
                X_CASE  0E2H, P_LOOP
                X_CASE  0E3H, P_JCXZ
                X_CASE  0E8H, P_CALLN
                X_CASE  0E9H, P_JMPN
                X_CASE  0EAH, P_JMPF
                X_CASE  0EBH, P_JMPS
                X_CASE  0F0H, P_LOCK
                X_CASE  0F2H, P_REPNE
                X_CASE  0F3H, P_REPE
                X_CASE  0F4H, P_HLT
                X_CASE  0F5H, P_CMC
                X_CASE  0F8H, P_CLC
                X_CASE  0F9H, P_STC
                X_CASE  0FAH, P_CLI
                X_CASE  0FBH, P_STI
                X_CASE  0FCH, P_CLD
                X_CASE  0FDH, P_STD
                X_MASK_EQ 0F0H,  70H, P_JCOND8
                X_MASK_EQ 0F8H,  50H, P_PUSH
                X_MASK_EQ 0F8H,  58H, P_POP
                X_MASK_EQ 0F8H,  40H, P_INC2
                X_MASK_EQ 0F8H,  48H, P_DEC2
                X_MASK_EQ 0F8H,  90H, P_XCHG2
                X_MASK_EQ 0F8H, 0B0H, P_MOV3B
                X_MASK_EQ 0F8H, 0B8H, P_MOV3W
                X_MASK_EQ 0F8H, 0D8H, P_ESC
                X_MASK_EQ 0FCH,  80H, P_ARITH2
                X_MASK_EQ 0FCH,  88H, P_MOV1
                X_MASK_EQ 0FCH, 0A0H, P_MOV4
                X_MASK_EQ 0FDH,  68H, P_PUSH_IMMED
                X_MASK_EQ 0FDH,  69H, P_IMUL_IMMED
                X_MASK_EQ 0FDH,  8CH, P_MOV5
                X_MASK_EQ 0FEH,  6CH, P_INS
                X_MASK_EQ 0FEH,  6EH, P_OUTS
                X_MASK_EQ 0FEH,  84H, P_TEST1
                X_MASK_EQ 0FEH,  86H, P_XCHG1
                X_MASK_EQ 0FEH, 0A4H, P_MOVS
                X_MASK_EQ 0FEH, 0A6H, P_CMPS
                X_MASK_EQ 0FEH, 0A8H, P_TEST3
                X_MASK_EQ 0FEH, 0AAH, P_STOS
                X_MASK_EQ 0FEH, 0ACH, P_LODS
                X_MASK_EQ 0FEH, 0AEH, P_SCAS
                X_MASK_EQ 0FEH, 0C0H, P_ROTATE3
                X_MASK_EQ 0FEH, 0C2H, P_RETN
                X_MASK_EQ 0FEH, 0CAH, P_RETF
                X_MASK_EQ 0FEH, 0C6H, P_MOV2
                X_MASK_EQ 0FEH, 0D0H, P_ROTATE1
                X_MASK_EQ 0FEH, 0D2H, P_ROTATE2
                X_MASK_EQ 0FEH, 0E4H, P_IN1
                X_MASK_EQ 0FEH, 0E6H, P_OUT1
                X_MASK_EQ 0FEH, 0ECH, P_IN2
                X_MASK_EQ 0FEH, 0EEH, P_OUT2
                X_MASK_EQ 0FEH, 0F6H, P_F6_F7
                X_MASK_EQ 0FEH, 0FEH, P_FE_FF
                X_MASK_EQ 0E7H,  07H, P_POP_SEG
                X_MASK_EQ 0C4H,  00H, P_ARITH1
                X_MASK_EQ 0C6H,  04H, P_ARITH3
                X_MASK_EQ 0C7H,  06H, P_PUSH_SEG
FAILURE         DB      "???", Z_END

P_0F            LABEL   BYTE
                X_MORE
                X_SWITCH
                X_CASE   00H, P_0F00
                X_CASE   01H, P_0F01
                X_CASE   02H, P_LAR
                X_CASE   03H, P_LSL
                X_CASE   06H, P_CLTS
                X_CASE   20H, P_MOV_FROM_CR
                X_CASE   21H, P_MOV_FROM_DR
                X_CASE   22H, P_MOV_TO_CR
                X_CASE   23H, P_MOV_TO_DR
                X_CASE   24H, P_MOV_FROM_TR
                X_CASE   26H, P_MOV_TO_TR
                X_CASE  0A4H, P_SHLD_IMMED
                X_CASE  0A5H, P_SHLD_CL
                X_CASE  0ACH, P_SHRD_IMMED
                X_CASE  0ADH, P_SHRD_CL
                X_CASE  0AFH, P_IMUL_MEM
                X_CASE  0B2H, P_LSS
                X_CASE  0B4H, P_LFS
                X_CASE  0B5H, P_LGS
                X_CASE  0BAH, P_BT1
                X_CASE  0BCH, P_BSF
                X_CASE  0BDH, P_BSR
                X_MASK_EQ 0FEH, 0BEH, P_MOVSX
                X_MASK_EQ 0FEH, 0B6H, P_MOVZX
                X_MASK_EQ 0F0H,  80H, P_JCOND16
                X_MASK_EQ 0F0H,  90H, P_SET
                X_MASK_EQ 0E7H, 0A3H, P_BT2
                X_MASK_EQ 0C7H,  80H, P_PUSH_SEG
                X_MASK_EQ 0C7H,  81H, P_POP_SEG
                X_JUMP  FAILURE

P_0F00          LABEL   BYTE
                X_MOD_REG_RM
                X_LOAD_REG
                X_SWITCH
                X_CASE  00H, P_SLDT
                X_CASE  01H, P_STR
                X_CASE  02H, P_LLDT
                X_CASE  03H, P_LTR
                X_CASE  04H, P_VERR
                X_CASE  05H, P_VERW
                X_JUMP  FAILURE

P_0F01          LABEL   BYTE
                X_MOD_REG_RM
                X_LOAD_REG
                X_SWITCH
                X_CASE  00H, P_SGDT
                X_CASE  01H, P_SIDT
                X_CASE  02H, P_LGDT
                X_CASE  03H, P_LIDT
                X_CASE  04H, P_SMSW
                X_CASE  06H, P_LMSW
                X_JUMP  FAILURE

P_FE_FF         LABEL   BYTE
                X_MOD_REG_RM
                X_LOAD_REG
                X_SWITCH
                X_CASE  00H, P_INC1
                X_CASE  01H, P_DEC1
                X_CASE  02H, P_CALL_INDN
                X_CASE  03H, P_CALL_INDF
                X_CASE  04H, P_JMP_INDN
                X_CASE  05H, P_JMP_INDF
                X_CASE  06H, P_PUSH_MEM
                X_JUMP  FAILURE

P_F6_F7         LABEL   BYTE
                X_MOD_REG_RM
                X_LOAD_REG
                X_SWITCH
                X_CASE  00H, P_TEST2
                X_CASE  02H, P_NOT
                X_CASE  03H, P_NEG
                X_CASE  04H, P_MUL
                X_CASE  05H, P_IMUL
                X_CASE  06H, P_DIV
                X_CASE  07H, P_IDIV
                X_JUMP  FAILURE

P_AAA           DB      "AAA", Z_END
P_AAS           DB      "AAS", Z_END
P_DAA           DB      "DAA", Z_END
P_DAS           DB      "DAS", Z_END
P_HLT           DB      "HLT", Z_END
P_NOP           DB      "NOP", Z_END
P_CMC           DB      "CMC", Z_END
P_CLC           DB      "CLC", Z_END
P_CLI           DB      "CLI", Z_END
P_CLD           DB      "CLD", Z_END
P_STC           DB      "STC", Z_END
P_STI           DB      "STI", Z_END
P_STD           DB      "STD", Z_END
P_CLTS          DB      "CLTS", Z_END
P_INTO          DB      "INTO", Z_END
P_LAHF          DB      "LAHF", Z_END
P_SAHF          DB      "SAHF", Z_END
P_LEAVE         DB      "LEAVE", Z_END
P_WAIT          DB      "WAIT", Z_END

;
; XLAT
;
; Bug:  prefix ignored
;
P_XLAT          DB      "XLAT", Z_END

P_A_PFX         LABEL   BYTE
                X_PREFIX PFX_ASIZE, SIZE_ON

P_O_PFX         LABEL   BYTE
                X_PREFIX PFX_OSIZE, SIZE_ON

P_CS            LABEL   BYTE
                X_PREFIX PFX_SEG, "C"

P_DS            LABEL   BYTE
                X_PREFIX PFX_SEG, "D"

P_ES            LABEL   BYTE
                X_PREFIX PFX_SEG, "E"

P_FS            LABEL   BYTE
                X_PREFIX PFX_SEG, "F"

P_GS            LABEL   BYTE
                X_PREFIX PFX_SEG, "G"

P_SS            LABEL   BYTE
                X_PREFIX PFX_SEG, "S"

P_LOCK          DB      "LOCK "
                X_PREFIX PFX_LOCK, 1

P_REPE          LABEL   BYTE
                X_PREFIX PFX_REP, V_REPE

P_REPNE         LABEL   BYTE
                X_PREFIX PFX_REP, V_REPNE

P_AAM           DB      "AAM"
                X_JUMP  P_10
P_AAD           DB      "AAD"
P_10            LABEL   BYTE
                X_MORE
                X_CMP   OPCODE, 10, P_10_1
                DB      Z_OP, Z_BYTE
P_10_1          DB      Z_END


P_INT           DB      "INT", Z_OP, Z_BYTE, Z_END

P_INT3          DB      "INT", Z_OP, "3", Z_END

P_ENTER         DB      "ENTER", Z_OP
                X_WORD
                X_COMMA
                X_BYTE
                X_END

P_LSS           DB      "LS"
                X_JUMP  P_LSEG

P_LFS           DB      "LF"
                X_JUMP  P_LSEG

P_LGS           DB      "LG"
                X_JUMP  P_LSEG

P_LES           DB      "LE"
                X_JUMP  P_LSEG

P_LDS           DB      "LD"
P_LSEG          DB      "S"
                X_FAR_OP
                X_CALL  OP_REG_MEM              ; (OP) reg, mem (END)

P_PUSH          LABEL   BYTE
                X_CALL  TPUSH
                X_JUMP  P_PUSH_POP

P_POP           LABEL   BYTE
                X_CALL  TPOP
P_PUSH_POP      LABEL   BYTE
                X_WORD_OP
                DB      Z_REGA, Z_END

P_PUSHF         DB      "PUSHF"
                X_CALL  APPEND_D

P_POPF          DB      "POPF"
                X_CALL  APPEND_D

P_PUSHA         DB      "PUSHA"
                X_CALL  APPEND_D

P_POPA          DB      "POPA"
                X_CALL  APPEND_D

P_IRET          DB      "IRET"
                X_EMPTY
                X_CALL  APPEND_D

P_CBW           DB      "C"
                X_O16   P_CBW_1
                DB      "WDE", Z_END
P_CBW_1         DB      "BW", Z_END

P_CDQ           DB      "C"
                X_O16   P_CDQ_1
                DB      "DQ", Z_END
P_CDQ_1         DB      "WD", Z_END


P_LTR           DB      "LTR"
                X_CALL  WMEM

P_STR           DB      "STR"
                X_CALL  WMEM

P_LLDT          DB      "LLDT"
                X_CALL  WMEM

P_LGDT          DB      "LGDT"
                X_CALL  WMEM                    ; bug: should be FWORD PTR

P_LIDT          DB      "LIDT"
                X_CALL  WMEM                    ; bug: should be FWORD PTR

P_VERR          DB      "VERR"
                X_CALL  WMEM

P_VERW          DB      "VERW"
                X_CALL  WMEM

P_SLDT          DB      "SLDT"
                X_CALL  WMEM

P_SGDT          DB      "SGDT"
                X_CALL  WMEM                    ; bug: should be FWORD PTR

P_SIDT          DB      "SIDT"
                X_CALL  WMEM                    ; bug: should be FWORD PTR

P_LAR           DB      "LAR"
P_LAR_1         LABEL   BYTE
                X_WORD_OP
                X_CALL  OP_REG_MEM              ; (OP) reg, mem (END)

P_LSL           DB      "LSL"
                X_JUMP  P_LAR_1

P_LMSW          DB      "LMSW"
                X_CALL  WMEM

P_SMSW          DB      "SMSW"
                X_CALL  WMEM

;
; Bug in 16 bit mode: reg16 instead of reg32
;
P_MOV_TO_CR     LABEL   BYTE
                X_CALL  TMOV
                DB      "C"
                X_JUMP  MOV_TO_SPEC

P_MOV_FROM_CR   LABEL   BYTE
                X_CALL  TMOV
                DB      Z_MOD_REG_RM
                X_WORD_OP
                X_MEM
                X_COMMA
                DB      "C"
                X_CALL  SPECIAL
                X_END

P_MOV_TO_DR     LABEL   BYTE
                X_CALL  TMOV
                DB      "D"
                X_JUMP  MOV_TO_SPEC

P_MOV_FROM_DR   LABEL   BYTE
                X_CALL  TMOV
                DB      Z_MOD_REG_RM
                X_WORD_OP
                X_MEM
                X_COMMA
                DB      "D"
                X_CALL  SPECIAL
                X_END

P_MOV_TO_TR     LABEL   BYTE
                X_CALL  TMOV
                DB      "T"
MOV_TO_SPEC     LABEL   BYTE
                X_MOD_REG_RM
                X_WORD_OP
                X_CALL  SPECIAL
                X_COMMA
                DB      Z_MEM, Z_END

P_MOV_FROM_TR   LABEL   BYTE
                X_CALL  TMOV
                DB      Z_MOD_REG_RM
                X_WORD_OP
                X_MEM
                X_COMMA
                DB      "T"
                X_CALL  SPECIAL
                X_END

P_SHLD_IMMED    LABEL   BYTE
                X_CALL  TSHL
                X_JUMP  P_SHD_IMMED
P_SHRD_IMMED    LABEL   BYTE
                X_CALL  TSHR
P_SHD_IMMED     DB      "D", Z_OP, Z_MOD_REG_RM
                X_WORD_OP
                X_MEM
                X_COMMA
                X_REG
                X_COMMA
                DB      Z_BYTE, Z_END

P_SHLD_CL       LABEL   BYTE
                X_CALL  TSHL
                X_JUMP  P_SHD_CL
P_SHRD_CL       LABEL   BYTE
                X_CALL  TSHR
P_SHD_CL        DB      "D", Z_OP, Z_MOD_REG_RM
                X_WORD_OP
                X_MEM
                X_COMMA
                X_REG
                X_COMMA
                DB      "CL", Z_END


;
; Bug: source operand is 32 bit instead of 16 bit. 8 bit is ok
;
P_MOVSX         DB      "MOVS"
P_MOVSZ         DB      "X", Z_OP
                X_WORD_OP
                DB      Z_MOD_REG_RM, Z_REG
                X_COMMA
                X_TEST  01H, P_MOVSZ_1
                X_BYTE_OP
P_MOVSZ_1       DB      Z_MEM, Z_END

P_MOVZX         DB      "MOVZ"
                X_JUMP  P_MOVSZ

;
; bug: address size prefix & segment overide prefix ignored
;
P_CMPS          LABEL   BYTE
                X_CALL  REP_COND
                DB      "CMPS"
STRING_BW       LABEL   BYTE
                X_TEST  01H, STRING_W
                DB      "B", Z_END
STRING_W        LABEL   BYTE
                X_O16   STRING_W1
                DB      "D", Z_END
STRING_W1       DB      "W", Z_END


                X_SUB   REP_COND
                X_CMP   PFX_REP, 0, REP_COND_2
                DB      "REP"
                X_CMP   PFX_REP, V_REPE, REP_COND_1
                DB      "N"
REP_COND_1      DB      "E "
REP_COND_2      DB      Z_RET

                X_SUB   REP_UNCOND
                X_CMP   PFX_REP, 0, REP_UNCOND_1
                DB      "REP "
REP_UNCOND_1    DB      Z_RET



;
; bug: address size prefix & segment overide prefix ignored
;
P_LODS          LABEL   BYTE
                X_CALL  REP_UNCOND
                DB      "LODS"
                X_JUMP  STRING_BW


;
; bug: address size prefix & segment overide prefix ignored
;
P_MOVS          LABEL   BYTE
                X_CALL  REP_UNCOND
                DB      "MOVS"
                X_JUMP  STRING_BW

;
; bug: address size prefix & segment overide prefix ignored
;
P_SCAS          LABEL   BYTE
                X_CALL  REP_COND
                DB      "SCAS"
                X_JUMP  STRING_BW

;
; bug: address size prefix & segment overide prefix ignored
;
P_STOS          LABEL   BYTE
                X_CALL  REP_UNCOND
                DB      "STOS"
                X_JUMP  STRING_BW

;
; bug: address size prefix & segment overide prefix ignored
;
P_INS           LABEL   BYTE
                X_CALL  REP_UNCOND
                DB      "INS"
                X_JUMP  STRING_BW

;
; bug: address size prefix & segment overide prefix ignored
;
P_OUTS          LABEL   BYTE
                X_CALL  REP_UNCOND
                DB      "OUTS"
                X_JUMP  STRING_BW


P_ARPL          DB      "ARPL"
                DB      Z_OP, Z_MOD_REG_RM, Z_MEM
                X_COMMA
                DB      Z_REG, Z_END

P_BOUND         DB      "BOUND"
                X_CALL  OP_REG_MEM              ; (OP) reg, mem (END)

                X_SUB   TPUSH
                DB      "PUSH", Z_OP, Z_RET

                X_SUB   TPOP
                DB      "POP", Z_OP, Z_RET

P_PUSH_SEG      LABEL   BYTE
                X_CALL  TPUSH
                X_BITS  38H
                X_CALL  SREG
                X_END

P_PUSH_MEM      LABEL   BYTE
                X_CALL  TPUSH
                DB      Z_MEM, Z_END

P_PUSH_IMMED    LABEL   BYTE
                X_CALL  TPUSH
                X_WORD_OP
                X_NOT
                X_TEST  02H, P_PUSH_IMMED_1
                X_SIGNED
P_PUSH_IMMED_1  DB      Z_IMMED, Z_END

P_POP_SEG       LABEL   BYTE
                X_CALL  TPOP
                X_BITS  38H
                X_CALL  SREG
                X_END

P_POP_MEM       LABEL   BYTE
                X_CALL  TPOP
                DB      Z_MOD_REG_RM, Z_MEM, Z_END

                
P_JMPS          LABEL   BYTE
                X_CALL  TJMP
                X_SHORT
                X_END

P_JMPN          LABEL   BYTE
                X_CALL  TJMP
                X_NEAR
                X_END

P_JMPF          LABEL   BYTE
                X_CALL  TJMP
                X_FAR
                X_END

P_CALLN         LABEL   BYTE
                X_CALL  TCALL
                X_NEAR
                X_END

P_CALLF         LABEL   BYTE
                X_CALL  TCALL
                X_FAR
                X_END

P_JCXZ          DB      "J"
                X_O16   P_JCXZ1
                DB      "E"
P_JCXZ1         DB      "CXZ", Z_OP
                X_SHORT
                X_END

P_LOOP          LABEL   BYTE
                X_CALL  TLOOP
                X_OP
                X_SHORT
                X_END

P_LOOPE         LABEL   BYTE
                X_CALL  TLOOP
                DB      "E", Z_OP
                X_SHORT
                X_END

P_LOOPNE        LABEL   BYTE
                X_CALL  TLOOP
                DB      "NE", Z_OP
                X_SHORT
                X_END

                X_SUB   TLOOP
                DB      "LOOP", Z_RET

P_SET           DB      "SET"
                X_CALL  COND
                X_BYTE_OP
                DB      Z_OP, Z_MOD_REG_RM, Z_MEM, Z_END

P_JCOND8        DB      "J"
                X_CALL  COND
                X_OP
                X_SHORT
                X_END

P_JCOND16       DB      "J"
                X_CALL  COND
                DB      Z_OP
                X_NEAR
                X_END

P_JMP_INDN      LABEL   BYTE
                X_CALL  TJMP
P_INDN          LABEL   BYTE
                X_WORD_OP
                DB      Z_MEM, Z_END

P_JMP_INDF      LABEL   BYTE
                X_CALL  TJMP
P_INDF          LABEL   BYTE
                X_FAR_OP
                DB      Z_MEM, Z_END

P_CALL_INDN     LABEL   BYTE
                X_CALL  TCALL
                X_JUMP  P_INDN

P_CALL_INDF     LABEL   BYTE
                X_CALL  TCALL
                X_JUMP  P_INDF

                X_SUB   TCALL
                DB      "CALL", Z_OP, Z_RET

                X_SUB   TJMP
                X_EMPTY
                DB      "JMP", Z_OP, Z_RET

                X_SUB   TMOV
                DB      "MOV", Z_OP, Z_RET

P_IN1           DB      "IN", Z_OP, Z_ACCUM
                X_COMMA
                DB      Z_BYTE, Z_END

P_IN2           DB      "IN", Z_OP, Z_ACCUM
                X_COMMA
                DB      "DX", Z_END

P_OUT1          DB      "OUT", Z_OP, Z_BYTE
                X_COMMA
                DB      Z_ACCUM, Z_END

P_OUT2          DB      "OUT", Z_OP, "DX"
                X_COMMA
                DB      Z_ACCUM, Z_END

P_INC1          DB      "INC"
                X_CALL  OP_MEM

P_DEC1          DB      "DEC"
                X_CALL  OP_MEM
P_INC2          DB      "INC", Z_OP
                X_WORD_OP
                DB      Z_REGA, Z_END

P_DEC2          DB      "DEC", Z_OP
                X_WORD_OP
                DB      Z_REGA, Z_END

P_BSF           DB      "BSF"
                X_JUMP  P_BS_1
P_BSR           DB      "BSR"
P_BS_1          LABEL   BYTE
                X_WORD_OP
                X_CALL  OP_REG_MEM               ; (OP) reg, mem (END)

P_BT1           DB      Z_MOD_REG_RM
                X_LOAD_REG
                X_CALL  BIT_TESTS
                X_OP
                X_MEM
                X_COMMA
                DB      Z_BYTE, Z_END

P_BT2           DB      Z_MOD_REG_RM
                X_BITS  38H
                X_CALL  BIT_TESTS
                X_OP
                X_MEM
                X_COMMA
                DB      Z_REG, Z_END

                X_SUB   BIT_TESTS
                DB      "BT"
                X_BITS  03H
                X_TABLE
                DW      BT00, BT01, BT02, BT03
BT00            DB      Z_RET
BT01            DB      "S", Z_RET
BT02            DB      "R", Z_RET
BT03            DB      "C", Z_RET


P_ROTATE1       DB      Z_MOD_REG_RM
                X_CALL  ROTATE
                DB      Z_OP, Z_MEM
                X_COMMA
                DB      "1", Z_END

P_ROTATE2       DB      Z_MOD_REG_RM
                X_CALL  ROTATE
                DB      Z_OP, Z_MEM
                X_COMMA
                DB      "CL", Z_END

P_ROTATE3       DB      Z_MOD_REG_RM
                X_CALL  ROTATE
                DB      Z_OP, Z_MEM
                X_COMMA
                DB      Z_BYTE, Z_END


                X_SUB   ROTATE
                X_LOAD_REG
                X_TABLE
                DW      ROTATE00, ROTATE01, ROTATE02, ROTATE03
                DW      ROTATE04, ROTATE05, ROTATE06, ROTATE07

ROTATE00        DB      "ROL", Z_RET
ROTATE01        DB      "ROR", Z_RET
ROTATE02        DB      "RCL", Z_RET
ROTATE03        DB      "RCR", Z_RET
                X_SUB   TSHL
ROTATE04        DB      "SHL", Z_RET
                X_SUB   TSHR
ROTATE05        DB      "SHR", Z_RET
ROTATE06        DB      "???", Z_RET
ROTATE07        DB      "SAR", Z_RET

                X_SUB   TTEST
                DB      "TEST", Z_OP, Z_RET

;
; TEST
;       reg, reg
;       reg, mem
;
P_TEST1         LABEL   BYTE
                X_CALL  TTEST
                X_CALL  REG_MEM                 ; reg, mem (END)

;
; Does not return!
;
; (OP) reg, mem (END)
;
                X_SUB   OP_REG_MEM
                X_OP
;
; reg, mem (END)
;
                X_SUB   REG_MEM
                DB      Z_MOD_REG_RM, Z_REG
                X_COMMA
                DB      Z_MEM, Z_END



;
; (OP) mem.w (END)
;
                X_SUB   WMEM
                X_WORD_OP
;
; (OP) mem (END)
;
                X_SUB   OP_MEM
                DB      Z_OP, Z_MEM, Z_END

;
; TEST
;       reg, immed
;       mem, immed
;
P_TEST2         LABEL   BYTE
                X_CALL  TTEST
                X_MEM
                X_COMMA
                DB      Z_IMMED, Z_END

;
; TEST
;       accum, immed
;
P_TEST3         LABEL   BYTE
                X_CALL  TTEST
                X_ACCUM
                X_COMMA
                DB      Z_IMMED, Z_END

;
; XCHG
;       reg, reg
;       reg, mem
;
P_XCHG1         DB      "XCHG", Z_OP
                X_CALL  REG_MEM                 ; reg, mem (END)

;
; XCHG
;       accum, reg
;
P_XCHG2         DB      "XCHG", Z_OP, Z_ACCUM
                X_COMMA
                DB      Z_REGA, Z_END


P_NOT           DB      "NOT"
                X_CALL  OP_MEM

P_NEG           DB      "NEG"
                X_CALL  OP_MEM

P_IMUL          DB      "I"
P_MUL           DB      "MUL"
                X_CALL  OP_MEM

P_IDIV          DB      "I"
P_DIV           DB      "DIV"
                X_CALL  OP_MEM

P_IMUL_IMMED    DB      "IMUL", Z_OP
                X_WORD_OP
                X_NOT
                X_TEST  02H, P_IMUL_I_1
                X_SIGNED
P_IMUL_I_1      DB      Z_MOD_REG_RM
                X_REG
                X_COMMA
                X_MEM
                X_COMMA
                DB      Z_IMMED, Z_END

P_IMUL_MEM      DB      "IMUL"
                X_WORD_OP
                X_CALL  OP_REG_MEM

P_RETN          DB      "RETN"
                X_JUMP  P_RET
P_RETF          DB      "RETF"
P_RET           LABEL   BYTE
                X_EMPTY
                X_TEST  01H, P_RET_1
                X_OP
                X_WORD
P_RET_1         DB      Z_END

P_LEA           DB      "LEA", Z_OP
                X_CALL  REG_MEM                 ; reg, mem (END)

;
; MOV
;       reg, reg
;       mem, reg
;       reg, mem
;
P_MOV1          LABEL   BYTE
                X_CALL  TMOV
                X_JUMP  DIRECTION

;
; MOV
;       mem, immed
;
P_MOV2          LABEL   BYTE
                X_CALL  TMOV
                DB      Z_MOD_REG_RM, Z_MEM
                X_COMMA
                DB      Z_IMMED, Z_END

;
; MOV
;       reg, immed
;
P_MOV3B         LABEL   BYTE
                X_BYTE_OP
                X_JUMP  P_MOV3

P_MOV3W         LABEL   BYTE
                X_WORD_OP
P_MOV3          LABEL   BYTE
                X_CALL  TMOV
                X_REGA
                X_COMMA
                DB      Z_IMMED, Z_END
;
; MOV
;       mem, accum
;       accum, mem
P_MOV4          LABEL   BYTE
                X_CALL  TMOV
                X_TEST  02H, P_MOV4_1
                X_ACCUM
                X_COMMA
                X_DISP
                X_END
P_MOV4_1        LABEL   BYTE
                X_DISP
                X_COMMA
                DB      Z_ACCUM, Z_END

;
; MOV
;       sreg, reg
;       sreg, mem
;       reg, sreg
;       mem, sreg
;
P_MOV5          LABEL   BYTE
                X_CALL  TMOV
                X_MOD_REG_RM
                X_WORD_OP
                X_TEST  02H, P_MOV5_1
                X_MEM
                X_COMMA
                X_LOAD_REG
                X_CALL  SREG
                X_END
P_MOV5_1        LABEL   BYTE
                X_LOAD_REG
                X_CALL  SREG
                X_COMMA
                DB      Z_MEM, Z_END


;
; ADD, OR, ADC, SBB, AND, SUB, XOR, CMP
;       reg, reg
;       mem, reg
;       reg, mem
;
P_ARITH1        LABEL   BYTE
                X_BITS  38H
                X_CALL  ARITH
                X_OP
DIRECTION       LABEL   BYTE
                X_MOD_REG_RM
                X_OPCODE
                X_TEST  02H, DIR1                       ; d=1?
                X_MEM
                X_COMMA
                DB      Z_REG, Z_END
DIR1            DB      Z_REG
                X_COMMA
                DB      Z_MEM, Z_END

;
; ADD, OR, ADC, SBB, AND, SUB, XOR, CMP
;       reg, immed
;       mem, immed
;
P_ARITH2        LABEL   BYTE
                X_NOT
                X_TEST  02H, P_ARITH2_1                 ; s=0?
                X_SIGNED
P_ARITH2_1      LABEL   BYTE
                X_MOD_REG_RM
                X_LOAD_REG
                X_CALL  ARITH
                X_OP
                X_MEM
                X_COMMA
                DB      Z_IMMED, Z_END

;
; ADD, OR, ADC, SBB, AND, SUB, XOR, CMP
;       accum, immed
;
P_ARITH3        LABEL   BYTE
                X_BITS  38H
                X_CALL  ARITH
                DB      Z_OP, Z_ACCUM
                X_COMMA
                DB      Z_IMMED, Z_END

;
; Insert arimthmetic instruction ?????aaa)
;
                X_SUB   ARITH
                X_BITS  07H
                X_TABLE
                DW      ARITH00, ARITH01, ARITH02, ARITH03
                DW      ARITH04, ARITH05, ARITH06, ARITH07

ARITH00         DB      "ADD", Z_RET
ARITH02         DB      "ADC", Z_RET
ARITH03         DB      "SBB", Z_RET
ARITH04         DB      "AND", Z_RET
ARITH05         DB      "SUB", Z_RET
ARITH06         DB      "X"
ARITH01         DB      "OR", Z_RET
ARITH07         DB      "CMP", Z_RET

;
; Insert condition code (accu = ????cccc)
;
                X_SUB   COND
                X_BITS  0FH
                X_TABLE
                DW      COND00, COND01, COND02, COND03
                DW      COND04, COND05, COND06, COND07
                DW      COND08, COND09, COND0A, COND0B
                DW      COND0C, COND0D, COND0E, COND0F
COND01          DB      "N"
COND00          DB      "O", Z_RET
COND03          DB      "N"
COND02          DB      "C", Z_RET
COND05          DB      "N"
COND04          DB      "E", Z_RET
COND06          DB      "BE", Z_RET
COND07          DB      "A", Z_RET
COND09          DB      "N"
COND08          DB      "S", Z_RET
COND0A          DB      "PE", Z_RET
COND0B          DB      "PO", Z_RET
COND0C          DB      "L", Z_RET
COND0D          DB      "GE", Z_RET
COND0E          DB      "LE", Z_RET
COND0F          DB      "G", Z_RET


;
; Insert segment register (accu = ?????sss)
;
                X_SUB   SREG
                X_CALL  SREG_1
                DB      "S", Z_RET

                X_SUB   SREG_1
                X_BITS  07H
                X_TABLE
                DW      SREG00, SREG01, SREG02, SREG03
                DW      SREG04, SREG05, SREG06, SREG07

SREG00          DB      "E", Z_RET
SREG01          DB      "C", Z_RET
SREG02          DB      "S", Z_RET
SREG03          DB      "D", Z_RET
SREG04          DB      "F", Z_RET
SREG05          DB      "G", Z_RET
SREG06          DB      "?", Z_RET
SREG07          DB      "?", Z_RET

;
; Insert special register (reg = ?????qqq)
; Caller must insert "C", "D", or "T" before calling this subroutine
;
                X_SUB   SPECIAL
                DB      "R"
                X_LOAD_REG
                X_NIBBLE
                X_RET


;
; Insert "D" if operand size = 32
; Does not return!
;
                X_SUB   APPEND_D
                X_O16   APPEND_D1
                DB      "D"
APPEND_D1       DB      Z_END

;
; Operand separator
;
                X_SUB   P_COMMA
                DB      ", ", Z_RET


;
; Load reg field of mod,reg,r/m into accu
;
                X_SUB   P_LOAD_REG
                X_LD_MOD_REG_RM
                X_BITS  00111000B
                X_RET


;
; Floating point
;

P_ESC           DB      "F"
                X_OPCODE                        ; Opcode -> accu
                X_PUSH                          ; Push opcode
                X_MORE                          ; Get next byte
                X_MASK_EQ 0C0H, 0C0H, ESC_REG   ; mod = 11 ->
                X_BACK                          ; Back to mod,reg,r/m
                X_MOD_REG_RM
                X_NO_PTR
                X_POP
                X_BITS  07H
                X_TABLE
                DW      NPXMEM_D8, NPXMEM_D9, NPXMEM_DA, NPXMEM_DB
                DW      NPXMEM_DC, NPXMEM_DD, NPXMEM_DE, NPXMEM_DF

ESC_REG         LABEL   BYTE
                X_XCHG                          ; Get 1st byte, save 2nd
                X_BITS  07H
                X_TABLE
                DW      NPXREG_D8, NPXREG_D9, NPXREG_DA, NPXREG_DB
                DW      NPXREG_DC, NPXREG_DD, NPXREG_DE, NPXREG_DF


NPXMEM_DA       LABEL   BYTE                    ; 32-bit real
NPXMEM_DE       LABEL   BYTE                    ; Integer
                DB      "I"
NPXMEM_D8       LABEL   BYTE                    ; 32-bit real
NPXMEM_DC       LABEL   BYTE                    ; 64-bit real
                X_LD_MOD_REG_RM
                X_CALL  NPX1
                X_OP
                X_MEM
                X_END


NPXMEM_D9       LABEL   BYTE
                X_LD_MOD_REG_RM
                X_CALL  NPX3
                X_OP
                X_MEM
                X_END

                X_SUB   NPX3
                X_BITS  38H
                X_TABLE
                DW      P_LD,        NPX_FAILURE, P_ST,        P_STP
                DW      P_LDENV,     P_LDCW,      P_STENV,     P_STCW

P_LDENV         DB      "LDENV", Z_RET
P_LDCW          DB      "LDCW", Z_RET
P_STENV         DB      "STENV", Z_RET
P_STCW          DB      "STCW", Z_RET

NPXMEM_DB       LABEL   BYTE
                X_LD_MOD_REG_RM
                X_CALL  NPX4
                X_OP
                X_MEM
                X_END

                X_SUB   NPX4
                X_BITS  38H
                X_TABLE
                DW      P_ILD, NPX_FAILURE, P_IST, P_ISTP
                DW      NPX_FAILURE, P_LD, NPX_FAILURE, P_STP

P_ILD           DB      "I"
P_LD            DB      "LD", Z_RET
P_IST           DB      "I"
P_ST            DB      "ST", Z_RET
P_ISTP          DB      "I"
P_STP           DB      "STP", Z_RET

NPXMEM_DD       LABEL   BYTE
                X_LD_MOD_REG_RM
                X_CALL  NPX5
                X_OP
                X_MEM
                X_END

                X_SUB   NPX5
                X_BITS  38H
                X_TABLE
                DW      P_LD, NPX_FAILURE, P_ST, P_STP
                DW      P_RSTOR, NPX_FAILURE, P_SAVE, P_STSW

P_RSTOR         DB      "RSTOR", Z_RET
P_SAVE          DB      "SAVE", Z_RET
P_STSW          DB      "STSW", Z_RET


NPXMEM_DF       LABEL   BYTE
                X_LD_MOD_REG_RM
                X_CALL  NPX6
                X_OP
                X_MEM
                X_END

                X_SUB   NPX6
                X_BITS  38H
                X_TABLE
                DW      P_ILD, NPX_FAILURE, P_IST, P_ISTP
                DW      P_BLD, P_ILD, P_BSTP, P_ISTP

P_BLD           DB      "BLD", Z_RET
P_BSTP          DB      "BSTP", Z_RET

NPXREG_D8       LABEL   BYTE
                X_POP
                X_PUSH
                X_CALL  NPX1
                X_POP
                DB      Z_OP, "ST"
                X_COMMA
                X_CALL  FREG
                X_END

NPXREG_DC       LABEL   BYTE
                X_POP
                X_PUSH
                X_CALL  NPX2
                X_POP
                X_OP
                X_CALL  FREG
                X_COMMA
                DB      "ST"
                X_END

                X_SUB   NPX1
                X_BITS  38H
                X_TABLE
                DW      NPX_ADD, NPX_MUL, NPX_COM, NPX_COMP
                DW      NPX_SUB, NPX_SUBR, NPX_DIV, NPX_DIVR

                X_SUB   NPX2
                X_BITS  38H
                X_TABLE
                DW      NPX_ADD, NPX_MUL, NPX_COM, NPX_COMP
                DW      NPX_SUBR, NPX_SUB, NPX_DIVR, NPX_DIV

NPX_ADD         DB      "ADD", Z_RET
NPX_MUL         DB      "MUL", Z_RET
NPX_COM         DB      "COM", Z_RET
NPX_COMP        DB      "COMP", Z_RET
NPX_SUB         DB      "SUB", Z_RET
NPX_SUBR        DB      "SUBR", Z_RET
NPX_DIV         DB      "DIV", Z_RET
NPX_DIVR        DB      "DIVR", Z_RET

NPXREG_D9       LABEL   BYTE
                X_POP
                X_PUSH
                X_BITS  38H
                X_TABLE
                DW      NPXREG_D9_0, NPXREG_D9_1, NPXREG_D9_2, NPX_FAILURE
                DW      NPXREG_D9_4, NPXREG_D9_5, NPXREG_D9_67, NPXREG_D9_67

NPXREG_D9_0     DB      "LD"
                X_CALL  NPX_ST
NPXREG_D9_1     DB      "XCH"
                X_CALL  NPX_ST
NPXREG_D9_2     DB      "NOP"
                X_CALL  NPX_ST

NPXREG_D9_4     LABEL   BYTE
                X_POP
                X_BITS  07H
                X_TABLE
                DW      NPXREG_D9_4_0, NPXREG_D9_4_1, NPX_FAILURE, NPX_FAILURE
                DW      NPXREG_D9_4_4, NPXREG_D9_4_5, NPX_FAILURE, NPX_FAILURE

NPXREG_D9_4_0   DB      "CHS", Z_END
NPXREG_D9_4_1   DB      "ABS", Z_END
NPXREG_D9_4_4   DB      "TST", Z_END
NPXREG_D9_4_5   DB      "XAM", Z_END

NPXREG_D9_5     DB      "LD"
                X_POP
                X_BITS  07H
                X_TABLE
                DW      NPXREG_D9_5_0, NPXREG_D9_5_1, NPXREG_D9_5_2, NPXREG_D9_5_3
                DW      NPXREG_D9_5_4, NPXREG_D9_5_5, NPXREG_D9_5_6, NPX_FAILURE

NPXREG_D9_5_0   DB      "1", Z_END
NPXREG_D9_5_1   DB      "2T", Z_END
NPXREG_D9_5_2   DB      "2E", Z_END
NPXREG_D9_5_3   DB      "PI", Z_END
NPXREG_D9_5_4   DB      "LG2", Z_END
NPXREG_D9_5_5   DB      "LN2", Z_END
NPXREG_D9_5_6   DB      "Z", Z_END

NPXREG_D9_67    LABEL   BYTE
                X_POP
                X_BITS  0FH
                X_TABLE
                DW      NPXREG_D9_6_0, NPXREG_D9_6_1, NPXREG_D9_6_2, NPXREG_D9_6_3
                DW      NPXREG_D9_6_4, NPXREG_D9_6_5, NPXREG_D9_6_6, NPXREG_D9_6_7
                DW      NPXREG_D9_7_0, NPXREG_D9_7_1, NPXREG_D9_7_2, NPXREG_D9_7_3
                DW      NPXREG_D9_7_4, NPXREG_D9_7_5, NPXREG_D9_7_6, NPXREG_D9_7_7

NPXREG_D9_6_0   DB      "2XM1", Z_END
NPXREG_D9_6_1   DB      "YL2X", Z_END
NPXREG_D9_6_2   DB      "PTAN", Z_END
NPXREG_D9_6_3   DB      "PATAN", Z_END
NPXREG_D9_6_4   DB      "XTRACT", Z_END
NPXREG_D9_6_5   DB      "PREM1", Z_END
NPXREG_D9_6_6   DB      "DECSTP", Z_END
NPXREG_D9_6_7   DB      "INCSTP", Z_END
NPXREG_D9_7_0   DB      "PREM", Z_END
NPXREG_D9_7_1   DB      "YL2XP1", Z_END
NPXREG_D9_7_2   DB      "SQRT", Z_END
NPXREG_D9_7_3   DB      "SINCOS", Z_END
NPXREG_D9_7_4   DB      "RNDINT", Z_END
NPXREG_D9_7_5   DB      "SCALE", Z_END
NPXREG_D9_7_6   DB      "SIN", Z_END
NPXREG_D9_7_7   DB      "COS", Z_END


NPXREG_DA       LABEL   BYTE
                X_POP
                X_SWITCH
                X_CASE  0E9H, NPXREG_DA_E9
                X_JUMP  NPX_FAILURE

NPXREG_DA_E9    DB      "U"
NPXREG_DE_D9    DB      "COMPP", Z_END

NPXREG_DB       LABEL   BYTE
                X_POP
                X_SWITCH
                X_CASE  0E2H, NPXREG_DB_E2
                X_CASE  0E3H, NPXREG_DB_E3
                X_JUMP  NPX_FAILURE

NPXREG_DB_E2    DB      "CLEX", Z_END
NPXREG_DB_E3    DB      "INIT", Z_END

NPXREG_DD       LABEL   BYTE
                X_POP
                X_PUSH
                X_BITS  38H
                X_TABLE
                DW      NPXREG_DD_0, NPXREG_DD_1, NPXREG_DD_2, NPXREG_DD_3
                DW      NPXREG_DD_4, NPXREG_DD_5, NPX_FAILURE, NPX_FAILURE

NPXREG_DD_0     DB      "FREE"
                X_CALL  NPX_ST
NPXREG_DD_1     DB      "XCH"
                X_CALL  NPX_ST
NPXREG_DD_2     DB      "ST"
                X_CALL  NPX_ST
NPXREG_DD_3     DB      "STP"
                X_CALL  NPX_ST
NPXREG_DD_4     DB      "UCOM"
                X_CALL  NPX_ST
NPXREG_DD_5     DB      "UCOMP"
                X_CALL  NPX_ST

NPXREG_DE       LABEL   BYTE
                X_POP
                X_SWITCH
                X_CASE  0D9H, NPXREG_DE_D9
                X_PUSH
                X_CALL  NPX2
                X_POP
                DB      "P"
                X_CALL  NPX_ST

NPXREG_DF       LABEL   BYTE
                X_POP
                X_SWITCH
                X_CASE   0E0H, NPXREG_DF_E0
                X_JUMP  NPX_FAILURE

NPXREG_DF_E0    DB      "STSW", Z_OP, "AX", Z_END


;
; Insert "<Tab>ST(accu)" and end
;

                X_SUB   NPX_ST
                X_OP
                X_CALL  FREG
                X_END

;
; Insert ST(accu)
;
                X_SUB   FREG
                DB      "ST("
                X_BITS  07H
                X_NIBBLE
                DB      ")"
                X_RET


NPX_FAILURE     DB      "???", Z_END


; ============================================================================
; End of disassembler language program
; ============================================================================


DISASM_CALL     LABEL   WORD
CIDX            =       0
                REPT    CALLCNT
                VDW     CALLV, %CIDX
CIDX            =       CIDX + 1
                ENDM
SV_DATA         ENDS





SV_CODE         SEGMENT

                ASSUME  CS:SV_CODE, DS:SV_DATA


XCHAR           MACRO   CHR
                MOV     BYTE PTR DS:[DI], CHR
                INC     DI
                ENDM


XTEXT           PROC    NEAR
                PUSH    AX
                PUSH    BX
                MOV     BX, DX
XTEXT1:         MOV     AL, [BX]
                OR      AL, AL
                JZ      SHORT XTEXT9
                XCHAR   AL
                INC     BX
                JMP     SHORT XTEXT1
XTEXT9:         POP     BX
                POP     AX
                RET
XTEXT           ENDP


XDWORD          PROC    NEAR
                ROR     EAX, 16
                CALL    XWORD
                ROR     EAX, 16
                CALL    XWORD
                RET
XDWORD          ENDP

XWORD           PROC    NEAR
                XCHG    AL, AH
                CALL    XBYTE
                XCHG    AL, AH
                CALL    XBYTE
                RET
XWORD           ENDP


XBYTE           PROC    NEAR
                PUSH    AX
                SHR     AL, 4
                CALL    XNIBBLE
                POP     AX
                PUSH    AX
                CALL    XNIBBLE
                POP     AX
                RET
XBYTE           ENDP

XNIBBLE         PROC    NEAR
                AND     AL, 0FH
                ADD     AL, 30H
                CMP     AL, 3AH
                JB      SHORT XNIB1
                ADD     AL, 7
XNIB1:          XCHAR   AL
                RET
XNIBBLE         ENDP


XSBYTE          PROC    NEAR
                PUSH    AX
                TEST    AL, 80H
                JNZ     SHORT XSBYTE2
                XCHAR   "+"
XSBYTE1:        CALL    XBYTE
                POP     AX
                RET
XSBYTE2:        XCHAR   "-"
                NEG     AL
                JMP     SHORT XSBYTE1
XSBYTE          ENDP


;
; In:   AL              Bits 0..2: register number
;       OPSIZE          operand size (0=BYTE, 1=WORD)
;       PFX_OSIZE       Operand size
;
XREG            PROC    NEAR
                PUSH    AX
                PUSH    BX
                LEA     BX, $REG8
                CMP     OPSIZE, 0
                JE      SHORT XREG1
                LEA     BX, $REG16
                CALL    OSIZE                   ; Operand size
                JZ      SHORT XREG1             ; 16 ->
                XCHAR   "E"
XREG1:          AND     AX, 07H
                SHL     AX, 1
                ADD     BX, AX
                MOV     AX, [BX]
                XCHAR   AL
                XCHAR   AH
                POP     BX
                POP     AX
                RET
XREG            ENDP


;
; In:   AL      Register number (bits 0..2)
;
REG32           PROC    NEAR
                XCHAR   "E"
REG16           PROC    NEAR
                PUSH    AX
                PUSH    BX
                MOV     BL, AL
                AND     BX, 07H
                SHL     BX, 1
                MOV     AX, WORD PTR $REG16[BX]
                XCHAR   AL
                XCHAR   AH
                POP     BX
                POP     AX
                RET
REG16           ENDP
REG32           ENDP



;
; In:   OPSIZE  
;       FLAGS
;
XIMMED          PROC    NEAR
                CMP     OPSIZE, 0
                JNE     SHORT XIMMED_WORD
                MOV     AL, FS:[ESI]
                INC     ESI
                CALL    XBYTE
                RET

XIMMED_WORD:    TEST    FLAGS, F_SIGNED         ; s?
                JZ      SHORT XIMMED_WORD1      ; No ->
                MOV     AL, FS:[ESI]
                INC     ESI
                CALL    XSBYTE
                RET

XIMMED_WORD1:   CALL    OSIZE                   ; Operand size
                JZ      SHORT XIMMED_WORD2      ; 16 ->
                MOV     EAX, FS:[ESI]
                ADD     ESI, 4
                MOV     SYM_ADDR, EAX
                MOV     SYM_FLAG, SYM_ANY
                CALL    XDWORD
                RET

XIMMED_WORD2:   MOV     AX, FS:[ESI]
                ADD     ESI, 2
                CALL    XWORD
                RET
XIMMED          ENDP


;
; Return address size
;
; Out:  NZ      32 bit
;       ZR      16 bit
;
ASIZE           PROC    NEAR
                CMP     MODE_32, 0
                JE      SHORT ASIZE16
ASIZE32:        CMP     PFX_ASIZE, SIZE_ON
                RET
ASIZE16:        CMP     PFX_ASIZE, SIZE_OFF
                RET
ASIZE           ENDP


;
; Return operand size
;
; Out:  NZ      32 bit
;       ZR      16 bit
;
OSIZE           PROC    NEAR
                CMP     MODE_32, 0
                JE      SHORT OSIZE16
OSIZE32:        CMP     PFX_OSIZE, SIZE_ON
                RET
OSIZE16:        CMP     PFX_OSIZE, SIZE_OFF
                RET
OSIZE           ENDP



;
;
; In:   FS:ESI  Source
;       DS:DI   Destination
;       ECX     Value of EIP at FS:ESI (ESI, if no relocation used)
;       AL      address/operand size: 0:16, 1:32
;
; Out:  DS:DI   Pointer to terminating byte of destination (0)
;       AL      SYM_NONE=no symbol; SYM_ANY, SYM_TEXT, SYM_DATA
;       AH      Non-zero, if an empty line should be output after this line
;       EDX     Symbol address (if AL<>SYM_NONE)
;
DISASM          PROC    NEAR
                SUB     ECX, ESI
                MOV     RELOCATION, ECX
                MOV     MODE_32, AL
                MOV     PFX_ASIZE, SIZE_OFF
                MOV     PFX_OSIZE, SIZE_OFF
                MOV     PFX_SEG, 0
                MOV     DISASM_SP, OFFSET SV_DATA:DISASM_STACK
                MOV     ACCU_SP, OFFSET SV_DATA:ACCU_STACK
                MOV     NOT_FLAG, 0
                MOV     FLAGS, 0
                MOV     SYM_FLAG, SYM_NONE
DISASM_FETCH::  LEA     BX, DISASM_PROGRAM
DISASM_MORE::   MOV     AL, FS:[ESI]
                INC     ESI
                MOV     ACCU, AL
                MOV     OPCODE, AL
                AND     AL, 01H
                MOV     OPSIZE, AL
DISASM_NEXT:    MOV     CL, [BX]
                INC     BX
                CMP     CL, 20H                 ; Command?
                JB      SHORT DISASM1           ; Yes ->
                CMP     CL, 80H                 ; Subroutine call?
                JAE     SHORT DISASM2           ; Yes ->
                XCHAR   CL
                JMP     SHORT DISASM_NEXT

DISASM1:        MOVZX   ECX, CL
                CALL    DISASM_JMP[ECX*2]
                JMP     SHORT DISASM_NEXT

DISASM2:        MOVZX   ECX, CL
                MOV     DX, BX
                SUB     DISASM_SP, 2
                MOV     BX, DISASM_SP
                MOV     [BX], DX
                MOV     BX, DISASM_CALL[ECX*2-80H*2]
                JMP     SHORT DISASM_NEXT

DISASM          ENDP


Q_END:          POP     AX
                MOV     BYTE PTR [DI], 0
                MOV     AL, SYM_FLAG
                MOV     AH, FLAGS
                AND     AH, F_EMPTY
                MOV     EDX, SYM_ADDR
                RET

Q_MORE:         POP     AX
                JMP     DISASM_MORE

Q_EXT:          MOV     CL, [BX]
                INC     BX
                CMP     CL, 4
                JAE     Q_END
                MOVZX   ECX, CL
                JMP     EXT_JMP[ECX*2]

Q_PREFIX:       MOV     DX, [BX+0]
                MOV     AL, [BX+2]
                MOV     BX, DX
                MOV     [BX], AL
                POP     AX
                JMP     DISASM_FETCH

Q_RET:          MOV     BX, DISASM_SP
                MOV     BX, [BX]
                ADD     DISASM_SP, 2
                RET

Q_OPCODE:       MOV     AL, OPCODE
                MOV     ACCU, AL
                RET

Q_NOT:          XOR     NOT_FLAG, 1
                RET


Q_OP:           XCHAR   TAB
                RET

Q_FLAGS:        MOV     AL, [BX]
                INC     BX
                OR      FLAGS, AL
                RET

Q_JMP_DST:      MOV     AL, [BX]
                INC     BX
                CMP     AL, 1
                JE      SHORT Q_NEAR
                CMP     AL, 2
                JE      Q_FAR
Q_SHORT:        MOVSX   EAX, BYTE PTR FS:[ESI]
                INC     ESI
                JMP     SHORT DNEAR2
                
Q_NEAR:         CALL    ASIZE
                JNZ     SHORT DNEAR1
                MOVSX   EAX, WORD PTR FS:[ESI]
                ADD     ESI, 2
                JMP     SHORT DNEAR2
DNEAR1:         MOV     EAX, FS:[ESI]
                ADD     ESI, 4
DNEAR2:         ADD     EAX, ESI
                ADD     EAX, RELOCATION
                MOV     SYM_ADDR, EAX
                MOV     SYM_FLAG, SYM_TEXT
                CALL    XDWORD
                RET


Q_SWITCH:       MOVZX   ECX, BYTE PTR [BX]
                INC     BX
                JECXZ   DSWITCH9
DSWITCH1:       MOV     AL, [BX+0]
                CMP     ACCU, AL
                JE      SHORT DSWITCH2
                ADD     BX, 3
                LOOP    DSWITCH1
DSWITCH9:       RET

DSWITCH2:       MOV     BX, [BX+1]
                RET

Q_MASK_EQ:      MOV     DL, [BX+0]
                MOV     DH, [BX+1]
                ADD     BX, 2
                MOV     AL, ACCU
                AND     AL, DL
                CMP     AL, DH
                SETE    AL
COND_JUMP:      CMP     AL, NOT_FLAG
                MOV     NOT_FLAG, 0
                JNE     SHORT Q_JUMP
                ADD     BX, 2
                RET

Q_JUMP:         MOV     BX, [BX]
                RET

Q_LD_MOD_REG_RM:MOV     AL, MOD_REG_RM
                MOV     ACCU, AL
                RET

Q_TEST:         MOV     AL, [BX]
                INC     BX
                TEST    ACCU, AL
                SETNE   AL
                JMP     COND_JUMP

Q_BITS:         MOVZX   DX, BYTE PTR [BX]
                INC     BX
                BSF     CX, DX
                AND     ACCU, DL
                SHR     ACCU, CL
                RET

Q_CMP:          MOV     DX, [BX+0]
                MOV     AL, [BX+2]
                ADD     BX, 3
                XCHG    BX, DX
                CMP     AL, [BX]
                SETE    AL
                XCHG    BX, DX
                JMP     COND_JUMP

Q_IMMED:        CALL    XIMMED
                AND     FLAGS, NOT F_SIGNED
                RET

Q_MOD_REG_RM:   MOV     AL, FS:[ESI]            ; mod,reg,r/m
                INC     ESI
                MOV     MOD_REG_RM, AL
                CALL    ASIZE                   ; Address size
                JZ      SHORT Q_MOD_REG_RM_9    ; 16 ->
                CMP     AL, 0C0H                ; mod=11?
                JAE     SHORT Q_MOD_REG_RM_9    ; Yes -> no SIB
                AND     AL, 07H                 ; r/m
                CMP     AL, 04H                 ; SIB?
                JNE     SHORT Q_MOD_REG_RM_9    ; No ->
                MOV     AL, FS:[ESI]
                INC     ESI
                MOV     SIB, AL
Q_MOD_REG_RM_9: RET


Q_REG:          MOV     AL, MOD_REG_RM
                SHR     AL, 3
                CALL    XREG
                RET

Q_REGA:         MOV     AL, ACCU
                CALL    XREG
                RET

Q_MEM:          MOV     AL, MOD_REG_RM
                CMP     AL, 40H
                JB      SHORT QMEM00
                CMP     AL, 80H
                JB      SHORT QMEM01
                CMP     AL, 0C0H
                JB      SHORT QMEM10
QMEM11:         CALL    XREG
                RET

QMEM00:         AND     AL, 07H                 ; r/m
                MOV     AH, 06H
                CALL    ASIZE                   ; Address size
                JZ      SHORT QMEM00_1          ; 16 -> 6 is special case
                MOV     AH, 05H                 ; 32 -> 5 is special case
QMEM00_1:       CMP     AL, AH
                JE      SHORT QMEM00_2
                CALL    QMEM_IND
                JMP     SHORT QMEM_END

QMEM00_2:       CALL    QMEM_BEGIN
                CALL    QMEM_DISPW
                JMP     SHORT QMEM_END

QMEM01:         CALL    QMEM_IND
                MOV     AL, FS:[ESI]
                INC     ESI
                CALL    XSBYTE
QMEM_END:       XCHAR   "]"
                RET

QMEM10:         CALL    QMEM_IND
                XCHAR   "+"
                CALL    QMEM_DISPW
                JMP     SHORT QMEM_END


QMEM_BEGIN      PROC    NEAR
                TEST    FLAGS, F_NO_PTR
                JNZ     SHORT QMEM_B2A
                LEA     DX, $BYTE
                CMP     OPSIZE, 00H             ; byte
                JE      SHORT QMEM_B2
                LEA     DX, $DWORD
                CMP     OPSIZE, 01H             ; word
                JE      SHORT QMEM_B1
                CALL    OSIZE                   ; Operand size
                JZ      SHORT QMEM_B2           ; 16 ->
                XCHAR   "f"
                INC     DX
                JMP     SHORT QMEM_B2
QMEM_B1:        CALL    OSIZE                   ; Operand size
                JNZ     SHORT QMEM_B2           ; 32 ->
                INC     DX
QMEM_B2:        CALL    XTEXT
                LEA     DX, $PTR
                CALL    XTEXT
QMEM_B2A:       MOV     AL, PFX_SEG
                OR      AL, AL
                JZ      SHORT QMEM_B3
                XCHAR   AL
                XCHAR   "S"
                XCHAR   ":"
QMEM_B3:        XCHAR   "["
                RET
QMEM_BEGIN      ENDP

QMEM_IND        PROC    NEAR
                CALL    QMEM_BEGIN
                MOV     AL, MOD_REG_RM
                AND     AL, 07H                 ; r/m
                CALL    ASIZE                   ; Address size
                JNZ     SHORT QMEM_IND32        ; 32 ->
                PUSH    BX
                LEA     BX, RM_TAB
                XLAT
                MOV     BX, "BX"
                MOV     AH, 01H
                CALL    QMEM_IND1
                MOV     BX, "BP"
                MOV     AH, 02H
                CALL    QMEM_IND1
                MOV     BX, "SI"
                MOV     AH, 04H
                CALL    QMEM_IND1
                MOV     BX, "DI"
                MOV     AH, 08H
                CALL    QMEM_IND1
                POP     BX
                RET

QMEM_IND32:     CMP     AL, 04H
                JE      SHORT QMEM_SIB
                CALL    REG32
                RET

QMEM_SIB:       MOV     AL, SIB
                AND     AL, 07H                 ; base
                CMP     MOD_REG_RM, 40H         ; mod=0?
                JAE     SHORT QMEM_SIB1         ; No -> normal case
                CMP     AL, 05H                 ; base=5?
                JNE     SHORT QMEM_SIB1         ; No -> normal case
                CALL    QMEM_DISPW              ; disp32+(scale*index)
                JMP     SHORT QMEM_SIB2

QMEM_SIB1:      MOV     AL, SIB
                CALL    REG32
QMEM_SIB2:      MOV     AL, SIB
                AND     AL, 7 SHL 3
                CMP     AL, 4 SHL 3
                JE      SHORT QMEM_SIB4
                XCHAR   "+"
                MOV     AL, SIB
                SHR     AL, 6
                AND     AL, 03H
                JZ      SHORT QMEM_SIB3
                MOV     CL, AL
                MOV     AL, 01H
                SHL     AL, CL
                ADD     AL, "0"
                XCHAR   AL
                XCHAR   "*"
QMEM_SIB3:      MOV     AL, SIB
                SHR     AL, 3
                CALL    REG32
QMEM_SIB4:      RET

QMEM_IND        ENDP



QMEM_IND1       PROC    NEAR
                TEST    AL, AH
                JZ      SHORT QMEM_IND9
                TEST    AL, 80H
                JZ      SHORT QMEM_IND2
                XCHAR   "+"
QMEM_IND2:      OR      AL, 80H
                CALL    ASIZE                   ; Address size
                JZ      SHORT QMEM_IND3         ; 16 -> 
                XCHAR   "E"
QMEM_IND3:      XCHAR   BH
                XCHAR   BL
QMEM_IND9:      RET
QMEM_IND1       ENDP


Q_ACCUM:        MOV     AL, 0
                CALL    XREG
                RET

Q_NIBBLE:       MOV     AL, ACCU
                CALL    XNIBBLE
                RET

Q_O16:          CALL    OSIZE                   ; Operand size
                SETZ    AL                      ; 16 -> AL:=1
                JMP     COND_JUMP

Q_A16:          CALL    ASIZE                   ; Address size
                SETZ    AL                      ; 16 -> AL:=1
                JMP     COND_JUMP

Q_BYTE:         MOV     AL, FS:[ESI]
                INC     ESI
                CALL    XBYTE
                RET

QMEM_DISPW:     CALL    ASIZE                   ; Address size
                JZ      SHORT Q_WORD            ; 16 ->
                CALL    Q_DWORD
                MOV     SYM_ADDR, EAX
                MOV     SYM_FLAG, SYM_DATA
                RET

Q_DISP:         XCHAR   "["
                CALL    QMEM_DISPW
                XCHAR   "]"
                RET

Q_DWORD:        MOV     EAX, FS:[ESI]
                ADD     ESI, 4
                CALL    XDWORD
                RET

Q_WORD:         MOV     AX, FS:[ESI]
                ADD     ESI, 2
                CALL    XWORD
                RET

Q_TABLE:        MOVZX   AX, ACCU
                SHL     AX, 1
                ADD     BX, AX
                MOV     BX, [BX]
                RET

Q_OP_SIZE:      MOV     AL, [BX]
                INC     BX
                MOV     OPSIZE, AL
                RET

Q_FAR:          CALL    ASIZE                   ; Address size
                JNZ     SHORT DFAR1             ; 32 ->
                MOVZX   ECX, WORD PTR FS:[ESI]  ; Oops, was MOVSX
                ADD     ESI, 2
                JMP     SHORT DFAR2
DFAR1:          MOV     ECX, FS:[ESI]           ; Get offset
                ADD     ESI, 4
DFAR2:          MOV     AX, FS:[ESI]            ; Get selector
                ADD     ESI, 2                  ; Skip selector
                CALL    XWORD                   ; Output selector
                XCHAR   ":"
                MOV     EAX, ECX                ; Oops, adding ESI was wrong!
                CALL    XDWORD                  ; Output offset
                RET

Q_PUSH:         PUSH    BX
                DEC     ACCU_SP
                MOV     BX, ACCU_SP
                MOV     AL, ACCU
                MOV     [BX], AL
                POP     BX
                RET

Q_POP:          PUSH    BX
                MOV     BX, ACCU_SP
                MOV     AL, [BX]
                MOV     ACCU, AL
                INC     ACCU_SP
                POP     BX
                RET

Q_XCHG:         PUSH    BX
                MOV     BX, ACCU_SP
                MOV     AL, ACCU
                XCHG    AL, [BX]
                MOV     ACCU, AL
                POP     BX
                RET

Q_BACK:         DEC     ESI
                RET

SV_CODE         ENDS

              ELSE

;
; No disassembler
;
SV_CODE         SEGMENT

                ASSUME  CS:SV_CODE, DS:NOTHING

DISASM          PROC    NEAR
                INC     ESI
                RET
DISASM          ENDP

SV_CODE         ENDS

              ENDIF

                END
