;
; SWAPPER.ASM -- Swap memory pages in and out
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
                INCLUDE OPRINT.INC
                INCLUDE EXCEPT.INC
                INCLUDE PMIO.INC
                INCLUDE PAGING.INC
                INCLUDE MEMORY.INC
                INCLUDE PMINT.INC
                INCLUDE SIGNAL.INC
                INCLUDE PROCESS.INC
                INCLUDE SEGMENTS.INC
                INCLUDE TABLES.INC
                INCLUDE DEBUG.INC

                PUBLIC  SWAP_HANDLE, SWAP_FLAG, SNATCH_COUNT
                PUBLIC  SWAP_FAULTS, SWAP_READS, SWAP_WRITES, SWAP_SIZE
                PUBLIC  PAGE_FAULT, SWAP_OUT, INIT_SWAP, ALLOC_SWAP
                PUBLIC  CLEANUP_SWAP, SET_TMP_DIR

SER_FLAG        =       FALSE                   ; Don't use serial interface

;
; The page directory has two parts.  The first 1024 entries contain
; the page directory entries as required by the 386.  The next 1024
; entries are called the swap directory.  Each entry is either zero
; (no swap  table) or contains the physical address of the swap table
; associated with the page table.
;
; A swap table entry has the following format:
;
;  31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12
;  |---------------Page number in swap/exec file-------------|
;
;  11 10  09 08 07 06 05 04 03 02 01 00
;   0 |A| |SRC| |----Process index----|
;
; Bits   Description
; 0-7    This identifies the owner of the page:
;          00H       not owned, not accessible
;          FFH       (PIDX_SV) owned by supervisor
;          other     process index of owner (cf. P_PIDX)
; 8-9    This field tells where to get the page from:
;          SRC_NONE  no source, just allocate (stack, for example)
;          SRC_ZERO  zero fill (bss, for example)
;          SRC_EXEC  executable file (code, for example)
;          SRC_SWAP  swap file (modified data, for example)
; 10     This bit is set if swap space is allocated to this page (SWAP_ALLOC)
; 11     Currently not used
; 12-31  Page number in executable/swap file, 0=not assigned
;

SV_DATA         SEGMENT

SWAP_ROVER      DD      ?               ; Roving pointer for swapping out
SNATCH_ROVER    DD      ?               ; Roving pointer for getting swap pages

SWAP_FAULTS     DD      0               ; Number of page faults
SWAP_READS      DD      0               ; Number of swap file reads
SWAP_WRITES     DD      0               ; Number of swap file writes
SNATCH_COUNT    DD      0               ; Number of swap pages snatched
SWAP_SIZE       DD      0               ; Size of swap file

;
; Variables for SWAP_OUT
;
SO_SWAP_TABLE   DD      ?               ; Address of swap table

SWAP_HANDLE     DW      NO_FILE_HANDLE  ; Handle of swap file
SWAP_FLAG       DB      FALSE           ; Swapping enabled (initially off)
DISK_FULL       DB      FALSE           ; Disk full while expanding swap file


              IF DEBUG_SWAP
$PF1            DB      "Page fault, SRC=", 0
$PF2            DB      CR, LF, "Address=", 0
$PF3            DB      CR, LF, 0
$SRC_NONE       DB      "NONE", 0
$SRC_ZERO       DB      "ZERO", 0
$SRC_EXEC       DB      "EXEC", 0
$SRC_SWAP       DB      "SWAP", 0
$SWAP_EXEC      DB      "File position=", 0
              ENDIF

$SWAP_OUT_OF_MEM        DB      "Out of memory or swap space", CR, LF, 0
$SWAP_ERROR     DB      "Swap file I/O error", CR, LF, 0
$SWAP_CREATE    DB      "Cannot create swap file", CR, LF, 0
$SWAP_SPACE     DB      "Out of swap space", CR, LF, 0

SWAP_FNAME      DB      64+1+13 DUP (?)

ZERO_BYTE       DB      0

SV_DATA         ENDS



SV_CODE         SEGMENT

                ASSUME  CS:SV_CODE, DS:NOTHING

                .386P

                ASSUME  DS:SV_DATA
                ASSUME  BP:PTR ISTACKFRAME
PAGE_FAULT      PROC    NEAR
              IF SER_FLAG
                SERIAL  "P"
              ENDIF
                MOV     EAX, CR2                ; Get linear address of fault
                STI                             ; Be venturous
;
; Protected-mode interrupt routines must not cause a page fault!
; Protected-mode interrupt routines must not issue INT 21H (buffer)!
; Protected-mode interrupt routines must not access the page tables!
;
                MOV     FAULT_ADDR, EAX
                TEST    I_ERRCD, PAGE_PRESENT   ; Caused by not-present page?
                JNZ     DUMP                    ; No  -> dump
                CMP     EAX, LIN_START          ; Address valid?
                JB      DUMP                    ; No -> dump
                MOV     BX, G_PAGEDIR_SEL
                MOV     ES, BX
                MOV     BX, G_PHYS_SEL
                MOV     FS, BX                  ; Setup physical addressing
                SHR     EAX, 22                 ; Compute page directory index
                MOV     ESI, ES:[4*EAX+0]       ; Get page directory entry
                MOV     EDI, ES:[4*EAX+4096]    ; Get swap directory entry
                TEST    ESI, PAGE_PRESENT       ; Page table present?
                JZ      DUMP                    ; No -> dump
                OR      EDI, EDI                ; Is the swap table present?
                JZ      DUMP                    ; No -> dump
                AND     ESI, NOT 0FFFH          ; Page table address
                MOV     EAX, FAULT_ADDR         ; Get linear address of fault
                SHR     EAX, 12
                AND     EAX, 03FFH              ; Page table index
                LEA     ESI, [ESI+4*EAX]        ; Address of page table entry
                LEA     EDI, [EDI+4*EAX]        ; Address of swap table entry
                MOV     EAX, FS:[EDI]           ; Get swap table entry
                MOV     SWAP_PIDX, AL           ; Save PIDX
                CMP     AL, PIDX_SV             ; Supervisor page?
                JE      SHORT PF_PIDX_OK        ; Yes -> ok
                CMP     AL, 01H                 ; PIDX >= 1?
                JB      DUMP
                CMP     AL, MAX_PROCESSES       ; PIDX <= MAX_PROCESSES?
                JA      DUMP
PF_PIDX_OK:     MOV     AX, BUF_SEG             ; Current buffer segment
                MOV     BX, BUF2_SEG            ; Segment of swapper buffer
                CMP     AX, BX                  ; Already in swapper?
                JE      DUMP                    ; Yes -> dump
                MOV     BUF_SEG, BX             ; Switch to swapper buffer
                MOV     BUF_SEL, G_BUF2_SEL
;
; Don't jump to DUMP from here on, BUF_SEG and BUF_SEL changed!
;
                CALL    GET_PROC                ; Get process table entry
                MOV     FAULT_PROC, BX          ; Save process pointer
                OR      BX, BX                  ; Supervisor?
                JZ      SHORT PF_SV_1           ; Yes -> skip
                INC     (PROCESS PTR [BX]).P_PAGE_FAULTS ; Process' counter
PF_SV_1:        INC     SWAP_FAULTS             ; Totals counter
              IF DEBUG_SWAP
                CMP     DEBUG_FLAG, FALSE
                JE      SHORT PF_02
                LEA     EDX, $PF1
                CALL    OTEXT
                MOV     EAX, FS:[EDI]           ; Get swap table entry
                AND     AX, SRC_MASK
                LEA     EDX, $SRC_ZERO
                CMP     AX, SRC_ZERO
                JE      SHORT PF_01
                LEA     EDX, $SRC_EXEC
                CMP     AX, SRC_EXEC
                JE      SHORT PF_01
                LEA     EDX, $SRC_SWAP
                CMP     AX, SRC_SWAP
                JE      SHORT PF_01
                LEA     EDX, $SRC_NONE
PF_01:          CALL    OTEXT
                LEA     EDX, $PF2
                CALL    OTEXT
                MOV     EAX, FAULT_ADDR
                CALL    ODWORD
                LEA     EDX, $PF3
                CALL    OTEXT
PF_02:
              ENDIF
                MOV     EAX, FS:[ESI]           ; Get page table entry
                TEST    AX, PAGE_ALLOC          ; Memory allocated?
                JZ      SHORT PF_ALLOC          ; No  -> allocate memory
                AND     EAX, NOT 0FFFH          ; Extract address
                JMP     SHORT PF_USE
PF_ALLOC:       CALL    PM_ALLOC                ; Allocate a page (swap out)
                CMP     EAX, NULL_PHYS          ; Failure?
                JE      SWAP_OUT_OF_MEM         ; Yes -> error
PF_USE:         MOV     EDX, EAX                ; Save physical address
                MOV     EAX, FS:[ESI]           ; Page table entry
                AND     EAX, PAGE_CLEAR         ; Clear address, P, D, A
                OR      EAX, EDX                ; Insert phyiscal address
                OR      AX, PAGE_PRESENT OR PAGE_ALLOC ; Page present
                MOV     FS:[ESI], EAX           ; Store page table entry
                CALL    CLEAR_TLB               ; Clear TLB
                AND     EAX, NOT 0FFFH          ; Extract physical address
                MOV     EDX, FS:[EDI]           ; Get swap table entry
                CALL    LOAD_PAGE               ; Initialize the page
                AND     WORD PTR FS:[ESI], NOT PAGE_DIRTY
                CALL    CLEAR_TLB
PF_DONE:
              IF STOP_SWAP AND DEBUG_SWAP
                MOV     AH, 01H
                INT     21H
              ENDIF
                MOV     AX, BUF1_SEG            ; Switch back to main buffer
                MOV     BUF_SEG, AX
                MOV     BUF_SEL, G_BUF1_SEL
              IF SER_FLAG
                SERIAL  "p"
              ENDIF
                JMP     EXCEPT_RET

;
; Error while accessing swap file
;
SWAP_ERROR::    LEA     EDX, $SWAP_ERROR
                CALL    OTEXT
                MOV     AX, 4CFFH
                INT     21H
                JMP     $

;
; Cannot find a memory page for swapping in requested page
;
SWAP_OUT_OF_MEM:
                LEA     EDX, $SWAP_OUT_OF_MEM
                CALL    OTEXT
                MOV     AX, 4CFFH
                INT     21H
                JMP     $

DUMP:           CLI
                RET

                ASSUME  BP:NOTHING
PAGE_FAULT      ENDP



;
; Initialize a page in physical memory.
;
; This procedure fills the page with zeros, loads the page from the
; exec file, loads the page from the swap file or does nothing
; (stack pages).
;
; In:  EAX      Physical address
;      EDX      Swap table entry
;
LOAD_PAGE       PROC    NEAR
                PUSH    ES                      ; Save registers
                PUSH    EAX
                PUSH    EBX
                PUSH    ECX
                PUSH    EDX
                PUSH    EDI
                MOV     EDI, EAX                ; EDI := physical address
                MOV     AX, DX
                AND     AX, SRC_MASK            ; Get type of source
                CMP     AX, SRC_NONE            ; No source (stack)?
                JE      LP_NONE                 ; Yes -> done
                CMP     AX, SRC_ZERO            ; Zero-filled (bss)?
                JE      LP_ZERO                 ; Yes -> fill
                CMP     AX, SRC_EXEC            ; From exec file?
                JE      SHORT LP_EXEC           ; Yes -> read from exec file
;
; Read from swap file
;
LP_SWAP:        INC     SWAP_READS              ; Update statistics
                AND     EDX, NOT 0FFFH          ; Extract address
                MOV     BX, SWAP_HANDLE         ; Handle of swap file
                MOV     ECX, 4096               ; Read one page
                JMP     SHORT LP_READ           ; (EDX is address in file)

;
; Fill page with zeros
;
LP_ZERO:        MOV     BX, G_PHYS_SEL
                MOV     ES, BX
                MOV     ECX, 1024
                XOR     EAX, EAX
                CLD
                REP     STOS DWORD PTR ES:[EDI]
                JMP     LP_RET                  ; Done

;
; Read from exec file
;
LP_EXEC:
              IF DEBUG_SWAP
                CMP     DEBUG_FLAG, FALSE
                JE      SHORT LP_EXEC_1
                LEA     EDX, $SWAP_EXEC
                CALL    OTEXT
                MOV     EAX, FS:[EDI]
                AND     EAX, NOT 0FFFH
                CALL    ODWORD
                CALL    OCRLF
LP_EXEC_1:
              ENDIF
                MOV     BX, FAULT_PROC
                OR      BX, BX                  ; Supervisor (cannot happen)?
                JZ      SWAP_ERROR              ; Yes -> impossible (wrong msg)
                ASSUME  BX:PTR PROCESS
                TEST    [BX].P_FLAGS, PF_PRELOADING     ; Preloading?
                JNZ     SHORT LP_ZERO           ; Yes -> zero page
                AND     EDX, NOT 0FFFH          ; Extract address
                MOV     ECX, 4096
                MOV     EAX, [BX].P_LAST_PAGE
                OR      EAX, EAX                ; Incomplete last page?
                JZ      SHORT LP_EXEC_2         ; No -> skip
                SHL     EAX, 12
                CMP     EDX, EAX                ; On last page?
                JNE     SHORT LP_EXEC_2         ; No  -> skip
                MOVZX   ECX, [BX].P_LAST_BYTES  ; Adjust number of bytes
LP_EXEC_2:      MOV     EAX, [BX].P_SYM_PAGE
                SHL     EAX, 12
                CMP     EDX, EAX                ; Symbol table page?
                JB      SHORT LP_EXEC_3         ; No  -> skip
                ADD     EDX, [BX].P_RELOC_SIZE  ; Adjust page address
LP_EXEC_3:      ADD     EDX, [BX].P_EXEC_OFFSET ; Compute address in file
                MOV     BX, [BX].P_EXEC_HANDLE  ; Get handle of exec file
                ASSUME  BX:NOTHING
;
; Load page from a file (BX=handle, EDX=address)
;
LP_READ:        CALL    SEEK                    ; Move file pointer
                JC      SWAP_ERROR              ; Error -> abort
                MOV     EDX, EDI
                MOV     AX, G_PHYS_SEL          ; Read page from file
                CALL    READ                    ; into physical memory
                JC      SWAP_ERROR              ; Error -> abort
                JNZ     SWAP_ERROR              ; Error -> abort
                ADD     EDI, ECX                ; Compute end address
                SUB     ECX, 4096               ; Number of remaining bytes
                JE      SHORT LP_RET            ; Complete page -> done
                MOV     AX, G_PHYS_SEL
                MOV     ES, AX                  ; Zero rest of page
                NEG     ECX                     ; Number of remaining bytes
                XOR     AL, AL
                CLD
                REP     STOS BYTE PTR ES:[EDI]
LP_NONE:
LP_RET:         POP     EDI                     ; Restore registers
                POP     EDX
                POP     ECX
                POP     EBX
                POP     EAX
                POP     ES
                RET
LOAD_PAGE       ENDP

;
; Get process table entry for process index
;
; In:   SS:BP   Exception stack frame with
;               SWAP_PIDX = process index
;
; Out:  BX      Pointer to process table entry or
;               0000H supervisor page
;
GET_PROC        PROC    NEAR
                MOVZX   BX, SWAP_PIDX           ; Get process index
                CMP     BL, PIDX_SV             ; Supervisor?
                JE      SHORT GET_PROC_SV       ; Yes -> return 0
                DEC     BX                      ; Now zero based
                IMUL    BX, SIZE PROCESS        ; Compute offset
                LEA     BX, PROCESS_TABLE[BX]   ; Compute address
                RET
GET_PROC_SV:    XOR     BX, BX                  ; Return 0 (supervisor)
                RET
GET_PROC        ENDP

;
; Initialize the swap file bitmap. Attention: this causes page
; faults!
;
INIT_SWAP       PROC    NEAR
                MOV     ECX, 4096               ; Bitmap size: 4096 bytes
                LEA     SI, G_SWAP_BMP_DESC     ; Bitmap segment descriptor
                CALL    SV_SEGMENT              ; Create segment
                JC      SHORT IS_RET            ; Out of memory -> don't swap
                MOV     AX, G_SWAP_BMP_SEL
                MOV     ES, AX
                XOR     DI, DI
                MOV     ECX, 4096 / 4
                XOR     EAX, EAX
                NOT     EAX                     ; All pages free
                CLD
                REP     STOS DWORD PTR ES:[DI]
                MOV     SWAP_FLAG, NOT FALSE
                MOV     EAX, LIN_START
                MOV     SWAP_ROVER, EAX
                MOV     SNATCH_ROVER, EAX
IS_RET:         RET
INIT_SWAP       ENDP


;
; Return (in EAX) the physical address of the swapped-out page
; or NULL_PHYS if out of memory.
;
; (This code cannot handle swapping of page-tables (TLB!!!))
;
; Note: If SWAP_ROVER equals LIN_START and there are no pages present,
;       this code makes one superfluous, benign pass.
;

                ASSUME  DS:SV_DATA
SWAP_OUT        PROC    NEAR
                CMP     SWAP_FLAG, FALSE        ; Swapping enabled?
                JNE     SHORT SO_1              ; Yes -> throw out a page
                MOV     EAX, NULL_PHYS          ; Out of memory
                RET

SO_1:           PUSH    ES                      ; Save registers
                PUSH    FS
                PUSH    EBX
                PUSH    ECX
                PUSH    EDX
                PUSH    ESI
                PUSH    EDI
                PUSH    EBP
                MOV     AX, G_PAGEDIR_SEL
                MOV     FS, AX
                MOV     AX, G_PHYS_SEL
                MOV     ES, AX
                MOV     EDI, SWAP_ROVER
                MOV     BP, 0                   ; Count number of rounds
SO_LOOP1:       MOV     EAX, EDI                ; Rover
                SHR     EAX, 22                 ; Compute page directory index
                MOV     EBX, FS:[4*EAX]         ; Get page directory entry
                TEST    EBX, PAGE_PRESENT       ; Page table present?
                JZ      SHORT SO_NTBL0          ; No -> skip to next table
                MOV     EAX, FS:[4*EAX+4096]    ; Address of swap table
                OR      EAX, EAX                ; Does it exist?
                JZ      SHORT SO_NTBL0          ; No -> skip to next table
                MOV     SO_SWAP_TABLE, EAX      ; Save address
                AND     EBX, NOT 0FFFH          ; Address of page table
                MOV     EAX, EDI                ; Compute page table index
                SHR     EAX, 12
                AND     EAX, 03FFH              ; EAX := page table index
                LEA     ESI, [EBX+4*EAX]        ; Address of page table entry
                MOV     ECX, 1024
                SUB     ECX, EAX                ; Number of remaining entries
                TALIGN  4
SO_LOOP2:       MOV     EAX, ES:[ESI]           ; Get page table entry
                TEST    AX, PAGE_ALLOC          ; Memory allocated?
                JZ      SHORT SO_NEXT           ; No -> skip
;
; We use only pages that are swappable and were not accessed since
; the last round.
;
                TEST    AX, PAGE_ACCESSED OR PAGE_LOCKED    ; Both bits zero?
                JZ      SHORT SO_USE            ; Yes -> use this page
                TEST    AX, PAGE_LOCKED         ; Page locked?
                JNZ     SHORT SO_NEXT           ; Yes -> skip
                AND     AL, NOT PAGE_ACCESSED   ; Clear accessed bit
                MOV     ES:[ESI], AL            ; Update page table entry
                TALIGN  4
SO_NEXT:        ADD     ESI, 4                  ; Next entry
                ADD     EDI, 4096
                LOOP    SO_LOOP2                ; All page table entries
                JMP     SHORT SO_NTBL1          ; Next page table
;
; Skip to next page table
;
SO_NTBL0:       AND     EDI, NOT ((1 SHL 22) - 1)   ; First page table entry
                ADD     EDI, 1 SHL 22           ; Next page table
SO_NTBL1:       OR      EDI, EDI                ; End of virtual memory?
                JNZ     SHORT SO_NTBL2          ; No -> continue
                MOV     EDI, LIN_START          ; Restart from beginning
                CMP     BP, 2                   ; Up to 2 rounds allowed
                JAE     SHORT SO_LOOSE          ; More -> failure
                INC     BP                      ; Increment counter
SO_NTBL2:       JMP     SO_LOOP1

SO_LOOSE:       MOV     EAX, NULL_PHYS          ; No pages found, sorry
                JMP     SO_RET

;
; Recycle this page
;
; If the page is dirty and the SWAP_ALLOC bit is not set, allocate a
; swap page.  If the page is dirty, write the page to the swap page.
; Recycle the memory page by returning the address of the page.
; 
                TALIGN  2
SO_USE:         MOV     SWAP_ROVER, EDI
                SUB     EBX, ESI
                NEG     EBX                     ; Offset
                ADD     EBX, SO_SWAP_TABLE      ; Address of swap table entry
                TEST    AL, PAGE_DIRTY          ; Dirty page?
                JZ      SO_RECYCLE              ; No -> just discard
;
; The page is present and dirty.  Write it to the swap file.
;
                MOV     EAX, ES:[EBX]           ; Get swap table entry
                TEST    AX, SWAP_ALLOC          ; Swap space allocated?
                JZ      SHORT SO_ALLOC          ; No  -> allocate swap space
                MOV     EDX, EAX                ; Copy it (address!)
                AND     AX, NOT SRC_MASK        ; Clear SRC field
                OR      AX, SRC_SWAP            ; Reload page from swap file
                AND     EDX, NOT 0FFFH          ; Extract page address
                JMP     SHORT SO_WRITE          ; Write the page

SO_ALLOC:       CALL    ALLOC_SWAP              ; Allocate a page in swap file
                JC      OUT_OF_SWAP_SPACE       ; Out of swap space
                MOV     EDX, EAX                ; Address to EDX
                MOV     EAX, ES:[EBX]           ; Get swap table entry
                AND     EAX, 0FFFH AND NOT SRC_MASK ; Clear address & SRC field
                OR      EAX, EDX                ; Insert address into entry
                OR      AX, SRC_SWAP OR SWAP_ALLOC ; Reload page from swap file
SO_WRITE:       MOV     ES:[EBX], EAX           ; Update swap table entry
                PUSH    EBX
                MOV     BX, SWAP_HANDLE         ; Handle of swap file
                CALL    SEEK                    ; Set file pointer to page
                POP     EBX                     ; (address in EDX)
                JC      SWAP_ERROR              ; Error -> abort
                INC     SWAP_WRITES             ; Update statistics
                MOV     EDX, ES:[ESI]           ; Get physical address of
                AND     EDX, NOT 0FFFH          ; the page
                MOV     AX, G_PHYS_SEL          ; Write the page to the
                MOV     ECX, 4096               ; swap file
                PUSH    EBX
                MOV     BX, SWAP_HANDLE         ; Handle of swap file
                CALL    WRITE
                POP     EBX
                JC      SWAP_ERROR              ; Error -> abort
                JNZ     SWAP_ERROR              ; disk full (cannot happen)
;
; Recycle the physical memory page.
;
SO_RECYCLE:     MOV     EAX, ES:[ESI]           ; Get page table entry
                MOV     ECX, EAX                ; Copy it
                AND     ECX, PAGE_CLEAR         ; Clear address, P, D, A
                AND     CX, NOT PAGE_ALLOC      ; Clear PAGE_ALLOC
SO_DONE:        MOV     ES:[ESI], ECX           ; Update the page table
                AND     EAX, NOT 0FFFH          ; Extract address (ret value)
                CALL    CLEAR_TLB               ; Clear the TLB
SO_RET:         POP     EBP                     ; Restore registers
                POP     EDI
                POP     ESI
                POP     EDX
                POP     ECX
                POP     EBX
                POP     FS
                POP     ES
                RET

;
; Out of swap space.  Display message and abort.
;
OUT_OF_SWAP_SPACE:
                LEA     EDX, $SWAP_SPACE
                CALL    OTEXT
                MOV     AX, 4CFFH
                INT     21H
                JMP     $
SWAP_OUT        ENDP

;
; Try to get a swap page by snatching the swap page of a present page
; which has swap space allocated.  This must not be done if PF_COMMIT
; is set.  The swap page stays marked as used!
;
; As SWAP_OUT calls SWAP_ALLOC only if swap space is not allocated,
; and PAGE_FAULT calls PM_ALLOC only if the page is not pressent,
; there is no danger of snatching the swap space of the page currently
; operated on.
;
; Out: EAX      External address of page (if NC), a multiple of 4096
;      CY       Error (no page available)
;

                ASSUME  DS:SV_DATA
SNATCH_SWAP     PROC    NEAR
                PUSH    ES                      ; Save registers
                PUSH    FS
                PUSH    EBX
                PUSH    ECX
                PUSH    EDX
                PUSH    ESI
                PUSH    EDI
                PUSH    EBP
                MOV     AX, G_PAGEDIR_SEL
                MOV     FS, AX
                MOV     AX, G_PHYS_SEL
                MOV     ES, AX
                MOV     EDI, SNATCH_ROVER
                MOV     BP, 0
SS_LOOP1:       MOV     EAX, EDI                ; Rover
                SHR     EAX, 22                 ; Compute page directory index
                MOV     EBX, FS:[4*EAX+0]       ; Get page directory entry
                TEST    EBX, PAGE_PRESENT       ; Page table present?
                JZ      SHORT SS_NTBL0          ; No -> skip to next table
                MOV     ESI, FS:[4*EAX+4096]    ; Address of swap table
                OR      ESI, ESI                ; Does it exist?
                JZ      SHORT SS_NTBL0          ; No -> skip to next table
                AND     EBX, NOT 0FFFH          ; Address of page table
                MOV     EAX, EDI                ; Compute page table index
                SHR     EAX, 12
                AND     EAX, 03FFH              ; EAX := page table index
                MOV     ECX, 1024
                SUB     ECX, EAX                ; Number of remaining entries
                TALIGN  4
SS_LOOP2:       TEST    WORD PTR ES:[EBX+4*EAX], PAGE_ALLOC ; Memory allocated?
                JZ      SHORT SS_NEXT           ; No  -> skip
                TEST    WORD PTR ES:[ESI+4*EAX], SWAP_ALLOC ; Swap space?
                JNZ     SHORT SS_USE            ; Yes -> use this page
SS_NEXT:        INC     EAX                     ; Next entry
                ADD     EDI, 4096
                CMP     EDI, SNATCH_ROVER       ; All pages examined?
                JE      SHORT SS_FAIL           ; Yes -> no page found
                LOOP    SS_LOOP2                ; All page table entries
                JMP     SHORT SS_NTBL1          ; Next page table
;
; Skip to next page table
;
SS_NTBL0:       AND     EDI, NOT ((1 SHL 22) - 1)   ; First page table entry
                ADD     EDI, 1 SHL 22           ; Next page table
SS_NTBL1:       OR      EDI, EDI                ; End of virtual memory?
                JNZ     SHORT SS_NTBL2          ; No -> continue
                MOV     EDI, LIN_START          ; Restart from beginning
                INC     BP                      ; In case SNATCH_ROVER points
                CMP     BP, 2                   ; into a not-present page table
                JAE     SHORT SS_FAIL           ; stop after 2 loops
SS_NTBL2:       JMP     SS_LOOP1

SS_FAIL:        XOR     EAX, EAX
                STC                             ; No pages found
                JMP     SS_RET

;
; Use the swap space of this page.  This is done by marking the
; page as dirty, clearing the SWAP_ALLOC bit and returning the
; address of the swap page.
;
                TALIGN  2
SS_USE:         MOV     SNATCH_ROVER, EDI
                INC     SNATCH_COUNT            ; Update statistics
                OR      BYTE PTR ES:[EBX+4*EAX], PAGE_DIRTY
                LEA     EDI, [ESI+4*EAX]
                MOV     EDX, ES:[EDI]           ; Get swap table entry
                MOV     EAX, EDX
                AND     EAX, 0FFFH AND NOT SWAP_ALLOC
                MOV     ES:[EDI], EAX
                MOV     EAX, EDX
                AND     EAX, NOT 0FFFH          ; Also clears CY flag
                CALL    CLEAR_TLB               ; Clear the TLB (dirty bit!)
SS_RET:         POP     EBP                     ; Restore registers
                POP     EDI
                POP     ESI
                POP     EDX
                POP     ECX
                POP     EBX
                POP     FS
                POP     ES
                RET
SNATCH_SWAP     ENDP


;
; Open swap file
;
SWAP_OPEN       PROC    NEAR
                CMP     SWAP_HANDLE, NO_FILE_HANDLE
                JNE     SHORT SOP_9             ; Already open -> skip
                PUSH    EAX
                PUSH    ECX
                PUSH    EDX
                PUSH    ESI
                PUSH    EDI
                PUSH    ES
                MOV     AX, DS
                LEA     EDX, SWAP_FNAME
                CALL    CREATE_TMP              ; Create temporary file
                JC      SHORT SOP_ERROR         ; Error -> abort
                MOV     SWAP_HANDLE, AX         ; Save file handle
                POP     ES
                POP     EDI
                POP     ESI
                POP     EDX
                POP     ECX
                POP     EAX
SOP_9:          RET

SOP_ERROR:      LEA     EDX, $SWAP_CREATE
                CALL    OTEXT
                MOV     AX, 4CFFH
                INT     21H
                JMP     $

SWAP_OPEN       ENDP

;
; Allocate a page in the swap file.
;
; 1. Create the swap file if it hasn't been opened yet.
;
; 2. Use a bitmap to locate a free page.  If there is no page
;    available, return with CY set (error; this happens if the size
;    of the swap file exceeds 128 MB).
;
; 3. If the page is beyond the current end of the swap file
;    (taken from the SWAP_SIZE variable) expand the swap file.
;    If this succeeds, update the SWAP_SIZE variable.  If expanding
;    the swap file fails, try to snatch a swap page.  If this fails,
;    return with CY set (error).  Expanding the swap file is not tried
;    if DISK_FULL is set (which is set if expanding the swap file
;    fails).  If snatching a swap page fails, DISK_FULL is cleared
;    and ALLOC_SWAP tries again to expand the swap file.
; 
; 4. Mark the page as used, clear CY (success) and return the page
;    address.
;
; Out: EAX      External address of page (if NC), a multiple of 4096
;      CY       Error
;
ALLOC_SWAP      PROC    NEAR
                PUSH    ES                      ; Save registers
                PUSH    EBX
                PUSH    ECX
                PUSH    EDX
                PUSH    EDI
                CALL    SWAP_OPEN               ; Open swap file
                MOV     AX, G_SWAP_BMP_SEL
                MOV     ES, AX                  ; Access bitmap
                XOR     EDI, EDI                ; Scan bitmap for a
                MOV     ECX, 4096 / 4           ; DWORD with at least
                XOR     EAX, EAX                ; one bit set
                CLD
                REPE    SCAS DWORD PTR ES:[EDI]
                JE      SHORT AS_FAIL           ; All pages in use -> fail
                SUB     EDI, 4                  ; Address of DWORD
                BSF     EAX, DWORD PTR ES:[EDI] ; Find bit in DWORD
                JZ      SHORT $                 ; Cannot happen
                SHL     EDI, 3                  ; 8 bits per bytes
                ADD     EDI, EAX                ; Compute page number
                SHL     EDI, 12                 ; Convert to page address
                LEA     EAX, [EDI+4096]         ; End of page
                CMP     EAX, SWAP_SIZE          ; Grow swap file?
                JBE     SHORT AS_OK             ; No  -> skip
                CMP     DISK_FULL, FALSE        ; Disk full?
                JNE     SHORT AS_SNATCH         ; Yes -> don't try to grow file
                CALL    GROW_SWAP               ; Grow the swap file
                JNC     SHORT AS_OK             ; Success -> done
                CALL    SNATCH_SWAP             ; Try to snatch a swap page
                JNC     SHORT AS_RET            ; Success -> done
                JMP     SHORT AS_FAIL           ; Error

AS_OK:          MOV     EAX, EDI                ; Return page address in EAX
                SHR     EDI, 12                 ; Compute bit number
                BTR     ES:[0], EDI             ; Mark page as used
                JNC     SHORT $                 ; Must not happen
                CLC
AS_RET:         POP     EDI                     ; Restore registers
                POP     EDX
                POP     ECX
                POP     EBX
                POP     ES
                RET

AS_SNATCH:      CALL    SNATCH_SWAP             ; Try to snatch a swap page
                JNC     AS_RET                  ; Success -> use this page
                MOV     DISK_FULL, FALSE        ; Try again to grow swap file
                LEA     EAX, [EDI+4096]         ; New file size
                CALL    GROW_SWAP               ; Disk full?
                JNC     SHORT AS_RET            ; No  -> success
AS_FAIL:        XOR     EAX, EAX                ; Set constant return value
                STC                             ; Error
                JMP     SHORT AS_RET

ALLOC_SWAP      ENDP


;
; Grow the swap file
;
; In:  EAX      New size
;
; Out: CY       Disk full
;
GROW_SWAP       PROC    NEAR
                PUSH    EAX                     ; Save registers
                PUSH    EBX
                PUSH    ECX
                PUSH    EDX
                PUSH    EAX                     ; Save size
                MOV     EDX, EAX                ; New size of swap file
                DEC     EDX                     ; Address of (new) last byte
                MOV     BX, SWAP_HANDLE         ; Handle of swap file
                CALL    SEEK                    ; Set file pointer to last byte
                JC      SWAP_ERROR              ; Error -> abort
                MOV     AX, DS                  ; Write a byte just before the
                LEA     EDX, ZERO_BYTE          ; (new) end of the file
                MOV     ECX, 1                  ; (ECX=0 never fails!)
                CALL    WRITE                   ; Call INT 21H, function 40H
                JC      SWAP_ERROR              ; Error -> abort
                POP     EAX                     ; Restore size
                JNE     SHORT GS_FAIL
                MOV     SWAP_SIZE, EAX          ; Update variable (file size)
                CLC                             ; Success
                JMP     SHORT GS_RET

GS_FAIL:        MOV     DISK_FULL, NOT FALSE    ; Set flag
                STC                             ; Failure
GS_RET:         POP     EDX
                POP     ECX
                POP     EBX
                POP     EAX
                RET
GROW_SWAP       ENDP

SV_CODE         ENDS



INIT_CODE       SEGMENT

                ASSUME  CS:INIT_CODE, DS:NOTHING

;
; Set directory for temporary file
;
; In:   DI=0    EMXTMP and TMP not set
;       ES:DI   Pointer to value of EMXTMP (or TMP)
;
                ASSUME  DS:SV_DATA
SET_TMP_DIR     PROC    NEAR
                OR      DI, DI                  ; EMXTMP or TMP set?
                JZ      SHORT STD_DEFAULT       ; No -> use \
                LEA     BX, SWAP_FNAME          ; Copy value to SWAP_FNAME
                MOV     CX, 64                  ; Copy up to 64 bytes
STD_1:          MOV     AL, ES:[DI]             ; Fetch byte from environment
                MOV     [BX], AL                ; Store to SWAP_FNAME
                INC     DI
                INC     BX
                OR      AL, AL                  ; End reached?
                JZ      SHORT STD_RET           ; Yes -> ok
                LOOP    STD_1                   ; Repeat
STD_DEFAULT:    MOV     SWAP_FNAME[0], "\"      ; Use \ if EMXTMP/TMP not set
                MOV     SWAP_FNAME[1], 0        ; or if the value is too long
STD_RET:        RET
SET_TMP_DIR     ENDP


;
; Close and delete the swap file if it has been created.
; If called from critical error handler, this should not be done.
;
                ASSUME  DS:SV_DATA
CLEANUP_SWAP    PROC    NEAR
                MOV     BX, NO_FILE_HANDLE      ; Get handle of swap file
                XCHG    BX, SWAP_HANDLE         ; and mark the file as closed
                CMP     BX, NO_FILE_HANDLE      ; Swap file created?
                JE      SHORT CUW_RET           ; No -> done
                CMP     CRIT_ERR_FLAG, FALSE    ; Critical error?
                JNE     SHORT CUW_RET           ; Yes -> skip
                MOV     AH, 3EH
                INT     21H                     ; Close swap file
                JC      SHORT CUW_RET           ; Error -> do not delete
                LEA     DX, SWAP_FNAME
                MOV     AH, 41H
                INT     21H                     ; Delete swap file
CUW_RET:        RET
CLEANUP_SWAP    ENDP

INIT_CODE       ENDS

                END
