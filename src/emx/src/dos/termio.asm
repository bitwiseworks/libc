;
; TERMIO.ASM -- General terminal interface
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

__TERMIO        =       1
                INCLUDE EMX.INC
                INCLUDE TERMIO.INC
                INCLUDE SIGNAL.INC
                INCLUDE PROCESS.INC
                INCLUDE ERRORS.INC

                PUBLIC  STDIN_TERMIO, STDIN_FL
                PUBLIC  TERMIO_INIT, TERMIO_READ, STDIN_AVAIL, KBD_FLUSH
                PUBLIC  POLL_KEYBOARD, INIT_TERMIO

SV_DATA         SEGMENT

CHAR_SIG        =       0100H
CHAR_NO         =       0101H
CHAR_TIME       =       0102H

STDIN_TERMIO    TERMIO  <>
STDIN_ESCAPE    DB      FALSE
                DALIGN  4
STDIN_FL        DD      0
STDIN_BUFFERED  DD      0
STDIN_BUF_PTR   DD      ?
STDIN_BUFFER    DB      256 DUP (?)

EXT_KBD         DB      00H                     ; Either 00H or 10H

TR_OUT_TMP_CHAR DB      ?

KBD_PTR_IN      DW      KBD_BUF
KBD_PTR_OUT     DW      KBD_BUF
KBD_BUF         DB      64 DUP (?)
KBD_BUF_END     =       THIS BYTE

SV_DATA         ENDS

SV_CODE         SEGMENT

                ASSUME  CS:SV_CODE, DS:NOTHING


;
; Set default values for a TERMIO structure
;
; In:   BX      Pointer to TERMIO structure
;
; The default values for c_iflag, c_lflag and c_cc differ from Unix
;
                TALIGN  2
                ASSUME  BX:PTR TERMIO
TERMIO_INIT     PROC    NEAR
                MOV     [BX].C_IFLAG, BRKINT OR ICRNL OR IXON OR IXANY
                MOV     [BX].C_OFLAG, 0
                MOV     [BX].C_CFLAG, B9600 OR CS8 OR CREAD OR HUPCL
                MOV     [BX].C_LFLAG, ISIG OR ICANON OR IECHO OR ECHOE OR ECHOK OR IDEFAULT
                MOV     [BX].C_LINE, 0
                MOV     [BX].C_CC[VINTR], 03H           ; Ctrl-C
                MOV     [BX].C_CC[VQUIT], 1CH           ; Ctrl-\
                MOV     [BX].C_CC[VERASE], 08H          ; Ctrl-H
                MOV     [BX].C_CC[VKILL], 15H           ; Ctrl-U
                MOV     [BX].C_CC[VEOF], 04H            ; Ctrl-D
                MOV     [BX].C_CC[VEOL], 0              ; Disabled
                MOV     [BX].C_CC[VMIN], 6
                MOV     [BX].C_CC[VTIME], 1
                RET
                ASSUME  BX:NOTHING
TERMIO_INIT     ENDP

;
; Query `struct termio'
;
; In:   EAX     File handle
;       ES:EBX  Destination address, points to `struct termio'
;
; Out:  EAX     Return code (0 on success, -1 on error)
;       ECX     errno (0 on success)

                TALIGN  4
                ASSUME  DS:SV_DATA
                ASSUME  EBX:NEAR32 PTR TERMIO
TERMIO_GET      PROC    NEAR
                CMP     EAX, 0                  ; stdin
                JNE     TG_EBADF
                IRP     F, <C_IFLAG, C_OFLAG, C_CFLAG, C_LFLAG>
                MOV     EAX, STDIN_TERMIO.F
                MOV     ES:[EBX].F, EAX
                ENDM
                MOV     ES:[EBX].C_LINE, 0
                MOV     ESI, 0
TG_1:           MOV     AL, STDIN_TERMIO.C_CC[SI]
                MOV     ES:[EBX].C_CC[ESI], AL
                INC     SI
                CMP     SI, NCC
                JB      SHORT TG_1
                XOR     EAX, EAX
                XOR     ECX, ECX
                RET

TG_EBADF:       MOV     EAX, -1
                MOV     ECX, EBADF
                RET
TERMIO_GET      ENDP


;
; Set terminal attributes from `struct termio'
;
; In:   EAX     File handle
;       ES:EBX  Source address, points to `struct termio'
;
; Out:  EAX     Return code (0 on success, -1 on error)
;       ECX     errno (0 on success)
;
                TALIGN  4
                ASSUME  DS:SV_DATA
                ASSUME  EBX:NEAR32 PTR TERMIO
TERMIO_SET      PROC    NEAR
                CMP     EAX, 0                ; stdin?
                JNE     SHORT TS_EBADF
                IRP     F, <C_IFLAG, C_OFLAG, C_CFLAG, C_LFLAG>
                MOV     EAX, ES:[EBX].F
                MOV     STDIN_TERMIO.F, EAX
                ENDM
                MOV     ESI, 0
TS_1:           MOV     AL, ES:[EBX].C_CC[ESI]
                MOV     STDIN_TERMIO.C_CC[SI], AL
                INC     SI
                CMP     SI, NCC
                JB      SHORT TS_1
                MOV     STDIN_TERMIO.C_CC[VSUSP], 0     ; Disabled
                MOV     STDIN_TERMIO.C_CC[VSTOP], 13H   ; Ctrl-S
                MOV     STDIN_TERMIO.C_CC[VSTART], 11H  ; Ctrl-Q
                XOR     EAX, EAX
                XOR     ECX, ECX
                RET

TS_EBADF:       MOV     EAX, -1
                MOV     ECX, EBADF
                RET
TERMIO_SET      ENDP


;
; Flush input queue for termio
;
; In:   EAX     File handle
;
; Out:  EAX     Return code (0 on success, -1 on error)
;       ECX     errno (0 on success)
;
                TALIGN  4
                ASSUME  DS:SV_DATA
TERMIO_FLUSH    PROC    NEAR
                CMP     EAX, 0                  ; stdin
                JNE     SHORT TF_EBADF
                CALL    KBD_FLUSH
                XOR     EAX, EAX
                XOR     ECX, ECX
                RET

TF_EBADF:       MOV     EAX, -1
                MOV     ECX, EBADF
                RET
TERMIO_FLUSH    ENDP


;
; Query `struct termios'
;
; In:   EAX     File handle
;       ES:EBX  Destination address, points to `struct termios'
;
; Out:  EAX     Return code (0 on success, -1 on error)
;       ECX     errno (0 on success)
;
                TALIGN  4
                ASSUME  DS:SV_DATA
                ASSUME  EBX:NEAR32 PTR TERMIOS
TERMIOS_GET     PROC    NEAR
                CMP     EAX, 0                  ; stdin
                JNE     TSG_ENOTTY
                IRP     F, <C_IFLAG, C_OFLAG, C_CFLAG>
                MOV     EAX, STDIN_TERMIO.F
                MOV     ES:[EBX].F, EAX
                ENDM
                MOV     EAX, STDIN_TERMIO.C_LFLAG
                AND     EAX, NOT IDEFAULT
                MOV     ES:[EBX].C_LFLAG, EAX
                MOV     ESI, 0
TSG_1:          MOV     AL, STDIN_TERMIO.C_CC[SI]
                MOV     ES:[EBX].C_CC[ESI], AL
                INC     SI
                CMP     SI, NCCS
                JB      SHORT TSG_1
                XOR     EAX, EAX
                XOR     ECX, ECX
                RET

TSG_ENOTTY:     MOV     EAX, -1
                MOV     ECX, ENOTTY
                RET
TERMIOS_GET     ENDP

;
; Set terminal attributes from `struct termios'
;
; In:   EAX     File handle
;       ES:EBX  Source address, points to `struct termios'
;
; Out:  EAX     Return code (0 on success, -1 on error)
;       ECX     errno (0 on success)
;
                TALIGN  4
                ASSUME  DS:SV_DATA
                ASSUME  EBX:NEAR32 PTR TERMIOS
TERMIOS_SET     PROC    NEAR
                CMP     EAX, 0                ; stdin?
                JNE     SHORT TCS_ENOTTY
                IRP     F, <C_IFLAG, C_OFLAG, C_CFLAG>
                MOV     EAX, ES:[EBX].F
                MOV     STDIN_TERMIO.F, EAX
                ENDM
                MOV     EAX, ES:[EBX].C_LFLAG
                AND     EAX, NOT IDEFAULT
                MOV     STDIN_TERMIO.C_LFLAG, EAX
                MOV     ESI, 0
TCS_1:          MOV     AL, ES:[EBX].C_CC[ESI]
                MOV     STDIN_TERMIO.C_CC[SI], AL
                INC     SI
                CMP     SI, NCCS
                JB      SHORT TCS_1
                XOR     EAX, EAX
                XOR     ECX, ECX
                RET

TCS_ENOTTY:     MOV     EAX, -1
                MOV     ECX, ENOTTY
                RET
TERMIOS_SET     ENDP


;
; Flush input queue for termios
;
; In:   EAX     File handle
;
; Out:  EAX     Return code (0 on success, -1 on error)
;       ECX     errno (0 on success)
;
                TALIGN  4
                ASSUME  DS:SV_DATA
TERMIOS_FLUSH   PROC    NEAR
                CMP     EAX, 0                  ; stdin
                JNE     SHORT TF_ENOTTY
                CALL    KBD_FLUSH
                XOR     EAX, EAX
                XOR     ECX, ECX
                RET

TF_ENOTTY:      MOV     EAX, -1
                MOV     ECX, ENOTTY
                RET
TERMIOS_FLUSH   ENDP


;
; Get and preprocess a character for termio input
;
; In:   CX=FALSE        Return AX=CHAR_NO if no character available
;       CX!=FALSE       Wait until signal or character available
; Out:  AX=CHAR_SIG     Interrupted by signal
;       AX=CHAR_NO      No character available (for CX!=FALSE)
;       AX=CHAR_TIME    Time-out
;       AL              Character (AH=0)
;
                TALIGN  2
                ASSUME  DS:SV_DATA
TR_GET          PROC    NEAR
TR_GET_0:       PUSH    CX
                CALL    READ_KEYBOARD
                POP     CX
                OR      AH, AH
                JNZ     SHORT TR_GET_RET
;
; Strip bit 7 if ISTRIP is set
;
                TEST    STDIN_TERMIO.C_IFLAG, ISTRIP
                JZ      SHORT TR_GET_1
                AND     AL, 7FH
TR_GET_1:
;
; Ignore CR characters if IGNCR is set
;
                TEST    STDIN_TERMIO.C_IFLAG, IGNCR
                JZ      SHORT TR_GET_2
                CMP     AL, CR
                JE      SHORT TR_GET_0
TR_GET_2:
;
; Translate LF to CR if INLCR is set (don't translate the CR back to LF if
; ICRNL is also set; IGNCR is also ignored for a CR created by this
; translation)
;
                TEST    STDIN_TERMIO.C_IFLAG, INLCR
                JZ      SHORT TR_GET_3
                CMP     AL, LF
                JNE     SHORT TR_GET_3
                MOV     AL, CR
                JMP     SHORT TR_GET_4
TR_GET_3:
;
; Translate CR to LF if ICRNL is set
;
                TEST    STDIN_TERMIO.C_IFLAG, ICRNL
                JZ      SHORT TR_GET_4
                CMP     AL, CR
                JNE     SHORT TR_GET_4
                MOV     AL, LF
TR_GET_4:
;
; Translate upper-case letters to lower case if IUCLC is set
;
                TEST    STDIN_TERMIO.C_IFLAG, IUCLC
                JZ      SHORT TR_GET_5
                CMP     AL, "A"
                JB      SHORT TR_GET_5
                CMP     AL, "Z"
                JA      SHORT TR_GET_5
                ADD     AL, "a" - "A"
TR_GET_5:
;
; No further input transformations are available
;
TR_GET_RET:     RET
TR_GET          ENDP


;
; Read keyboard
;
; In:   CX=FALSE        Return AX=CHAR_NO if no character available
;       CX!=FALSE       Wait until signal, time-out or character available 
; Out:  AX=CHAR_SIG     Interrupted by signal
;       AX=CHAR_NO      No character available (for CX!=FALSE)
;       AX=CHAR_TIME    Time-out
;       AL              Character (AH=0)
;
                TALIGN  4
                ASSUME  DS:SV_DATA
READ_KEYBOARD   PROC    NEAR
RK_LOOP:        MOV     AX, 7F16H               ; Poll keyboard
                INT     21H
                MOV     BX, PROCESS_PTR
                ASSUME  BX:PTR PROCESS
                MOV     AX, CHAR_SIG
                CMP     [BX].P_SIG_PENDING, 0
                JNE     SHORT RK_RET
                TEST    [BX].P_FLAGS, PF_TERMIO_TIME
                MOV     AX, CHAR_TIME
                JNZ     SHORT RK_RET
                ASSUME  BX:NOTHING
                CLI
                MOV     BX, KBD_PTR_OUT
                CMP     BX, KBD_PTR_IN
                JE      SHORT RK_NONE
                MOV     AL, [BX]
                INC     BX
                CMP     BX, OFFSET KBD_BUF_END
                JNE     SHORT RK_OK
                LEA     BX, KBD_BUF
RK_OK:          MOV     KBD_PTR_OUT, BX
                STI
                MOV     AH, 0
                JMP     SHORT RK_RET

RK_NONE:        STI
                CMP     CX, FALSE
                JNE     SHORT RK_LOOP
                MOV     AX, CHAR_NO
RK_RET:         RET
READ_KEYBOARD   ENDP

;
; Return the number of characters in the keyboard buffer
;
                TALIGN  4
STDIN_AVAIL     PROC    NEAR
                MOV     AX, 7F16H               ; Poll keyboard
                INT     21H
                CLI
                MOV     AX, KBD_PTR_IN
                SUB     AX, KBD_PTR_OUT
                JAE     SHORT SA_1
                ADD     AX, SIZE KBD_BUF
SA_1:           STI
                MOVZX   EAX, AX
                RET
STDIN_AVAIL     ENDP

;
; Unix-like read() for terminal
;
; In:   ES:EDI  Destination address
;       ECX     Number of bytes
;
; Out:  CY      Error
;       EAX     Number of bytes read (NC)
;       EAX     errno (CY)
;
; Currently implemented for the keyboard only
;
TR_COUNT        EQU     (DWORD PTR [BP-1*4])
TR_RESULT       EQU     (DWORD PTR [BP-2*4])
TR_TIMER        EQU     (DWORD PTR [BP-3*4])
TR_TIMER_FLAG   EQU     (DWORD PTR [BP-4*4])
TR_MIN          EQU     (DWORD PTR [BP-5*4])
TR_TMP          EQU     (DWORD PTR [BP-6*4])

                TALIGN  2
                ASSUME  DS:SV_DATA
TERMIO_READ     PROC    NEAR
                PUSH    BP
                MOV     BP, SP
                SUB     SP, 6*4
                MOV     TR_COUNT, ECX
                MOV     TR_RESULT, 0
                MOV     TR_TIMER_FLAG, FALSE
                MOV     BX, PROCESS_PTR
                AND     (PROCESS PTR [BX]).P_FLAGS, NOT PF_TERMIO_TIME
                TEST    STDIN_TERMIO.C_LFLAG, ICANON
                JNZ     TR_CANON
                MOVZX   EAX, STDIN_TERMIO.C_CC[VMIN]
                MOV     TR_MIN, EAX
;
; Check for O_NDELAY mode
;
                TEST    STDIN_FL, O_NDELAY      ; O_NDELAY set?
                JZ      SHORT TR_RAW_DELAY      ; No  -> skip
;
; Rule out all cases where we have to wait
;
                MOVZX   EAX, STDIN_TERMIO.C_CC[VMIN]
                OR      EAX, EAX
                JNZ     SHORT TR_RAW_COUNT      ; Check count
                CMP     STDIN_TERMIO.C_CC[VTIME], 0
                JE      SHORT TR_RAW_DELAY      ; Both zero -> won't wait
                INC     EAX                     ; At least one character
TR_RAW_COUNT:   PUSH    EAX
                CALL    STDIN_AVAIL             ; Available characters
                POP     ECX
                CMP     EAX, ECX                ; Enough characters for VMIN?
                JAE     TR_RAW_DELAY            ; Yes ->
                CMP     EAX, TR_COUNT           ; Enough characters for COUNT?
                JAE     TR_RAW_DELAY            ; Yes ->
                MOV     EAX, EAGAIN
                STC
                JMP     TR_RET

TR_RAW_DELAY:
;
; Start timer if VMIN = 0 and VTIME > 0
;
                CMP     STDIN_TERMIO.C_CC[VMIN], 0
                JNE     SHORT TR_RAW_LOOP
                CMP     STDIN_TERMIO.C_CC[VTIME], 0
                JE      SHORT TR_RAW_LOOP
                CALL    TR_START_TIMER
                MOV     TR_MIN, 1
TR_RAW_LOOP:    CMP     TR_COUNT, 0
                JE      TR_OK
                MOV     CX, NOT FALSE           ; Wait
                MOV     EAX, TR_RESULT
                CMP     EAX, TR_MIN
                JB      SHORT TR_RAW_GET
                MOV     CX, FALSE
TR_RAW_GET:     CALL    TR_GET
                CMP     AX, CHAR_NO
                JE      TR_OK
                CMP     AX, CHAR_TIME
                JE      TR_OK
                CMP     AX, CHAR_SIG
                JE      SHORT TR_SIG
                MOV     ES:[EDI], AL            ;...ext
                INC     EDI
                INC     TR_RESULT
                DEC     TR_COUNT
                CALL    TR_ECHO
                CMP     TR_COUNT, 0
                JE      TR_OK
;
; Restart timer for VMIN > 0 and VTIME > 0
;
                CMP     STDIN_TERMIO.C_CC[VMIN], 0
                JE      SHORT TR_RAW_LOOP
                CMP     STDIN_TERMIO.C_CC[VTIME], 0
                JE      SHORT TR_RAW_LOOP
;
; Stop timer if TR_TIMER_FLAG
;
                CMP     TR_TIMER_FLAG, FALSE
                JE      SHORT TR_RAW_INTER_1
                CALL    TR_STOP_TIMER
TR_RAW_INTER_1: MOV     BX, PROCESS_PTR
                AND     (PROCESS PTR [BX]).P_FLAGS, NOT PF_TERMIO_TIME
                CALL    TR_START_TIMER
                JMP     TR_RAW_LOOP

TR_SIG:         CMP     TR_TIMER_FLAG, FALSE
                JE      SHORT TR_SIG_1
                CALL    TR_STOP_TIMER
TR_SIG_1:       MOV     EAX, EINTR
                STC
                JMP     TR_RET

TR_CANON:       CMP     STDIN_BUFFERED, 0
                JE      SHORT TR_CANON_10
                MOV     ECX, TR_COUNT
                CMP     ECX, STDIN_BUFFERED
                JBE     SHORT TR_CANON_01
                MOV     ECX, STDIN_BUFFERED
TR_CANON_01:    SUB     TR_COUNT, ECX
                SUB     STDIN_BUFFERED, ECX
                ADD     TR_RESULT, ECX
                MOV     ESI, STDIN_BUF_PTR
                ADD     STDIN_BUF_PTR, ECX
                CLD
                REP     MOVS BYTE PTR ES:[EDI], BYTE PTR DS:[ESI]
                JMP     TR_OK

TR_CANON_10:    CMP     TR_COUNT, 0
                JZ      TR_OK
;
; Read a line
;
                MOV     STDIN_BUFFERED, 0
                LEA     ESI, STDIN_BUFFER
                MOV     STDIN_BUF_PTR, ESI
                MOV     STDIN_ESCAPE, FALSE
TR_CANON_LOOP:  MOV     CX, NOT FALSE
                CALL    TR_GET
                CMP     AX, CHAR_SIG
                JE      TR_SIG
                OR      AL, AL
                JZ      TR_CANON_EXT
                CMP     AL, LF
                JE      TR_CANON_LF
                CMP     AL, STDIN_TERMIO.C_CC[VEOL]
                JE      TR_CANON_LF
                CMP     AL, STDIN_TERMIO.C_CC[VEOF]
                JE      TR_CANON_EOF
                CMP     AL, STDIN_TERMIO.C_CC[VERASE]
                JE      SHORT TR_CANON_ERASE
                CMP     AL, STDIN_TERMIO.C_CC[VKILL]
                JE      SHORT TR_CANON_KILL
                CMP     STDIN_ESCAPE, FALSE
                JNE     TR_CANON_QUOTE
                CMP     AL, "\"
                JE      TR_CANON_ESC
TR_CANON_PUT:   MOV     STDIN_ESCAPE, FALSE
                CALL    TR_PUT
                CALL    TR_ECHO
                JMP     SHORT TR_CANON_LOOP

TR_CANON_ERASE: CMP     STDIN_ESCAPE, FALSE
                JNE     SHORT TR_CANON_ESC_PUT
                CMP     STDIN_BUFFERED, 0
                JE      TR_CANON_LOOP
                CALL    TR_BACK
                JMP     TR_CANON_LOOP

TR_CANON_ESC_PUT:
                PUSH    AX
                MOV     AL, 08H
                CALL    TR_OUT                  ; Delete the backslash
                POP     AX
                JMP     SHORT TR_CANON_PUT

TR_CANON_KILL:  CMP     STDIN_ESCAPE, FALSE
                JNE     SHORT TR_CANON_ESC_PUT
                MOV     STDIN_BUFFERED, 0
                LEA     ESI, STDIN_BUFFER
                TEST    STDIN_TERMIO.C_LFLAG, ECHOK
                JZ      TR_CANON_LOOP
                MOV     AL, CR
                CALL    TR_OUT
                MOV     AL, LF
                CALL    TR_OUT
                JMP     TR_CANON_LOOP

TR_CANON_EXT:   MOV     CX, NOT FALSE
                CALL    TR_GET
                CMP     AX, CHAR_SIG
                JE      TR_SIG
                JMP     TR_CANON_LOOP

TR_CANON_EOF:   CMP     STDIN_ESCAPE, FALSE
                JNE     SHORT TR_CANON_ESC_PUT
                CMP     STDIN_BUFFERED, 0
                JE      SHORT TR_OK
                JMP     TR_CANON

TR_CANON_LF:    CALL    TR_PUT
                CALL    TR_ECHO
                JMP     TR_CANON

TR_CANON_ESC:   MOV     STDIN_ESCAPE, NOT FALSE
                CALL    TR_ECHO
                JMP     TR_CANON_LOOP

TR_CANON_QUOTE: PUSH    EAX
                MOV     AL, "\"
                CALL    TR_PUT
                POP     EAX
                JMP     TR_CANON_PUT

TR_OK:          CMP     TR_TIMER_FLAG, FALSE
                JE      SHORT TR_OK_1
                CALL    TR_STOP_TIMER
TR_OK_1:        MOV     EAX, TR_RESULT
                CLC
TR_RET:         MOV     SP, BP
                NOP                             ; Try to fix Cyrix 486DLC bug
                POP     BP
                RET
TERMIO_READ     ENDP

;
;
;
                TALIGN  2
                ASSUME  DS:SV_DATA
TR_START_TIMER  PROC    NEAR
                MOVZX   EAX, STDIN_TERMIO.C_CC[VTIME]
                MOV     ECX, 91
                MUL     ECX                     ; Multiply by 1.82
                ADD     EAX, 49
                MOV     ECX, 50
                DIV     ECX
                MOV     BX, PROCESS_PTR
                MOV     DX, TT_TERMIO_TIME
                CALL    SET_TIMER
                MOV     TR_TIMER_FLAG, NOT FALSE
                RET
TR_START_TIMER  ENDP

;
;
;
                TALIGN  2
                ASSUME  DS:SV_DATA
TR_STOP_TIMER   PROC    NEAR
                MOV     BX, PROCESS_PTR
                MOV     DX, TT_TERMIO_TIME
                XOR     EAX, EAX
                CALL    SET_TIMER
                MOV     TR_TIMER_FLAG, FALSE
                RET
TR_STOP_TIMER   ENDP

;
;
;
                TALIGN  2
                ASSUME  DS:SV_DATA
TR_PUT          PROC    NEAR
                CMP     STDIN_BUFFERED, SIZE STDIN_BUFFER
                JAE     SHORT TR_PUT_OV
                MOV     [ESI], AL
                INC     ESI
                INC     STDIN_BUFFERED
TR_PUT_OV:      RET
TR_PUT          ENDP

;
;
;
                TALIGN  2
                ASSUME  DS:SV_DATA
TR_ECHO         PROC    NEAR
                TEST    STDIN_TERMIO.C_LFLAG, IECHO
                JZ      SHORT TR_ECHO_RET
                CMP     AL, LF
                JE      SHORT TR_ECHO_LF
        if 1
                JMP     SHORT TR_ECHO_1
        else
                CMP     AL, 20H
                JAE     SHORT TR_ECHO_1
                PUSH    EAX
                MOV     AL, "^"
                CALL    TR_OUT
                POP     EAX
                PUSH    EAX
                ADD     AL, 40H
                CALL    TR_OUT
                POP     EAX
                RET
        endif

TR_ECHO_LF:     MOV     AL, CR
                CALL    TR_OUT
                MOV     AL, LF
TR_ECHO_1:      CALL    TR_OUT
TR_ECHO_RET:    RET
TR_ECHO         ENDP

;
;
;
                TALIGN  2
                ASSUME  DS:SV_DATA
TR_BACK         PROC    NEAR
                DEC     STDIN_BUFFERED
                DEC     ESI
                MOV     AL, 08H
                CALL    TR_OUT
                MOV     AL, " "
                CALL    TR_OUT
                MOV     AL, 08H
                CALL    TR_OUT
                RET
TR_BACK         ENDP

;
; Note: Ctrl-C!!!
;
; Don't use function 02H as that would require calling MAP_HANDLE.
;
                TALIGN  4
TR_OUT          PROC    NEAR
                PUSH    EAX
                PUSH    EBX
                PUSH    ECX
                PUSH    EDX
                MOV     TR_OUT_TMP_CHAR, AL
                LEA     EDX, TR_OUT_TMP_CHAR
                MOV     ECX, 1
                MOV     EBX, 1                  ; Standard output
                MOV     AH, 40H
                INT     21H
                POP     EDX
                POP     ECX
                POP     EBX
                POP     EAX
                RET
TR_OUT          ENDP


                TALIGN  4
KBD_FLUSH       PROC    NEAR
KF_1:           MOV     AH, 01H
                ADD     AH, EXT_KBD
                INT     16H
                JZ      SHORT KF_2
                MOV     AH, 00H
                ADD     AH, EXT_KBD
                INT     16H
                JMP     SHORT KF_1
                TALIGN  4
KF_2:           CLI
                LEA     AX, KBD_BUF
                MOV     KBD_PTR_IN, AX
                MOV     KBD_PTR_OUT, AX
                STI
                MOV     STDIN_BUFFERED, 0
                RET
KBD_FLUSH       ENDP

SV_CODE         ENDS


INIT_CODE       SEGMENT

                ASSUME  CS:INIT_CODE, DS:NOTHING

;
; Get keyboard type
;
                ASSUME  DS:SV_DATA
                TALIGN  4
INIT_TERMIO     PROC    NEAR
                MOV     EXT_KBD, 00H
                MOV     AX, 40H
                MOV     ES, AX
                TEST    BYTE PTR ES:[96H], 10H  ; Extended keyboard?
                JZ      SHORT IT_1
                MOV     EXT_KBD, 10H
IT_1:           RET
INIT_TERMIO     ENDP

;
; Read keyboard
;
; If the keyboard buffer is full, it will be cleared
;
                ASSUME  DS:SV_DATA
                TALIGN  4
POLL_KEYBOARD   PROC    NEAR
PK_LOOP:        MOV     AH, 01H
                ADD     AH, EXT_KBD
                INT     16H
                JZ      PK_RET
                MOV     AH, 00H
                ADD     AH, EXT_KBD
                INT     16H
                OR      AX, AX                  ; Ctrl-Break?
                JZ      SHORT PK_LOOP           ; Yes -> repeat
                OR      AH, AH                  ; Alt+numeric keypad?
                JZ      SHORT PK_NORMAL         ; Yes -> no extended code
                OR      AL, AL                  ; Extended code?
                JZ      SHORT PK_EXT            ; Yes ->
                CMP     AL, 0E0H                ; New extended code?
                JE      SHORT PK_EXT            ; Yes ->
PK_NORMAL:      TEST    STDIN_TERMIO.C_IFLAG, IDELETE
                JZ      SHORT PK_1
                CMP     AH, 0EH                 ; Backspace?
                JNE     SHORT PK_1              ; No  -> skip
                MOV     AH, AL                  ; Save character code
                MOV     AL, 08H                 ; BS
                CMP     AH, 7FH                 ; DEL?
                JE      SHORT PK_1              ; Yes -> convert to BS
                MOV     AL, 7FH                 ; DEL
                CMP     AH, 08H                 ; BS?
                JE      SHORT PK_1              ; Yes -> convert to DEL
                MOV     AL, AH                  ; Restore character code
PK_1:           TEST    STDIN_TERMIO.C_LFLAG, ISIG
                JZ      SHORT PK_PUT
                MOVZX   EBX, PROCESS_PTR
                ASSUME  EBX:NEAR32 PTR PROCESS
                CMP     BX, NO_PROCESS
                JE      SHORT PK_PUT
                CMP     AL, STDIN_TERMIO.C_CC[VINTR]
                MOV     ECX, SIGINT
                JE      SHORT PK_SIG
                CMP     AL, STDIN_TERMIO.C_CC[VQUIT]
                MOV     ECX, SIGQUIT
                JE      SHORT PK_SIG
PK_PUT:         CALL    KBD_PUT
                JMP     SHORT PK_LOOP

                TALIGN  4
PK_EXT:         XOR     AL, AL                  ; Prefix
                CALL    KBD_PUT
                MOV     AL, AH                  ; Scan code
                CALL    KBD_PUT
                JMP     PK_LOOP

                TALIGN  4
PK_SIG:         CMP     [EBX].P_SIG_HANDLERS[4*ECX], SIG_IGN
                JE      PK_LOOP
                BTS     (PROCESS PTR [BX]).P_SIG_PENDING, ECX
                JMP     PK_LOOP

                TALIGN  4
PK_RET:         RET
                ASSUME  EBX:NOTHING
POLL_KEYBOARD   ENDP


                TALIGN  4
KBD_PUT         PROC    NEAR
                MOV     BX, KBD_PTR_IN
                MOV     [BX], AL
                INC     BX
                CMP     BX, OFFSET KBD_BUF_END
                JNE     SHORT KP_1
                LEA     BX, KBD_BUF
KP_1:           MOV     KBD_PTR_IN, BX
                RET
KBD_PUT         ENDP


INIT_CODE       ENDS

                END
