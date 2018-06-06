;
; LOADER.ASM -- Load executable files
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

                INCLUDE EMX.INC
                INCLUDE TABLES.INC
                INCLUDE PAGING.INC
                INCLUDE SEGMENTS.INC
                INCLUDE MEMORY.INC
                INCLUDE PMIO.INC
                INCLUDE MISC.INC
                INCLUDE SIGNAL.INC
                INCLUDE PROCESS.INC
                INCLUDE SYMBOLS.INC
                INCLUDE DEBUG.INC
                INCLUDE PMINT.INC
                INCLUDE HEADERS.INC
                INCLUDE OPTIONS.INC
                INCLUDE ERRORS.INC
                INCLUDE VERSION.INC

                PUBLIC  LOADER

HEADER_SIZE     =       SIZE A_OUT_HEADER
              IF (SIZE EXE1_HEADER) GT HEADER_SIZE
HEADER_SIZE     =       SIZE EXE1_HEADER
              ENDIF

SV_DATA         SEGMENT

SYM_SIZE        DWORD   ?
STR_SIZE        DWORD   ?
TEXT_PAGE       DWORD   ?
DATA_PAGE       DWORD   ?

;
; The file header (this buffer is used for different types of headers)
;
HDR_E           LABEL   EXE1_HEADER
HDR_A           LABEL   A_OUT_HEADER
HDR             BYTE    HEADER_SIZE DUP (?)

;
; The emxbind patch area
;
HDR_B           BIND_HEADER     <>

;
; The layout table (aka OS/2 patch area)
;
HDR_L           LAYOUT_TABLE    <>

$CANT_OPEN      BYTE    "Cannot open program file", CR, LF, 0
$CANT_READ      BYTE    "Cannot read program file", CR, LF, 0
$INVALID_PROG   BYTE    "Invalid program file", CR, LF, 0
$INVALID_VER    BYTE    "Wrong emx version", CR, LF, 0
$LOADER_MEM     BYTE    "Out of memory", CR, LF, 0
$INV_OPT        BYTE    "Invalid emx option", CR, LF, 0

SV_DATA         ENDS



SV_CODE         SEGMENT

;
; Load executable file, called by NEW_PROCESS
;
; In:   DI      Pointer to process entry
;       AX:EDX  Pointer to file name
;
; Out:  AX      Return code (0:ok)
;       EDX     Error message (for AX > 0)
;

                ASSUME  CS:SV_CODE, DS:SV_DATA

                ASSUME  DI:PTR PROCESS
LOADER          PROC    NEAR
                CALL    L_OPEN                  ; Open the file
                OR      AX, AX
                JNZ     SHORT LOADER_RET
                CALL    L_HEADER                ; Read header
                OR      AX, AX
                JNZ     SHORT LOADER_CLOSE
                CALL    L_OPTIONS               ; Parse emx options
                OR      AX, AX
                JNZ     SHORT LOADER_CLOSE
                CALL    L_LAYOUT                ; Compute memory layout
                OR      AX, AX
                JNZ     SHORT LOADER_CLOSE
                CALL    L_VMEM                  ; Allocate virtual memory
                OR      AX, AX
                JNZ     SHORT LOADER_CLOSE
                CALL    L_SYMBOLS               ; Load symbols
                OR      AX, AX
                JNZ     SHORT LOADER_CLOSE
                CALL    L_PRELOAD               ; Preload
                OR      AX, AX
                JZ      SHORT LOADER_RET
LOADER_CLOSE:   PUSH    AX
                PUSH    EDX
                MOV     BX, [DI].P_EXEC_HANDLE
                CALL    CLOSE
                POP     EDX
                POP     AX
LOADER_RET:     RET
                ASSUME  DI:NOTHING

;
; Open the file
;
                ASSUME  DI:PTR PROCESS
L_OPEN          PROC    NEAR
                MOV     CL, 20H                 ; Deny write; read access
                CALL    OPEN                    ; Open executable file
                JC      SHORT L_OPEN_ERROR      ; Error -> fail
                MOV     [DI].P_EXEC_HANDLE, AX  ; Save handle
                MOV     BX, AX                  ; Return handle in BX
                XOR     AX, AX                  ; No error
                RET

L_OPEN_ERROR:   MOV     AX, ENOENT              ; Return error code
                LEA     EDX, $CANT_OPEN         ; and error message
                RET
                ASSUME  DI:NOTHING
L_OPEN          ENDP


;
; Read the header
;
                ASSUME  DI:PTR PROCESS
L_HEADER        PROC    NEAR
                MOV     HDR_B.BND_OPTIONS[0], 0 ; This is for plain a.out files
                XOR     EDX, EDX                ; Read header at location 0
NEXT_HEADER:    MOV     [DI].P_EXEC_OFFSET, EDX ; Set offset
                CALL    SEEK                    ; Move to header
                MOV     ECX, SIZE HDR           ; Size of biggest header
                LEA     EDX, HDR
                MOV     AX, DS
                CALL    READ                    ; Read header
                JC      READ_ERROR              ; Error -> fail
                JNZ     INVALID_PROG
                CMP     HDR_E.EXE_MAGIC, 5A4DH  ; exe?
                JNE     SHORT LH_10             ; No  -> not bound
;
; It's an exe file, which works only if it's a bound executable
;
                CMP     [DI].P_EXEC_OFFSET, 0   ; First header?
                JNE     INVALID_PROG            ; No  -> error
                MOVZX   EDX, HDR_E.EXE_HDR_SIZE ; Header size (paragraphs)
                SHL     EDX, 4                  ; Header size (bytes)
                CALL    SEEK                    ; Start of image
                MOV     ECX, SIZE HDR_B
                LEA     EDX, HDR_B
                MOV     AX, DS
                CALL    READ                    ; Read patch area
                JC      READ_ERROR              ; Error -> fail
                JNZ     INVALID_PROG
                ASSUME  DI:NOTHING
                PUSH    ES
                PUSH    SI
                PUSH    DI
                MOV     AX, G_HDR_SEL
                MOV     ES, AX
                ASSUME  ES:HDR_SEG
                XOR     DI, DI
                LEA     SI, HDR_B               ; Compare signature and
                MOV     CX, HDR_VERSION_LEN     ; version
                CLD
                REPE    CMPS BYTE PTR DS:[SI], BYTE PTR ES:[DI]
                POP     DI
                POP     SI
                POP     ES
                JE      SHORT SIG_OK            ; Match -> ok
                MOV     AX, HDR_VERSION_LEN
                SUB     AX, CX                  ; Length of common prefix + 1
                CMP     AX, HDR_EMX_LEN
                JBE     INVALID_PROG            ; Different signature -> fail
;
; Version check is disabled.  Leave checking the version to the C runtime
; startup code, assuming that all DOS calls and system calls required for
; checking the version and displaying the error message are supported by
; all versions of emx.exe (or, vice versa: emx.exe supports the check done
; by the startup code for all versions of the C runtime library).
;
;;;;            JMP     INVALID_VERSION         ; Different version -> fail

                ASSUME  ES:NOTHING
SIG_OK:         CMP     HDR_B.BND_BIND_FLAG, FALSE
                JE      INVALID_PROG            ; Not bound ->
                MOV     EDX, HDR_B.BND_HDR_LOC
                JMP     NEXT_HEADER             ; Redo with next header

;
; Check for a.out header
;
                ASSUME  DI:PTR PROCESS
LH_10:          CMP     HDR_A.A_MAGIC, A_OUT_MAGIC ; GNU a.out -z ?
                JNE     INVALID_PROG            ; No  -> error
                ADD     [DI].P_EXEC_OFFSET, A_OUT_OFFSET - 1000H
                CMP     HDR_A.A_TEXT_SIZE, 0
                JZ      INVALID_PROG
                CMP     HDR_A.A_ENTRY, 10000H   ; Guard against old a.out files
                JNE     INVALID_PROG
;
; Read the layout table, which is required for retrieving the brk
; pointer of a dumped heap.  The layout table is located at the
; beginning of the data segment.
;
                MOV     EDX, HDR_A.A_TEXT_SIZE
                ADD     EDX, 0FFFH              ; Round
                AND     EDX, NOT 0FFFH
                ADD     EDX, [DI].P_EXEC_OFFSET
                ADD     EDX, 1000H              ; See P_EXEC_OFFSET above
                CALL    SEEK                    ; Move to layout table
                MOV     ECX, SIZE HDR_L
                LEA     EDX, HDR_L
                MOV     AX, DS
                CALL    READ                    ; Read header
                JC      READ_ERROR              ; Error -> fail
                JNZ     INVALID_PROG
;
; Check the layout table
;
                CMP     HDR_L.L_TEXT_BASE, 10000H
                JNE     INVALID_PROG
                MOV     AL, BYTE PTR HDR_L.L_FLAGS[3]
                MOV     [DI].P_INTERFACE, AL    ; Set interface version
                XOR     AX, AX                  ; No error
                RET
L_HEADER        ENDP

;
; Parse options.
;
;   1/ emxbind patch area
;   2/ EMXOPT environment variable
;   3/ command line (first process only)
;
L_OPTIONS       PROC    NEAR
;
; Process the emxbind patch area
;
                MOV_ES_DS
                LEA     ESI, HDR_B.BND_OPTIONS
                CALL    PM_OPTIONS
                JC      SHORT LO_ERROR
;
;
; Process the EMXOPT environment variable if present.
;
                MOV     AX, G_ENV_SEL
                MOV     ES, AX
                MOVZX   ESI, ENV_EMXOPT
                OR      ESI, ESI
                JZ      SHORT LO_10
                CALL    PM_OPTIONS
                JC      SHORT LO_ERROR
;
; Process the command line options.  Note that CMDL_OPTIONS[0] is
; set to 0 after starting the first process.  That is, all processes
; but the initial one ignore the command line options.
;
LO_10:          MOV_ES_DS
                LEA     ESI, CMDL_OPTIONS
                CALL    PM_OPTIONS
                JC      SHORT LO_ERROR
                XOR     AX, AX                  ; No error
                RET

LO_ERROR:       MOV     AX, ENOEXEC             ; Error:
                LEA     EDX, $INV_OPT           ; Invalid emx option
                RET
L_OPTIONS       ENDP

;
; Compute the memory layout of the process
;
L_LAYOUT        PROC    NEAR
                MOV     EAX, HDR_A.A_ENTRY
                MOV     [DI].P_ENTRY_POINT, EAX ; Set entry point
                MOV     [DI].P_TEXT_OFF, EAX
                MOV     EAX, HDR_A.A_TEXT_SIZE
                MOV     [DI].P_TEXT_SIZE, EAX   ; Set size of text segment
                MOV     EAX, HDR_A.A_DATA_SIZE
                MOV     [DI].P_DATA_SIZE, EAX
                MOV     EAX, [DI].P_TEXT_OFF
                ADD     EAX, HDR_A.A_TEXT_SIZE
                DEC     EAX
                AND     EAX, NOT 0FFFFH
                ADD     EAX, 10000H
                MOV     [DI].P_DATA_OFF, EAX
;
; Compute offset of bss segment
;
                ADD     EAX, HDR_A.A_DATA_SIZE
                MOV     [DI].P_BSS_OFF, EAX
;
; Compute address of heap: If the heap is loaded from the executable,
; use the heap base address and the brk pointer from the layout table.
; Otherwise, put the heap after the BSS
;
                MOV     EAX, HDR_L.L_HEAP_BRK
                OR      EAX, EAX                ; Preloaded heap?
                JZ      SHORT LL_HEAP_1         ; No  ->
                MOV     [DI].P_BRK, EAX
                MOV     EAX, HDR_L.L_HEAP_BASE
                MOV     [DI].P_INIT_BRK, EAX
                JMP     SHORT LL_HEAP_9

LL_HEAP_1:      MOV     EAX, [DI].P_BSS_OFF
                ADD     EAX, HDR_A.A_BSS_SIZE
                DEC     EAX                     ; Align to 64K
                AND     EAX, NOT 0FFFFH
                ADD     EAX, 10000H
                MOV     [DI].P_BRK, EAX
                MOV     [DI].P_INIT_BRK, EAX
LL_HEAP_9:
;
; Compute address of stack
;
                MOV     EAX, [DI].P_INIT_BRK    ; Heap base address
                AND     EAX, NOT 0FFFH
                ADD     EAX, 1000H
                ADD     EAX, [DI].P_STACK_SIZE
                MOV     [DI].P_STACK_ADDR, EAX
                MOV     [DI].P_DSEG_SIZE, EAX
;
; Compute page numbers
;
                MOV     ECX, 1
                MOV     TEXT_PAGE, ECX
                MOV     EAX, [DI].P_TEXT_SIZE
                ADD     EAX, 0FFFH
                SHR     EAX, 12
                ADD     ECX, EAX
                MOV     DATA_PAGE, ECX
                MOV     EAX, [DI].P_DATA_SIZE
                ADD     EAX, 0FFFH
                SHR     EAX, 12
                ADD     ECX, EAX
                MOV     [DI].P_SYM_PAGE, ECX
;
; Set sizes of bss and symbols areas
;
                MOV     EAX, HDR_A.A_BSS_SIZE
                MOV     [DI].P_BSS_SIZE, EAX
                MOV     EAX, HDR_A.A_DRSIZE
                ADD     EAX, HDR_A.A_TRSIZE
                MOV     [DI].P_RELOC_SIZE, EAX
                MOV     EAX, HDR_A.A_SYM_SIZE
                MOV     SYM_SIZE, EAX
                MOV     STR_SIZE, 0
;
; ...
;
                MOV     [DI].P_LAST_PAGE, 0
                OR      EAX, EAX
                JZ      SHORT LL_29
                CMP     STEP_FLAG, FALSE
                JE      SHORT LL_29
                MOV     EDX, [DI].P_SYM_PAGE
                SHL     EDX, 12
                ADD     EDX, [DI].P_EXEC_OFFSET
                ADD     EDX, [DI].P_RELOC_SIZE
                ADD     EDX, EAX                ; EAX=SYM_SIZE
                CALL    SEEK
                JC      INVALID_PROG
                PUSH    ECX
                LEA     EDX, STR_SIZE
                MOV     AX, DS
                MOV     ECX, 4
                CALL    READ
                POP     ECX
                JC      READ_ERROR
                JNZ     INVALID_PROG
                MOV     EAX, SYM_SIZE
                ADD     EAX, STR_SIZE
                MOV     EDX, EAX
                ADD     EAX, 0FFFH
                SHR     EAX, 12
                ADD     EAX, [DI].P_SYM_PAGE
                DEC     EAX                     ; Number of last page
                AND     EDX, 0FFFH              ; Complete page?
                JZ      SHORT LL_29             ; Yes -> skip
                MOV     [DI].P_LAST_PAGE, EAX   ; Special handling of
                MOV     [DI].P_LAST_BYTES, DX   ; last page
LL_29:          XOR     AX, AX                  ; No error
                RET
                ASSUME  DI:NOTHING
L_LAYOUT        ENDP


;
; Prepare page tables for virtual memory
;
                ASSUME  DI:PTR PROCESS
L_VMEM          PROC    NEAR
;
; Check parameters
;
                TEST    [DI].P_STACK_SIZE, 0FFFH
                JNZ     INVALID_PROG
                TEST    [DI].P_STACK_ADDR, 0FFFH
                JNZ     INVALID_PROG
                TEST    [DI].P_TEXT_OFF, 0FFFH
                JNZ     INVALID_PROG
                TEST    [DI].P_DATA_OFF, 0FFFH
                JNZ     INVALID_PROG
                MOV     EAX, [DI].P_TEXT_OFF
                MOV     ECX, [DI].P_TEXT_SIZE
                ADD     EAX, ECX
                ADD     EAX, 0FFFH
                AND     EAX, NOT 0FFFH
                CMP     EAX, [DI].P_DATA_OFF
                JA      INVALID_PROG
;
; Allocate linear address space and initialize page tables
;
                MOV     EAX, [DI].P_DSEG_SIZE   ; Add 1.5 MB for __memaccess
                ADD     EAX, 180000H            ; and ADD_PAGES
                MOV     [DI].P_LIN_SIZE, EAX
                ADD     EAX, [DI].P_DATA_OFF
                MOV     ECX, EAX                ; Combined size
                MOV     AL, [DI].P_PIDX         ; Get process index
                CALL    ALLOC_PAGES
                MOV     [DI].P_LINEAR, EAX
                CALL    INIT_PAGES
                JC      OUT_OF_MEM
;
; SRC_EXEC pages (code)
;
                MOV     EAX, [DI].P_TEXT_OFF
                ADD     EAX, [DI].P_LINEAR
                MOV     ECX, [DI].P_TEXT_SIZE
                ADD     ECX, 0FFFH
                SHR     ECX, 12
                MOV     EBX, PAGE_USER          ; Page table entry
                MOV     EDX, TEXT_PAGE
                SHL     EDX, 12
                OR      EDX, SRC_EXEC           ; Swap table entry
                MOV     DL, [DI].P_PIDX         ; PIDX
                CALL    SET_PAGES
                MOV     EBX, [DI].P_LINEAR
                MOV     ECX, [DI].P_TEXT_OFF
                ADD     ECX, [DI].P_TEXT_SIZE
                TEST    [DI].P_HW_ACCESS, HW_ACCESS_CODE ; Data executable?
                JZ      SHORT TEXT_EXEC         ; No  -> restrict CS to .text
                MOV     ECX, [DI].P_DSEG_SIZE   ; Big code segment, with stack
TEXT_EXEC:      MOV     SI, [DI].P_LDT_PTR
                ADD     SI, L_CODE_SEL AND NOT 07H
                MOV     AX, A_CODE32 OR DPL_3
                CALL    CREATE_SEG

                MOV     ECX, [DI].P_DSEG_SIZE
                MOV     EAX, [DI].P_LINEAR
                ADD     EAX, [DI].P_DATA_OFF
                SUB     ECX, [DI].P_DATA_OFF
                ADD     ECX, 0FFFH
                SHR     ECX, 12
                MOV     EBX, 000H               ; Page table entry
                MOV     EDX, 000H               ; Swap table entry
                CALL    SET_PAGES               ; Inaccessible

;
; SRC_NONE pages (stack)
;
                MOV     EAX, [DI].P_STACK_ADDR
                ADD     EAX, [DI].P_LINEAR
                MOV     ECX, [DI].P_STACK_SIZE
                SUB     EAX, ECX
                SHR     ECX, 12
                MOV     EBX, PAGE_WRITE OR PAGE_USER
                MOV     EDX, SRC_NONE           ; SRC
                TEST    [DI].P_FLAGS, PF_DONT_ZERO
                JNZ     SHORT LV_04
                MOV     EDX, SRC_ZERO           ; Swap table entry
LV_04:          MOV     DL, [DI].P_PIDX         ; PIDX
                CALL    SET_PAGES               ; Stack
;
; Commit stack pages
;
                TEST    [DI].P_FLAGS, PF_COMMIT
                JZ      SHORT LV_09
                MOV     ECX, [DI].P_COMMIT_SIZE
                ADD     ECX, 4*4096
                CMP     ECX, [DI].P_STACK_SIZE
                JB      SHORT LV_05
                MOV     ECX, [DI].P_STACK_SIZE
LV_05:          MOV     EAX, [DI].P_STACK_ADDR
                ADD     EAX, [DI].P_LINEAR
                SUB     EAX, ECX
                SHR     ECX, 12
                CALL    SET_COMMIT              ; Set PAGE_ALLOC bit
                CALL    SET_PAGES
                JC      OUT_OF_MEM
LV_09:
;
; SRC_ZERO pages (bss)
;
                MOV     EAX, [DI].P_BSS_OFF
                ADD     EAX, [DI].P_LINEAR
                MOV     ECX, [DI].P_BSS_SIZE
                JECXZ   LV_11
                ADD     ECX, EAX
                DEC     ECX
                SHR     ECX, 12                 ; Last bss page
                MOV     EDX, EAX
                SHR     EDX, 12                 ; First bss page
                SUB     ECX, EDX
                INC     ECX
                MOV     EBX, PAGE_WRITE OR PAGE_USER ; Page table entry
                MOV     EDX, SRC_ZERO           ; Swap table entry
                MOV     DL, [DI].P_PIDX         ; PIDX
                CALL    SET_COMMIT              ; Set PAGE_ALLOC bit
                CALL    SET_PAGES               ; Stack
                JC      OUT_OF_MEM              ; ...deallocate memory
LV_11:

;
; SRC_EXEC pages (data)
;
                MOV     EAX, [DI].P_DATA_OFF
                ADD     EAX, [DI].P_LINEAR
                MOV     ECX, [DI].P_DATA_SIZE
                ADD     ECX, 0FFFH
                SHR     ECX, 12
                MOV     EBX, PAGE_WRITE OR PAGE_USER
                MOV     EDX, DATA_PAGE
                SHL     EDX, 12
                OR      EDX, SRC_EXEC
                MOV     DL, [DI].P_PIDX         ; PIDX
                CALL    SET_COMMIT              ; Set PAGE_ALLOC bit
                CALL    SET_PAGES
                JC      OUT_OF_MEM              ; ...deallocate memory

                MOV     EBX, [DI].P_LINEAR
                MOV     ECX, [DI].P_DSEG_SIZE
                MOV     SI, [DI].P_LDT_PTR
                ADD     SI, L_DATA_SEL AND NOT 07H
                MOV     AX, A_DATA32 OR DPL_3
                CALL    CREATE_SEG
                CALL    CLEAR_TLB
                XOR     AX, AX
                RET
                ASSUME  DI:NOTHING
L_VMEM          ENDP

;
; Load symbols
;
                ASSUME  DI:PTR PROCESS
L_SYMBOLS       PROC    NEAR
                CMP     STEP_FLAG, FALSE        ; Debugging?
                JE      SHORT L_SYMBOLS_RET     ; No -> done
                MOV     EAX, SYM_SIZE
                OR      EAX, EAX
                JZ      SHORT L_SYMBOLS_RET
                MOV     [DI].P_STR_OFF, EAX
                XOR     EDX, EDX
                MOV     EBX, SIZE NLIST
                DIV     EBX
                MOV     [DI].P_SYMBOLS, EAX
                MOV     ECX, SYM_SIZE
                ADD     ECX, STR_SIZE
                MOV     AL, [DI].P_PIDX         ; Get process index
                CALL    ALLOC_PAGES
                CALL    INIT_PAGES
                JC      OUT_OF_MEM
                MOV     EBX, PAGE_USER          ; Read only
                MOV     EDX, [DI].P_SYM_PAGE
                SHL     EDX, 12
                OR      EDX, SRC_EXEC           ; SRC
                MOV     DL, [DI].P_PIDX         ; PIDX
                CALL    SET_PAGES
                MOV     ECX, SYM_SIZE
                ADD     ECX, STR_SIZE
                MOV     EBX, EAX
                MOV     SI, [DI].P_LDT_PTR
                ADD     SI, L_SYM_SEL AND NOT 07H
                MOV     AX, A_READ32 OR DPL_3
                CALL    CREATE_SEG
                CALL    CLEAR_TLB
L_SYMBOLS_RET:  XOR     AX, AX
                RET
                ASSUME  DI:NOTHING
L_SYMBOLS       ENDP


;
; Preload code and initialized data
;
                ASSUME  DI:PTR PROCESS
L_PRELOAD       PROC    NEAR
                TEST    [DI].P_FLAGS, PF_PRELOAD; Preloading disabled?
                JNZ     L_PRELOAD_RET           ; Yes -> return
                CALL    PM_AVAIL                ; Number of available pages
                MOV     EDX, EAX                ; of memory
                MOV     EAX, [DI].P_DATA_OFF
                ADD     EAX, [DI].P_DATA_SIZE
                ADD     EAX, 0FFFH              ; Number of pages required
                SHR     EAX, 12
                CMP     EAX, EDX                ; Enough memory?
                JA      L_PRELOAD_RET           ; No  -> don't preload
                OR      [DI].P_FLAGS, PF_PRELOADING     ; Preloading...
                MOV     BX, [DI].P_EXEC_HANDLE  ; Get file handle
                .386P
                SLDT    AX
                PUSH    AX                      ; Save LDTR
                LLDT    [DI].P_LDT              ; Switch to new process
                .386
;
; Preload code
;
                MOV     EDX, TEXT_PAGE
                SHL     EDX, 12
                ADD     EDX, [DI].P_EXEC_OFFSET ; Get location of image
                CALL    SEEK                    ; Seek to code segment
                JC      SHORT L_PRELOAD_ERR
                MOV     ECX, [DI].P_TEXT_SIZE   ; Size of code
                MOV     EDX, [DI].P_TEXT_OFF    ; Destination address
                MOV     AX, L_DATA_SEL
                CALL    READ                    ; Read image
                JC      SHORT L_PRELOAD_ERR
                JNZ     SHORT L_PRELOAD_ERR
                MOV     EAX, [DI].P_TEXT_OFF
                ADD     EAX, [DI].P_LINEAR
                MOV     ECX, [DI].P_TEXT_SIZE
                ADD     ECX, 0FFFH
                SHR     ECX, 12
                CALL    CLEAR_DIRTY
;
; Preload data
;
                MOV     EDX, DATA_PAGE
                SHL     EDX, 12
                ADD     EDX, [DI].P_EXEC_OFFSET ; Get location of image
                CALL    SEEK                    ; Seek to data segment
                JC      SHORT L_PRELOAD_ERR
                MOV     ECX, [DI].P_DATA_SIZE   ; Size of data
                MOV     EDX, [DI].P_DATA_OFF    ; Destination address
                MOV     AX, L_DATA_SEL
                CALL    READ                    ; Read image
                JC      SHORT L_PRELOAD_ERR
                JNZ     SHORT L_PRELOAD_ERR
                MOV     EAX, [DI].P_DATA_OFF
                ADD     EAX, [DI].P_LINEAR
                MOV     ECX, [DI].P_DATA_SIZE
                ADD     ECX, 0FFFH
                SHR     ECX, 12
                CALL    CLEAR_DIRTY
                XOR     AX, AX                  ; NC, ZR
L_PRELOAD_ERR:  POP     AX
                .386P
                LLDT    AX                      ; Restore LDT
                .386
                JC      SHORT READ_ERROR
                JNZ     SHORT INVALID_PROG
                AND     [DI].P_FLAGS, NOT PF_PRELOADING ; End of preloading
L_PRELOAD_RET:  XOR     AX, AX                  ; No error
                RET
                ASSUME  DI:NOTHING
L_PRELOAD       ENDP


READ_ERROR::    MOV     AX, ENOEXEC
                LEA     EDX, $CANT_READ
                RET

INVALID_PROG::  MOV     AX, ENOEXEC
                LEA     EDX, $INVALID_PROG
                RET

INVALID_VERSION::
                MOV     AX, ENOEXEC
                LEA     EDX, $INVALID_VER
                RET

OUT_OF_MEM::    MOV     AX, ENOMEM
                LEA     EDX, $LOADER_MEM
                RET

LOADER          ENDP


SV_CODE         ENDS

                END
