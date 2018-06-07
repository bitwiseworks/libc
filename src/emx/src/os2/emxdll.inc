;
; EMXDLL.INC
;
; Copyright (c) 1992-1995 by Eberhard Mattes
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

                OPTION  PROC:PRIVATE

; ----------------------------------------------------------------------------
; Segments
; ----------------------------------------------------------------------------

TEXT32          SEGMENT DWORD FLAT PUBLIC 'CODE'
TEXT32          ENDS

TEXT16          SEGMENT DWORD USE16 PUBLIC 'CODE'
TEXT16          ENDS

BSS16           SEGMENT DWORD USE16 PUBLIC 'BSS'
BSS16           ENDS

BSS32           SEGMENT DWORD FLAT PUBLIC 'BSS'
BSS32           ENDS

c_common        SEGMENT DWORD FLAT PUBLIC 'BSS'
c_common        ENDS

COMMON32        SEGMENT DWORD FLAT PUBLIC 'COMMON'
COMMON32        ENDS

DGROUP          GROUP c_common

; ----------------------------------------------------------------------------
; Constants
; ----------------------------------------------------------------------------

;
; NOT FALSE will be used instead of TRUE to avoid size problems
;
FALSE           =       0

__EMXDOS        =       FALSE

SIGA_KILL               = 0
SIGA_IGNORE             = 1
SIGA_ACCEPT             = 2
SIGA_ERROR              = 3
SIGA_ACKNOWLEDGE        = 4

L_FLAG_DLL              = 0001H                 ; Executable is a DLL
L_FLAG_LINK386          = 0002H                 ; Linked by LINK386

; ----------------------------------------------------------------------------
; Structures
; ----------------------------------------------------------------------------

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

EXC_REGISTER    STRUCT
ES_PREV         DD      ?
ES_HANDLER      DD      ?
EXC_REGISTER    ENDS

TIB             STRUCT
TIB_EXC_CHAIN   DD      ?
TIB_STACK_BASE  DD      ?
TIB_STACK_END   DD      ?
TIB_SYS_PTR     DD      ?
TIB_VERSION     DD      ?
TIB_USER_DATA   DD      ?
TIB             ENDS

TIB2            STRUCT
TID             DWORD   ?
PRIORITY        DWORD   ?
VERSION         DWORD   ?
MC_COUNT        WORD    ?
MC_FORCE_FLAG   WORD    ?
TIB2            ENDS

;
; A REXX string (used by emxio.asm)
;
RXSTRING        STRUCT
RXSTRLEN        DWORD   ?
RXSTRPTR        DWORD   ?
RXSTRING        ENDS


; ----------------------------------------------------------------------------
; Macros
; ----------------------------------------------------------------------------

DALIGN          MACRO   VAL:REQ
                ALIGN   VAL
                ENDM

TALIGN          MACRO   VAL:REQ
                ALIGN   VAL
                ENDM

; End of EMXDLL.INC
