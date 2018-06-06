;
; PAGING.ASM -- Manage page tables
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

__PAGING        =       1

                INCLUDE EMX.INC
                INCLUDE OPRINT.INC
                INCLUDE MEMORY.INC
                INCLUDE TABLES.INC
                INCLUDE SEGMENTS.INC
                INCLUDE VCPI.INC
                INCLUDE PAGING.INC
                INCLUDE SWAPPER.INC
                INCLUDE MISC.INC

                PUBLIC  PAGE_DIR_PHYS, PAGE_DIR_SEG, PAGE_TAB0_SEG
                PUBLIC  LIN_START, PAGING
                PUBLIC  ALLOC_PAGES, INIT_PAGES, SET_PAGES, CLEAR_TLB
                PUBLIC  INIT_LIN_MAP, FREE_LIN, FREE_PAGES, GET_FREE_PAGE
                PUBLIC  INIT_PAGE_BMAP, FREE_AVAIL, CLEAR_DIRTY
                PUBLIC  INIT_PAGING, MAP_PAGES, RM_TO_PHYS, MAYBE_MAP_PAGE

;
; This constant limits physical memory (unless VCPI is used).  The
; value is specified in MB.  Must be a multiple of 4.
;
MAX_PHYS_MEM    =       1024            ; 1 GB
        
SV_DATA         SEGMENT

PAGE_DIR_PHYS   DD      ?               ; Physical address of page directory
                                        ; Use only for putting into CR3!
PAGE_DIR_SEG    DW      ?               ; Real-mode segment of page directory
                                        ; and swap directory
PAGE_TAB0_SEG   DW      ?               ; Real-mode segment of page table 0

TEMP_PAGE_SEG   DW      ?               ; Page for use as temporary page table
TEMP_PAGE_PHYS  DD      ?               ; during memory allocation
;
; Managing linear addresses. Linear addresses are divided into 4096 blocks
; of size 1M each. There's a table containing 4096 bytes, one for each
; block. This byte is either 0 (not allocated), 1..254 (allocated to
; the process with that process index), or 255 (reserved or allocated
; to supervisor).
;
; This could also be done by scanning page tables, but that seems to be
; more complicated.
;
; There's a chicken and egg problem: We have to allocate some linear
; addresses for creating this segment. Allocating linear addresses
; requires this segment. Solution: Allocate the segment in real mode.
; This is simple but wastes low memory. The segment contents are not
; initialized in real mode to avoid duplication of code.
;
LIN_MAP_SEG     DW      ?               ; Real-mode segment

;
; First usable linear address, used for initializing linear address map etc.
;
; Linear address below this address map identically to physical addresses
; (G_PHYS_SEL)
;
LIN_START       DD      ?

;
; We use a bitmap for keeping track of freed pages (FREE_PAGES). We do
; not use SWAP_OUT for allocating freed pages, as SWAP_OUT just swaps
; out the next page with PAGE_ACCESSED cleared (even if it must be
; reloaded from a file or is dirty). For efficiency (memory/disk space),
; freed pages must be reused before trying to swap out pages. To avoid
; scanning the bitmap each time we need a page of physical memory, we
; use a variable for counting freed pages.
;
FREED_PAGES     DD      0
PAGE_BMAP_FLAG  DB      FALSE           ; Page bitmap enabled?

;
; Paging enable flag.
;
PAGING          DB      FALSE           ; Paging disabled

$ALLOC_PAGES    DB      "Linear address space exhausted", CR, LF, 0

              IF DEBUG_MAP
$MMP1           DB      "Map phys=", 0
$MMP2           DB      " lin=", 0
              ENDIF
SV_DATA         ENDS



SV_CODE         SEGMENT

                .386P

                ASSUME  CS:SV_CODE, DS:NOTHING


;
; Initialize linear address map. Creating segments (SV_SEGMENT) is possible
; only after calling this routine.
;
; Reserved linear addresses: 0 ... LIN_START-1
;
; Available linear addresses: LIN_START ... 0FFFFFFFFH
;
; This code requires LIN_START to be a multiple of the linear address
; block size (1M).  Other code requires it to be a multiple of 4M.
; Both INIT_PAGING and INIT_VCPI make LIN_START a multiple of 4M.
;
                ASSUME  DS:SV_DATA
INIT_LIN_MAP    PROC    NEAR
                MOV     AX, G_LIN_MAP_SEL
                MOV     ES, AX
                CLD
                MOV     EBX, LIN_START
                TEST    EBX, (1 SHL 22)-1       ; Must be a multiple of 4M
                JNZ     SHORT $                 ; Impossible!
                SHR     EBX, 20                 ; Divide by 1M
                MOV     CX, BX
                XOR     DI, DI
                MOV     AL, PIDX_SV             ; Address reserved
                REP     STOS BYTE PTR ES:[DI]
                MOV     AL, 00H                 ; Address available
                MOV     CX, 4096
                SUB     CX, BX
                REP     STOS BYTE PTR ES:[DI]
                RET
INIT_LIN_MAP    ENDP


;
; Allocate a range of pages
;
; In:   ECX     Number of bytes
;       AL      Process index
;
; Out:  EAX     Linear address
;       ECX     Number of pages
;

                ASSUME  DS:SV_DATA
ALLOC_PAGES     PROC    NEAR
                PUSH    ES
                PUSH    DX
                PUSH    DI
                ADD     ECX, 0FFFH              ; Round up (pages)
                AND     ECX, NOT 0FFFH
                PUSH    ECX                     ; Save size
                ADD     ECX, (1 SHL 20) - 1     ; Round up (1M)
                SHR     ECX, 20                 ; Number of 1M blocks
                MOV     DX, G_LIN_MAP_SEL
                MOV     ES, DX
                XOR     DI, DI
                XOR     DX, DX                  ; 0 blocks found
                MOV     AH, AL                  ; Save process index in AH
                XOR     AL, AL                  ; Search for empty blocks
AP_LOOP:        CMP     DI, 4096
                JAE     SHORT AP_FAIL           ; Nothing found -> abort
                SCAS    BYTE PTR ES:[DI]        ; Block available?
                JE      SHORT AP_AVAILABLE      ; Yes -> increment counter
                XOR     DX, DX                  ; Reset counter
                JMP     SHORT AP_LOOP

AP_AVAILABLE:   INC     DX                      ; Increment counter
                CMP     DX, CX                  ; Enough blocks found?
                JB      SHORT AP_LOOP           ; No  -> continue
                SUB     DI, CX                  ; Starting block number
                MOV     DX, DI                  ; Save it
                CLD
                MOV     AL, AH                  ; Get process index
                REP     STOS BYTE PTR ES:[DI]   ; Allocate linear addresses
                MOVZX   EAX, DX                 ; Get block number
                SHL     EAX, 20                 ; Compute linear address
                POP     ECX
                SHR     ECX, 12                 ; ECX := number of pages
                POP     DI
                POP     DX
                POP     ES
                RET

AP_FAIL:        LEA     EDX, $ALLOC_PAGES       ; Linear address space
                CALL    OTEXT                   ; exhausted
                MOV     AX, 4CFFH               ; Abort
                INT     21H

ALLOC_PAGES     ENDP


;
; Free linear address space of a (dead) process
;
; In:   AL      Process index (most not be 0FFH!)
;
                ASSUME  DS:SV_DATA
FREE_LIN        PROC    NEAR
                PUSH    ES
                PUSH    CX
                PUSH    DI
                MOV     CX, G_LIN_MAP_SEL
                MOV     ES, CX
                MOV     CX, 4096                ; Examine 4096 entries
                XOR     DI, DI                  ; Start with entry 0
FL_LOOP:        SCAS    BYTE PTR ES:[DI]        ; Matching entry?
                JNE     SHORT FL_NEXT           ; No  -> skip
                MOV     BYTE PTR ES:[DI-1], 0   ; Make block available
FL_NEXT:        LOOP    FL_LOOP
                POP     DI
                POP     CX
                POP     ES
                RET
FREE_LIN        ENDP


;
; Adjust page tables of killed process
;
; In:   AL      Process index (must not be 00H!)
;
                ASSUME  DS:SV_DATA
FREE_PAGES      PROC    NEAR
                PUSH    ES
                PUSH    FS
                PUSH    GS
                PUSHAD
                MOV     CX, G_PAGEDIR_SEL
                MOV     FS, CX                  ; FS: page directory
                MOV     CX, G_PHYS_SEL
                MOV     ES, CX                  ; ES: page tables
                XOR     EBX, EBX
                MOV     DL, AL                  ; Save process index
FP_LOOP1:       MOV     ESI, FS:[EBX]           ; Get page directory entry
                TEST    ESI, 1                  ; Page table present?
                JZ      FP_NEXT_TABLE           ; No -> skip to next table
                MOV     EDI, FS:[EBX+4096]      ; Address of swap table
                OR      EDI, EDI                ; Does it exist?
                JZ      FP_NEXT_TABLE           ; No -> skip to next table
                AND     ESI, NOT 0FFFH          ; Address of page table
                MOV     CX, 1024
FP_LOOP2:       CMP     DL, ES:[EDI]            ; Matching process index?
                JNE     SHORT FP_NEXT_PAGE
                TEST    WORD PTR ES:[EDI], SWAP_ALLOC ; Swap space allocated?
                JZ      SHORT FP_FREE1          ; No -> skip
                MOV     AX, G_SWAP_BMP_SEL
                MOV     GS, AX                  ; GS: swap file bitmap
                MOV     EAX, ES:[EDI]
                SHR     EAX, 12                 ; Block number
                BTS     DWORD PTR GS:[0], EAX   ; Free swap file entry
                JC      SHORT $                 ; Cannot happen
FP_FREE1:       MOV     EAX, ES:[ESI]           ; Get page table entry
                TEST    AX, PAGE_ALLOC          ; Memory allocated?
                JZ      SHORT FP_ZERO           ; No  -> zero page table entry
                SHR     EAX, 12                 ; Compute page number
                CMP     EAX, 4096*8             ; Address beyond bitmap (128M)?
                JAE     SHORT FP_KEEP           ; Yes -> keep page table entry
                CMP     PAGE_BMAP_FLAG, FALSE   ; Page bitmap enabled?
                JE      SHORT FP_KEEP           ; No  -> keep page table entry
                INC     FREED_PAGES             ; One additional freed page
                MOV     BP, G_PAGE_BMP_SEL
                MOV     GS, BP                  ; GS: freed pages bitmap
                BTS     DWORD PTR GS:[0], EAX   ; Free page
                JC      SHORT $                 ; Cannot happen
FP_ZERO:        XOR     EAX, EAX
                MOV     DWORD PTR ES:[EDI], EAX
                MOV     DWORD PTR ES:[ESI], EAX
FP_NEXT_PAGE:   ADD     ESI, 4                  ; Next entry
                ADD     EDI, 4
                INC     EBP                     ; Increment page number
                LOOP    FP_LOOP2                ; All page table entries
FP_NEXT_TABLE:  ADD     EBX, 4                  ; Next page table
                CMP     EBX, 4096               ; End of page directory?
                JB      FP_LOOP1                ; No -> repeat
                CALL    CLEAR_TLB               ; Clear the TLB
                POPAD
                NOP                             ; Avoid 386 bug
                POP     GS
                POP     FS
                POP     ES
                RET

;
; The physical address of the page is beyond 128M (ie, beyond the bitmap).
; Let the swapper recycle this page.  Unfortunately, this almost doesn't
; work, as the PAGE_PRESENT bit is cleared as soon as new linear memory
; is allocated (SET_PAGES).  Nothing evil will happen, but memory beyond
; 128M will almost never be reused.
;
FP_KEEP:        MOV     DWORD PTR ES:[EDI], SRC_NONE
                AND     DWORD PTR ES:[ESI], PAGE_PRESENT OR PAGE_ALLOC OR NOT 0FFFH
                JMP     SHORT FP_NEXT_PAGE

FREE_PAGES      ENDP


;
; Initialize bitmap for freed pages
;
                ASSUME  DS:SV_DATA
INIT_PAGE_BMAP  PROC    NEAR
                MOV     ECX, 4096
                LEA     SI, G_PAGE_BMP_DESC
                CALL    SV_SEGMENT
                JC      SHORT IPB_RET
                MOV     AX, G_PAGE_BMP_SEL
                MOV     ES, AX
                MOV     ECX, 4096 / 4
                XOR     EAX, EAX                ; All pages used
                XOR     DI, DI
                CLD
                REP     STOS DWORD PTR ES:[DI]
                MOV     PAGE_BMAP_FLAG, NOT FALSE
IPB_RET:        RET
INIT_PAGE_BMAP  ENDP

;
; Search bitmap for freed page
;
; Out:  EAX     Physical address of page (or NULL_PHYS if no page found)
;
                ASSUME  DS:SV_DATA
GET_FREE_PAGE   PROC    NEAR
                MOV     EAX, NULL_PHYS          ; No page found
                CMP     FREED_PAGES, 0          ; Are there freed pages?
                JE      SHORT GFP_RET           ; No  -> done
                PUSH    ES
                PUSH    ECX
                PUSH    EDX
                PUSH    EDI
                MOV     AX, G_PAGE_BMP_SEL
                MOV     ES, AX
                XOR     EDI, EDI
                MOV     ECX, 4096 / 4
                XOR     EAX, EAX
                CLD
                REPE    SCAS DWORD PTR ES:[EDI]
                JE      SHORT $                 ; Cannot happen
                BSF     EAX, DWORD PTR ES:[EDI-4]
                JZ      SHORT $                 ; Cannot happen
                BTR     ES:[EDI-4], EAX         ; Clear bit (use page)
                DEC     FREED_PAGES             ; Adjust counter
                LEA     EDX, [EDI-4]
                SHL     EDX, 3                  ; 8 bits per byte
                ADD     EAX, EDX
                SHL     EAX, 12                 ; Convert to physical address
                POP     EDI
                POP     EDX
                POP     ECX
                POP     ES
GFP_RET:        RET
GET_FREE_PAGE   ENDP


;
; Add the number of freed pages to TAVAIL
;
                ASSUME  DS:SV_DATA
FREE_AVAIL      PROC    NEAR
                PUSH    EAX
                MOV     EAX, FREED_PAGES
                ADD     TAVAIL, EAX
                POP     EAX
                RET
FREE_AVAIL      ENDP


;
; Create page tables
;
; In:   EAX     Linear address
;       ECX     Number of pages
;
; Out:  CY=1    Out of memory
;

                ASSUME  DS:SV_DATA

INIT_PAGES      PROC    NEAR
                PUSHAD
                PUSH    ES
                PUSH    FS
                JECXZ   SHORT IP_OK             ; No pages -> done
                MOV     BX, G_PAGEDIR_SEL
                MOV     FS, BX
                MOV     BX, G_PHYS_SEL
                MOV     ES, BX                  ; Address page tables with ES
                MOV     EDI, EAX                ; Bits 22..31 of linear addr.
                SHR     EDI, 22                 ; EDI := page directory index
                MOV     EDX, EAX
                SHR     EDX, 12                 ; Bits 12..21 of linear addr.
                AND     EDX, 03FFH              ; Page table index
;
;    EDI = page directory index
; FS:0   = page directory
;    ECX = number of pages
;    EDX = page table index
;
IP_1:           MOV     EBX, FS:[4*EDI]         ; Get page directory entry
                OR      EBX, EBX                ; Does the entry exist?
                JNZ     SHORT IP_10             ; Yes -> use it
                CALL    PM_ALLOC                ; Allocate swap table
                CMP     EAX, NULL_PHYS          ; Out of memory?
                JE      SHORT IP_ERROR          ; Yes -> error
                CALL    INIT_PAGE_TABLE         ; Clear all entries
                MOV     FS:[4*EDI+4096], EAX    ; Store address
                CALL    PM_ALLOC                ; Allocate page table
                CMP     EAX, NULL_PHYS          ; Out of memory?
                JE      SHORT IP_ERROR          ; Yes -> error
                CALL    INIT_PAGE_TABLE         ; Clear all entries
                MOV     EBX, EAX                ; EBX := addr. of page table
                OR      EBX, PAGE_PRESENT OR PAGE_WRITE OR PAGE_USER
                MOV     FS:[4*EDI+0], EBX       ; Make page directory entry
IP_10:          MOV     EBX, 1024
                SUB     EBX, EDX                ; Number of remaining entries
                SUB     ECX, EBX
                JBE     SHORT IP_OK
                INC     EDI                     ; Next page table
                MOV     EDX, 0                  ; Page table index := 0
                JMP     SHORT IP_1

IP_ERROR:       STC                             ; Error return
                JMP     SHORT IP_RET

IP_OK:          CALL    CLEAR_TLB               ; Clear the TLB
                CLC                             ; Ok
IP_RET:         POP     FS
                POP     ES
                POPAD
                NOP                             ; Avoid 386 bug
                RET
INIT_PAGES      ENDP




;
; Set page table entries
;
; The page tables must exist (cf. INIT_PAGES)!
;
; In:   EAX     Linear address
;       EBX     Page table entry (if address != 0: autoincrement)
;       ECX     Number of pages
;       EDX     Swap table entry (if page number != 0: autoincrement)
;
; Out:  CY      Error (out of memory -- only if PAGE_ALLOC is set)
;
; The PAGE_ALLOC bit in EBX is handled specially.  It tells SET_PAGES
; to allocate memory or swap space for the pages.
;


                ASSUME  DS:SV_DATA

SET_PAGES       PROC    NEAR
                PUSHAD                          ; Save registers
                PUSH    ES
                PUSH    FS
                OR      ECX, ECX
                JZ      SP_OK                   ; No pages -> done
                MOV     BP, G_PAGEDIR_SEL
                MOV     FS, BP
                MOV     BP, G_PHYS_SEL
                MOV     ES, BP                  ; Address page tables with ES
                XCHG    EAX, EBX                ; EAX:=pt entry, EBX:=linear
                MOV     EBP, EBX                ; Linear address
                SHR     EBP, 12
                AND     EBP, 03FFH              ; EBP := page table index
                SHR     EBX, 22                 ; EBX := page directory index
;
; FS:0   = pointer to page directory
;    EBX = page directory index
; ES:ESI = pointer to page table entry
; ES:EDI = pointer to swap table entry
;    EAX = page table entry (=:PTE)
;    ECX = number of pages
;    EDX = swap table entry
;    EBP = page table index
;
SP_1:           MOV     ESI, FS:[4*EBX+0]       ; Get page directory entry
                AND     ESI, NOT 0FFFH          ; Page table address
                JZ      SHORT $                 ; Must not happen
                MOV     EDI, FS:[4*EBX+4096]    ; Get pointer to swap table
                OR      EDI, EDI
                JZ      SHORT $                 ; Must not happen
SP_2:           TEST    AX, PAGE_ALLOC          ; Allocate memory?
                JNZ     SHORT SP_MEM            ; Yes -> allocate
                TEST    DX, SWAP_ALLOC          ; Allocate swap space?
                JNZ     SHORT SP_SWAP           ; Yes -> allocate
SP_CONT:        MOV     ES:[ESI+4*EBP], EAX     ; Store page table entry
                MOV     ES:[EDI+4*EBP], EDX     ; Store swap table entry
                DEC     ECX                     ; Any pages left?
                JZ      SP_OK                   ; No  -> done
                TEST    EAX, NOT 0FFFH          ; Physical address = 0?
                JZ      SHORT SP_3              ; Yes -> skip
                ADD     EAX, 4096               ; No  -> next physical address
SP_3:           TEST    EDX, NOT 0FFFH          ; Page number = 0?
                JZ      SHORT SP_4              ; Yes -> skip
                ADD     EDX, 4096               ; No  -> next page number
SP_4:           INC     EBP                     ; Increment page table index
                CMP     EBP, 1024               ; End of page table?
                JB      SHORT SP_2              ; No  -> continue
                INC     EBX                     ; Next page table
                XOR     EBP, EBP                ; First PTE of this table
                JMP     SHORT SP_1              ; New page table

SP_MEM:         AND     EAX, 0FFFH              ; Clear address
                PUSH    EAX                     ; Save page table entry
                CALL    PM_ALLOC_NOSWAP         ; Allocate memory
                CMP     EAX, NULL_PHYS          ; Success?
                JE      SHORT SP_MEM_FAIL       ; No  -> allocate swap space
                OR      EAX, [ESP]              ; Insert address into PTE
                ADD     ESP, 4                  ; Remove PTE from stack
                JMP     SP_CONT                 ; Store PTE

SP_MEM_FAIL:    POP     EAX                     ; Restore page table entry
                AND     AX, NOT PAGE_ALLOC      ; Stop allocating memory
                OR      DX, SWAP_ALLOC          ; Allocate swap space instead
SP_SWAP:        PUSH    EAX                     ; Save page table entry
                CALL    ALLOC_SWAP              ; Allocate swap space
                JC      SHORT SP_FAIL           ; No space -> error
                AND     EDX, 0FFFH              ; Clear address
                OR      EDX, EAX                ; Make swap table entry
                POP     EAX                     ; Restore page table entry
                JMP     SP_CONT                 ; Continue

SP_FAIL:        POP     EAX                     ; Remove PTE from stack
                ;...undo changes to page tables, deallocate
                ;   swap pages (not yet done)
                STC
                JMP     SHORT SP_RET

SP_OK:          CLC
SP_RET:         CALL    CLEAR_TLB               ; Clear the TLB
                POP     FS                      ; Restore registers
                POP     ES
                POPAD
                NOP                             ; Avoid 386 bug
                RET
SET_PAGES       ENDP


;
; Clear the dirty & accessed bits
;
; In:   EAX     Linear address
;       ECX     Number of pages
;
                ASSUME  DS:SV_DATA
CLEAR_DIRTY     PROC    NEAR
                PUSHAD                          ; Save registers
                PUSH    ES
                PUSH    FS
                OR      ECX, ECX
                JZ      CD_RET                  ; No pages -> done
                MOV     DX, G_PAGEDIR_SEL
                MOV     FS, DX
                MOV     DX, G_PHYS_SEL
                MOV     ES, DX                  ; Address page tables with ES
                MOV     EBX, EAX                ; EBX:=linear address
                MOV     EDX, EBX                ; Linear address
                SHR     EDX, 12
                AND     EDX, 03FFH              ; EDX := page table index
                SHR     EBX, 22                 ; EBX := page directory index
;
; FS:0   = pointer to page directory
;    EBX = page directory index
; ES:ESI = pointer to page table entry
;    ECX = number of pages
;    EDX = page table index
;
CD_1:           MOV     ESI, FS:[4*EBX]         ; Get page directory entry
                AND     ESI, NOT 0FFFH          ; Page table address
                JZ      SHORT $                 ; Must not happen
CD_2:           AND     BYTE PTR ES:[ESI+4*EDX], NOT (PAGE_DIRTY+PAGE_ACCESSED)
                DEC     ECX                     ; Any pages left?
                JZ      CD_RET                  ; No  -> done
                INC     EDX                     ; Increment page table index
                CMP     EDX, 1024               ; End of page table?
                JB      SHORT CD_2              ; No  -> continue
                INC     EBX                     ; Next page table
                XOR     EDX, EDX                ; First PTE of this table
                JMP     SHORT CD_1              ; New page table

CD_RET:         CALL    CLEAR_TLB               ; Clear the TLB
                POP     FS                      ; Restore registers
                POP     ES
                POPAD
                NOP                             ; Avoid 386 bug
                RET
CLEAR_DIRTY     ENDP


;
; Clear the translation lookaside buffer
;
; Note: This procedure must not change the flags (see SET_PAGES).
;
CLEAR_TLB       PROC    NEAR
                PUSH    EAX
                MOV     EAX, CR3
                MOV     CR3, EAX                ; This statement clears the TLB
                POP     EAX
                RET
CLEAR_TLB       ENDP


                ASSUME  DS:NOTHING

;
; Initialize a page table (all pages not present)
; Set 1024 DWORDs to 0
;
; In:   ES:EAX  Address of page table or swap table
;
INIT_PAGE_TABLE PROC    NEAR
                PUSH    EAX
                PUSH    ECX
                PUSH    EDI
                MOV     EDI, EAX
                MOV     ECX, 1024               ; Number of DWORDs
                XOR     EAX, EAX
                CLD
                REP     STOS DWORD PTR ES:[EDI]
                POP     EDI
                POP     ECX
                POP     EAX
                RET
INIT_PAGE_TABLE ENDP

;
; Create a page table for a page, if not already done, which maps
; a page of physical memory below the LIN_START limit.  This function
; returns NULL_PHYS if the page is used for its own page table.
;
; In:   EAX     Physical address
;       EBX     Linear address
;
; Out:  EAX     Physical address (or NULL_PHYS to repeat allocation)
;
                ASSUME  DS:SV_DATA
MAYBE_MAP_PAGE  PROC    NEAR
                PUSH    FS
                PUSH    EDX
                CMP     EAX, LIN_START          ; Address valid in range?
                JAE     SHORT MMP_ERROR         ; No  -> must not happen
                MOV     DX, G_PAGEDIR_SEL       ; Access page directory
                MOV     FS, DX                  ; at FS:0
                MOV     EDX, EBX                ; Compute page directory index
                SHR     EDX, 22
                CMP     DWORD PTR FS:[4*EDX+0], 0  ; Page table initialized?
                JE      MMP_INIT                ; No  -> initialize
MMP_RET:        POP     EDX
                POP     FS
                RET

MMP_ERROR:      INT     3
                JMP     SHORT $

;
; Use the page as its own page table.
;
; First, use the temporary page for the page table to solve the
; chicken and egg problem.
;
; As the linear address is smaller than LIN_START, there will be
; no swap table, so one page is sufficient.
;
; In:   EAX     Physical address
;       EBX     Linear address
;       EDX     Page directory index
;       FS:0    Page directory

MMP_INIT:       PUSH    ES
                PUSH    ECX
                PUSH    ESI
                PUSH    EDI

              IF DEBUG_MAP
                PUSH    EDX
                LEA     EDX, $MMP1
                CALL    OTEXT
                CALL    ODWORD
                LEA     EDX, $MMP2
                CALL    OTEXT
                XCHG    EAX, EBX
                CALL    ODWORD
                XCHG    EAX, EBX
                CALL    OCRLF
                POP     EDX
              ENDIF
;
; Zero the temporary page
;
                PUSH    EAX
                MOV     AX, G_PHYS_SEL
                MOV     ES, AX
                XOR     EAX, EAX
                MOV     ECX, 1024
                MOV     EDI, TEMP_PAGE_PHYS
                CLD
                REP     STOS DWORD PTR ES:[EDI]
                POP     EAX
;
; Set all the page table entries, including the one which points
; to the target page.  This assumes that LIN_START points to a
; 4 MB (page table) boundary.
;
                PUSH    EAX
                MOV     EDI, TEMP_PAGE_PHYS
                MOV     ESI, EBX                ; Linear address
                SHR     ESI, 12
                AND     ESI, 03FFH              ; Page table index               MOV     EDX, EAX                ; EDX := physical address
                SHL     ESI, 10                 ; Relative physical address
                SUB     EAX, ESI                ; First physical address
                OR      EAX, PAGE_PRESENT OR PAGE_WRITE OR PAGE_USER
                MOV     ECX, 1024
                TALIGN  4
MMPI_1:         STOS    DWORD PTR ES:[EDI]
                ADD     EAX, 4096
                LOOP    MMPI_1
                POP     EAX
;
; Set the page directory entry to point to the temporary page
;
                MOV     ECX, TEMP_PAGE_PHYS
                OR      ECX, PAGE_PRESENT OR PAGE_WRITE OR PAGE_USER
                MOV     FS:[4*EDX+0], ECX
                CALL    CLEAR_TLB
;
; Now, the page is accessible and we can copy the temporary page,
; which is simpler than initializing it from scrach
;
                MOV     ESI, TEMP_PAGE_PHYS
                MOV     EDI, EAX                ; Physical address
                MOV     ECX, 1024
                PUSH    DS
                MOV_DS_ES
                ASSUME  DS:NOTHING
                REP     MOVS DWORD PTR ES:[EDI], DWORD PTR DS:[ESI]
                POP     DS
                ASSUME  DS:SV_DATA
;
; Finally, we let the page directory point to the new page
;
                MOV     ECX, EAX                ; Physical address
                OR      ECX, PAGE_PRESENT OR PAGE_WRITE OR PAGE_USER
                MOV     FS:[4*EDX+0], ECX
                CALL    CLEAR_TLB
                POP     EDI
                POP     ESI
                POP     ECX
                POP     ES
                MOV     EAX, NULL_PHYS
                JMP     MMP_RET
MAYBE_MAP_PAGE  ENDP

SV_CODE         ENDS



INIT_CODE       SEGMENT

                ASSUME  CS:INIT_CODE, DS:NOTHING

;
; Initialize paging
;
                ASSUME  DS:SV_DATA

INIT_PAGING     PROC    NEAR
                MOV     AX, 1
                CALL    RM_ALLOC                ; Allocate linear address map
                CMP     AX, NULL_RM             ; Out of memory?
                JE      RM_OUT_OF_MEM           ; Yes -> abort
                MOV     LIN_MAP_SEG, AX         ; Save segment

                MOV     AX, 2                   ; Allocate page directory
                CALL    RM_ALLOC                ; and swap directory
                CMP     AX, NULL_RM             ; Out of memory?
                JE      RM_OUT_OF_MEM           ; Yes -> abort
                MOV     PAGE_DIR_SEG, AX        ; Save segment
                MOV     ES, AX
                MOV     EAX, 0                  ; Zero page directory and
                MOV     DI, 0                   ; swap directory
                CLD
                MOV     ECX, 2*1024
                REP     STOS DWORD PTR ES:[DI]

                CMP     VCPI_FLAG, FALSE        ; VCPI?
                JE      SHORT IPG_1             ; No  -> setup page tables
                CALL    INIT_VCPI               ; Special handling for VCPI
                JMP     SHORT IPG_2             ; Continue

IPG_1:          MOV     FS, PAGE_DIR_SEG
                MOV     SI, 0                   ; FS:SI = pointer to page dir
                MOV     CX, 1                   ; 1 page table (4 MB)
                MOV     EBP, 0                  ; Address = 0
                CALL    MAP_PAGES
                .ERRE   MAX_PHYS_MEM GE 4       ; At least one page table
                .ERRE   MAX_PHYS_MEM LE 2048    ; No point in having more
                .ERRNZ  MAX_PHYS_MEM MOD 4
                MOV     ECX, MAX_PHYS_MEM * 1024 * 1024
                MOV     LIN_START, ECX          ; First unused linear address
                LEA     DI, G_PHYS_DESC
                CALL    RM_SEG_SIZE

IPG_2:          MOV     AX, PAGE_DIR_SEG
                XOR     DX, DX
                CALL    RM_TO_PHYS
                MOV     PAGE_DIR_PHYS, EAX
                MOV     V2P_CR3, EAX            ; For VCPI
                MOV     TSS_EX8_CR3, EAX        ; CR3 for exception 8
                MOV     TSS_EX10_CR3, EAX       ; CR3 for exception 10

                MOVZX   EAX, PAGE_DIR_SEG
                SHL     EAX, 4
                LEA     DI, G_PAGEDIR_DESC
                CALL    RM_SEG_BASE
                MOVZX   EAX, LIN_MAP_SEG
                SHL     EAX, 4
                LEA     DI, G_LIN_MAP_DESC
                CALL    RM_SEG_BASE
;
; Allocate a page to be used as temporary page table during memory
; allocation
;
                MOV     AX, 1
                CALL    RM_ALLOC
                CMP     AX, NULL_RM             ; Out of memory?
                JE      RM_OUT_OF_MEM           ; Yes -> abort
                MOV     TEMP_PAGE_SEG, AX       ; Save segment
                MOV     DX, 0
                CALL    RM_TO_PHYS
                MOV     TEMP_PAGE_PHYS, EAX     ; Save physical address
                RET
INIT_PAGING     ENDP



;
; Map pages (called in real mode)
;
; This function creates only complete page tables (for simplicity)
; Uses RM_TO_PHYS
;
; In:   CX      Number of page tables
;       EBP     Physical address
;       FS:SI   Pointer to page directory entry
;
; Out:  EBP     Next physical address
;       FS:SI   Pointer to next page directory entry
;
; Note: Successive calls initialize successive page tables.
;
MAP_PAGES       PROC    NEAR
                PUSH    DX
MP_OUTER_LOOP:  PUSH    CX                      ; Save page table counter
                MOV     AX, 1                   ; Allocate a page table
                CALL    RM_ALLOC
                CMP     AX, NULL_RM             ; Out of memory?
                JE      RM_OUT_OF_MEM           ; Yes -> abort
                PUSH    AX                      ; Save segment
                XOR     DX, DX                  ; Offset = 0
                CALL    RM_TO_PHYS              ; Get physical address
                OR      EAX, PAGE_PRESENT OR PAGE_WRITE OR PAGE_USER
                MOV     FS:[SI], EAX            ; Store page directory entry
                ADD     SI, 4                   ; Next page directory entry
                POP     ES
                MOV     EAX, EBP                ; Get entry/address
                OR      EAX, PAGE_PRESENT OR PAGE_WRITE OR PAGE_USER
                MOV     CX, 1024                ; Initialize 1024 entries
                MOV     DI, 0
MP_INNER_LOOP:  STOS    DWORD PTR ES:[DI]       ; Store page table entry
                ADD     EAX, 4096               ; Next page
                LOOP    MP_INNER_LOOP           ; Initialize all table entries
                ADD     EBP, 4096*1024          ; Save entry/address
                POP     CX                      ; Restore page table counter
                LOOP    MP_OUTER_LOOP           ; Create all page tables
                POP     DX
                RET
MAP_PAGES       ENDP


;
; Convert a real-mode address to the physical address
;
; In:   AX      Real-mode segment
;       DX      Real-mode offset
;
; Out:  EAX     Physical address
;       EDX     Upper word cleared
;
                ASSUME  DS:SV_DATA
RM_TO_PHYS      PROC    NEAR
                MOVZX   EAX, AX                 ; Clear upper word
                MOVZX   EDX, DX                 ; Clear upper word
                SHL     EAX, 4                  ; Compute linear address
                ADD     EAX, EDX
                CMP     VCPI_FLAG, FALSE        ; VCPI active?
                JE      SHORT RTP_RET           ; No -> physical = linear
                CALL    VCPI_LIN_TO_PHYS
RTP_RET:        RET

RM_TO_PHYS      ENDP


INIT_CODE       ENDS

                END
