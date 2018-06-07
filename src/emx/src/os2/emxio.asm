;
; EMXIO.ASM
;
; Copyright (c) 1992-1998 by Eberhard Mattes
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
; As special exception, emxio.dll can be distributed without source code
; unless it has been changed.  If you modify emxio.dll, this exception
; no longer applies and you must remove this paragraph from all source
; files for emxio.dll.
;


                .386

IMPORT_OS2      =       0
                INCLUDE EMXDLL.INC

                PUBLIC  emx_revision


REV_INDEX_TXT   EQU     <"60">                  ; Revision index (as text)
REV_INDEX_BIN   =       60                      ; Revision index (as number)


TEXT32          SEGMENT

                ASSUME  DS:FLAT, ES:FLAT

$REV_INDEX      BYTE    REV_INDEX_TXT, 0

;
; This REXX-callable entrypoint returns a string indicating the
; version and revision of emxio.dll as integer. The number will be
; incremented for each revision and is used only for comparing.
;

                TALIGN  4
emx_revision    PROC    C PUBLIC USES EBX ESI EDI,
                        PROCNAME:PTR BYTE, NUMARGS:DWORD, ARGS:PTR RXSTRING,
                        QUEUENAME:PTR BYTE, RETSTR:PTR RXSTRING
              .IF     NUMARGS != 0              ; Arguments given?
                MOV     EAX, 1                  ; Yes: error
              .ELSE
                MOV     EBX, RETSTR             ; Return value
                ASSUME  EBX:PTR RXSTRING
                LEA     ESI, $REV_INDEX
                MOV     EDI, [EBX].RXSTRPTR
REV1:           LODSB
                STOSB
                OR      AL, AL
                JNZ     REV1
                MOV     EAX, EDI
                DEC     EAX
                SUB     EAX, [EBX].RXSTRPTR     ; Compute length of string
                MOV     [EBX].RXSTRLEN, EAX     ; Set string length
                ASSUME  EBX:NOTHING
                XOR     EAX, EAX                ; Successful
              .ENDIF
                RET

                MOV     EAX, PROCNAME           ; Avoid warning
                MOV     EAX, ARGS               ; Avoid warning
                MOV     EAX, QUEUENAME          ; Avoid warning
emx_revision    ENDP

;
; unsigned _inp8 (unsigned port)
;
                TALIGN  4
emx_inp8        PROC    C PUBLIC, PORT:DWORD
                MOV     EDX, PORT
                MOV     EAX, 0
                CALL    MODE16
                RET
emx_inp8        ENDP

;
; void _outp8 (unsigned port, unsigned value)
;
                TALIGN  4
emx_outp8       PROC    C PUBLIC, PORT:DWORD, VALUE:DWORD
                MOV     EDX, PORT
                MOV     ECX, VALUE
                MOV     EAX, 1
                CALL    MODE16
                RET
emx_outp8       ENDP

;
; unsigned _inp16 (unsigned port)
;
                TALIGN  4
emx_inp16       PROC    C PUBLIC, PORT:DWORD
                MOV     EDX, PORT
                MOV     EAX, 2
                CALL    MODE16
                RET
emx_inp16       ENDP

;
; void _outp16 (unsigned port, unsigned value)
;
                TALIGN  4
emx_outp16      PROC    C PUBLIC, PORT:DWORD, VALUE:DWORD
                MOV     EDX, PORT
                MOV     ECX, VALUE
                MOV     EAX, 3
                CALL    MODE16
                RET
emx_outp16      ENDP

;
; unsigned _inp32 (unsigned port)
;
                TALIGN  4
emx_inp32       PROC    C PUBLIC, PORT:DWORD
                MOV     EDX, PORT
                MOV     EAX, 4
                CALL    MODE16
                RET
emx_inp32       ENDP

;
; void _outp32 (unsigned port, unsigned value)
;
                TALIGN  4
emx_outp32      PROC    C PUBLIC, PORT:DWORD, VALUE:DWORD
                MOV     EDX, PORT
                MOV     ECX, VALUE
                MOV     EAX, 5
                CALL    MODE16
                RET
emx_outp32      ENDP


;
; void _inps8 (unsigned port, unsigned char *dst, unsigned count)
;
                TALIGN  4
emx_inps8       PROC    C PUBLIC USES EDI, PORT:DWORD, DST:PTR BYTE, COUNT:DWORD
                MOV     EDI, DST
                MOV     EDX, PORT
                MOV     ECX, COUNT
                MOV     EAX, 6
                CALL    MODE16
                RET
emx_inps8       ENDP

;
; void _outps8 (unsigned port, const unsigned char *src, unsigned count)
;
                TALIGN  4
emx_outps8      PROC    C PUBLIC USES ESI, PORT:DWORD, SRC:PTR BYTE, COUNT:DWORD
                MOV     ESI, SRC
                MOV     EDX, PORT
                MOV     ECX, COUNT
                MOV     EAX, 7
                CALL    MODE16
                RET
emx_outps8      ENDP

;
; void _outpt (const void *table);
;
                TALIGN  4
emx_outpt       PROC    C PUBLIC USES ESI, TABLE:PTR DWORD
                MOV     ESI, TABLE
                MOV     EAX, 8
                CALL    MODE16
                RET
emx_outpt       ENDP

;
; void _inps16 (unsigned port, unsigned short *dst, unsigned count);
;
                TALIGN  4
emx_inps16      PROC    C PUBLIC USES EDI, PORT:DWORD, DST:PTR BYTE, COUNT:DWORD
                MOV     EDI, DST
                MOV     EDX, PORT
                MOV     ECX, COUNT
                MOV     EAX, 9
                CALL    MODE16
                RET
emx_inps16      ENDP

;
; void _outps16 (unsigned port, const unsigned short *src, unsigned count)
;
                TALIGN  4
emx_outps16     PROC    C PUBLIC USES ESI, PORT:DWORD, SRC:PTR BYTE, COUNT:DWORD
                MOV     ESI, SRC
                MOV     EDX, PORT
                MOV     ECX, COUNT
                MOV     EAX, 10
                CALL    MODE16
                RET
emx_outps16     ENDP

;
; void _inps32 (unsigned port, unsigned long *dst, unsigned count);
;
                TALIGN  4
emx_inps32      PROC    C PUBLIC USES EDI, PORT:DWORD, DST:PTR BYTE, COUNT:DWORD
                MOV     EDI, DST
                MOV     EDX, PORT
                MOV     ECX, COUNT
                MOV     EAX, 11
                CALL    MODE16
                RET
emx_inps32      ENDP

;
; void _outps32 (unsigned port, const unsigned long *src, unsigned count)
;
                TALIGN  4
emx_outps32     PROC    C PUBLIC USES ESI, PORT:DWORD, SRC:PTR BYTE, COUNT:DWORD
                MOV     ESI, SRC
                MOV     EDX, PORT
                MOV     ECX, COUNT
                MOV     EAX, 12
                CALL    MODE16
                RET
emx_outps32     ENDP

;
; void _wait01 (unsigned port, unsigned mask)
;
                TALIGN  4
emx_wait01      PROC    C PUBLIC, PORT:DWORD, BITS:DWORD
                MOV     EDX, PORT
                MOV     ECX, BITS
                MOV     EAX, 13
                CALL    MODE16
                RET
emx_wait01      ENDP

;
; void _wait10 (unsigned port, unsigned mask)
;
                TALIGN  4
emx_wait10      PROC    C PUBLIC, PORT:DWORD, BITS:DWORD
                MOV     EDX, PORT
                MOV     ECX, BITS
                MOV     EAX, 14
                CALL    MODE16
                RET
emx_wait10      ENDP


;
; void _wait0 (unsigned port, unsigned mask)
;
                TALIGN  4
emx_wait0       PROC    C PUBLIC, PORT:DWORD, BITS:DWORD
                MOV     EDX, PORT
                MOV     ECX, BITS
                MOV     EAX, 15
                CALL    MODE16
                RET
emx_wait0       ENDP

;
; void _wait1 (unsigned port, unsigned mask)
;
                TALIGN  4
emx_wait1       PROC    C PUBLIC, PORT:DWORD, BITS:DWORD
                MOV     EDX, PORT
                MOV     ECX, BITS
                MOV     EAX, 16
                CALL    MODE16
                RET
emx_wait1       ENDP


;
; void _outps8dac (unsigned port, const unsigned char *src, unsigned count)
;
                TALIGN  4
emx_outps8dac   PROC    C PUBLIC USES ESI, PORT:DWORD, SRC:PTR BYTE, COUNT:DWORD
                MOV     ESI, SRC
                MOV     EDX, PORT
                MOV     ECX, COUNT
                MOV     EAX, 17
                CALL    MODE16
                RET
emx_outps8dac   ENDP

;
; Call 16-bit code
;
; In:   AL      Function number
;
                TALIGN  4
MODE16          PROC
                PUSH    EBP
                MOV     EBP, ESP
                SUB     ESP, 2 * 4
                MOV     [EBP-2*4], ECX
                MOV     [EBP-1*4], EDX
                MOV     ECX, ESP
                MOV     DX, SS
                AND     DL, 3
                OR      DL, 4
                CMP     CX, 1000H
                JAE     SHORT MODE16_1
                XOR     CX, CX
                MOV     BYTE PTR [ECX], 0       ; Stack probe
                XCHG    ESP, ECX
                TALIGN  4
MODE16_1:       PUSH    SS
                PUSH    ECX                     ; Save stack pointer
;
; Convert ESP to SS:SP
;
                MOV     ECX, ESP
                ROL     ECX, 16
                SHL     CX, 3
                OR      CL, DL
                PUSH    ECX                     ; Push new SS
                SHR     ECX, 16
                PUSH    ECX                     ; Push new ESP
                MOV     ECX, [EBP-2*4]
                MOV     EDX, [EBP-1*4]
                LSS     ESP, [ESP]              ; Switch to new stack
;
; Call 16-bit code
;
                JMP     FAR PTR TEXT16:CALL16
                TALIGN  4
MODE16_RET::    MOVZX   ESP, SP                 ; Don't trust...
                LSS     ESP, [ESP]              ; Get 32-bit stack pointer
                ADD     ESP, 2*4                ; Remove local variables
                POP     EBP                     ; Restore EBP
                RET                             ; Return to 32-bit code

MODE16          ENDP

TEXT32          ENDS


TEXT16          SEGMENT

                ASSUME  DS:NOTHING, ES:NOTHING

                TALIGN  4
CALL16_TAB      DWORD   FAR16 PTR iopl_inp8     ; 0
                DWORD   FAR16 PTR iopl_outp8    ; 1
                DWORD   FAR16 PTR iopl_inp16    ; 2
                DWORD   FAR16 PTR iopl_outp16   ; 3
                DWORD   FAR16 PTR iopl_inp32    ; 4
                DWORD   FAR16 PTR iopl_outp32   ; 5
                DWORD   FAR16 PTR iopl_inps8    ; 6
                DWORD   FAR16 PTR iopl_outps8   ; 7
                DWORD   FAR16 PTR iopl_outpt    ; 8
                DWORD   FAR16 PTR iopl_inps16   ; 9
                DWORD   FAR16 PTR iopl_outps16  ; 10
                DWORD   FAR16 PTR iopl_inps32   ; 11
                DWORD   FAR16 PTR iopl_outps32  ; 12
                DWORD   FAR16 PTR iopl_wait01   ; 13
                DWORD   FAR16 PTR iopl_wait10   ; 14
                DWORD   FAR16 PTR iopl_wait0    ; 15
                DWORD   FAR16 PTR iopl_wait1    ; 15
                DWORD   FAR16 PTR iopl_outps8dac; 16

;
; Entry point from 32-bit code
;
                TALIGN  4
CALL16          PROC    FAR
                CALL    CALL16_TAB[EAX*4]
                JMP     FAR32 PTR FLAT:MODE16_RET
                RET                             ; Avoid warning

CALL16          ENDP

TEXT16          ENDS

IOPL16          SEGMENT DWORD USE16 PUBLIC 'CODE'

;
; The upper 24 bits of EAX are zero on entry (function number!)
;
                TALIGN  4
iopl_inp8       PROC    FAR16 PUBLIC
                IN      AL, DX
                RET
iopl_inp8       ENDP

                TALIGN  4
iopl_outp8      PROC    FAR16 PUBLIC
                MOV     AL, CL
                OUT     DX, AL
                RET
iopl_outp8      ENDP

                TALIGN  4
iopl_inp16      PROC    FAR16 PUBLIC
                IN      AX, DX
                RET
iopl_inp16      ENDP

                TALIGN  4
iopl_outp16     PROC    FAR16 PUBLIC
                MOV     AX, CX
                OUT     DX, AX
                RET
iopl_outp16     ENDP

                TALIGN  4
iopl_inp32      PROC    FAR16 PUBLIC
                IN      EAX, DX
                RET
iopl_inp32      ENDP

                TALIGN  4
iopl_outp32     PROC    FAR16 PUBLIC
                MOV     EAX, ECX
                OUT     DX, EAX
                RET
iopl_outp32     ENDP

                TALIGN  4
iopl_inps8      PROC    FAR16 PUBLIC
                REP INS BYTE PTR [EDI], DX
; Note: EDI and ECX can be incorrect now due to a bug in the 386
                RET
iopl_inps8      ENDP

;
;
;
                TALIGN  4
iopl_outps8     PROC    FAR16 PUBLIC
                REP OUTS DX, BYTE PTR [ESI]
                RET
iopl_outps8     ENDP


;
;
;
                TALIGN  4
iopl_outpt      PROC    FAR16 PUBLIC
                TALIGN  4
OUTPT1:         MOV     ECX, [ESI+0]
                JECXZ   OUTPT9
                MOV     EDX, [ESI+4]
                MOV     EAX, [ESI+8]
                ADD     ESI, 12
                TEST    EAX, EAX
                JZ      SHORT OUTPT_B
                DEC     EAX
                JZ      SHORT OUTPT_W
                DEC     EAX
                JNZ     SHORT OUTPT9
                REP OUTS DX, DWORD PTR [ESI]
                JMP     SHORT OUTPT1

                TALIGN  4
OUTPT_B:        REP OUTS DX, BYTE PTR [ESI]
                JMP     SHORT OUTPT1

                TALIGN  4
OUTPT_W:        REP OUTS DX, WORD PTR [ESI]
                JMP     SHORT OUTPT1

                TALIGN  4
OUTPT9:         RET
iopl_outpt      ENDP

;
;
;
                TALIGN  4
iopl_inps16     PROC    FAR16 PUBLIC
                REP INS WORD PTR [EDI], DX
; Note: EDI and ECX can be incorrect now due to a bug in the 386
                RET
iopl_inps16     ENDP

;
;
;
                TALIGN  4
iopl_outps16    PROC    FAR16 PUBLIC
                REP OUTS DX, WORD PTR [ESI]
                RET
iopl_outps16    ENDP


;
;
;
                TALIGN  4
iopl_inps32     PROC    FAR16 PUBLIC
                REP INS DWORD PTR [EDI], DX
; Note: EDI and ECX can be incorrect now due to a bug in the 386
                RET
iopl_inps32     ENDP

;
;
;
                TALIGN  4
iopl_outps32    PROC    FAR16 PUBLIC
                REP OUTS DX, DWORD PTR [ESI]
                RET
iopl_outps32    ENDP


;
;
;
                TALIGN  4
iopl_wait01     PROC    FAR16 PUBLIC
WAIT01_1:       IN      AL, DX
                TEST    AL, CL
                JNZ     WAIT01_1
                TALIGN  4
WAIT01_2:       IN      AL, DX
                TEST    AL, CL
                JZ      WAIT01_2
                RET
iopl_wait01     ENDP

;
;
;
                TALIGN  4
iopl_wait10     PROC    FAR16 PUBLIC
WAIT10_1:       IN      AL, DX
                TEST    AL, CL
                JZ      WAIT10_1
                TALIGN  4
WAIT10_2:       IN      AL, DX
                TEST    AL, CL
                JNZ     WAIT10_2
                RET
iopl_wait10     ENDP

;
;
;
                TALIGN  4
iopl_wait0      PROC    FAR16 PUBLIC
WAIT0_1:        IN      AL, DX
                TEST    AL, CL
                JNZ     WAIT0_1
                RET
iopl_wait0      ENDP

;
;
;
                TALIGN  4
iopl_wait1      PROC    FAR16 PUBLIC
WAIT1_1:        IN      AL, DX
                TEST    AL, CL
                JZ      WAIT1_1
                RET
iopl_wait1      ENDP

;
;
;
                TALIGN  4
iopl_outps8dac  PROC    FAR16 PUBLIC
                JCXZ    OUTPS8DAC_9
                TALIGN  4                       ; Avoid timing surprises
OUTPS8DAC_1:    MOV     AL, [ESI]
                INC     ESI
                NOP
                OUT     DX, AL
                LOOP    OUTPS8DAC_1
OUTPS8DAC_9:    RET
iopl_outps8dac  ENDP


IOPL16          ENDS

                END
