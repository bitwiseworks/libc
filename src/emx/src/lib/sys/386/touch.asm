;
; _touch routine borrowed from EMXDLL.ASM:
;
; Copyright (c) 1992-1996 by Eberhard Mattes
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
; As special exception, emx.dll can be distributed without source code
; unless it has been changed.  If you modify emx.dll, this exception
; no longer applies and you must remove this paragraph from all source
; files for emx.dll.
;

                .386

; ----------------------------------------------------------------------------
; Macros
; ----------------------------------------------------------------------------

TALIGN          MACRO   VAL:REQ
                ALIGN   VAL
                ENDM

; ----------------------------------------------------------------------------
; The code segment
; ----------------------------------------------------------------------------

TEXT32          SEGMENT

                ASSUME  DS:FLAT, ES:FLAT

                PUBLIC  ___libc_touch

;
; Touch each page in a range of addresses
;
; void __libc_touch (void *base, ULONG count);
;
; In: BASE      Start address
;     COUNT     Number of bytes
;
; This is required because DosRead seems not to be reentrant enough --
; if it's used to read data into a page to be loaded from the .EXE file
; (dumped heap), it's recursively called by the guard pageexception handler.
; This call seems to disturb the first call, which will return a strange
; `error code' (ESP plus some constant).
;

BASE            EQU     (DWORD PTR [EBP+2*4])
COUNT           EQU     (DWORD PTR [EBP+3*4])
                TALIGN  4
___libc_touch   PROC
                PUSH    EBP                     ; Make stack frame
                MOV     EBP, ESP
                PUSH    EBX                     ; Save register
                MOV     ECX, COUNT
                JECXZ   TOUCH_RET
                MOV     EBX, BASE
                MOV     AL, [EBX]               ; Touch first page
                MOV     EAX, EBX                ; Save start address
                OR      EBX, 0FFFH              ; Move pointer to
                INC     EBX                     ; start of next page
                SUB     EBX, EAX                ; Number of bytes skipped
                SUB     ECX, EBX                ; Adjust counter
                JBE     SHORT TOUCH_RET         ; Done -> return
                ADD     EBX, EAX                ; Recompute pointer
                TALIGN  4
TOUCH_1:        MOV     AL, [EBX]               ; Touch a page
                ADD     EBX, 1000H              ; Move pointer to next page
                SUB     ECX, 1000H              ; Adjust counter
                JA      SHORT TOUCH_1           ; More -> repeat
TOUCH_RET:      POP     EBX                     ; Restore register
                POP     EBP                     ; Remove stack frame
                RET                             ; Done
___libc_touch   ENDP

TEXT32          ENDS

                END
