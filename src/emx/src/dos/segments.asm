;
; SEGMENTS.ASM -- Manage segments
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

__SEGMENTS      =       1

                INCLUDE EMX.INC
                INCLUDE TABLES.INC
                INCLUDE MISC.INC
                INCLUDE PAGING.INC
                INCLUDE SEGMENTS.INC
                INCLUDE SIGNAL.INC              ; Required by PROCESS.INC
                INCLUDE PROCESS.INC

                PUBLIC  NEW_TSS, G_PHYS_BASE
                PUBLIC  CREATE_SEG, SEG_SIZE, SEG_BASE, SEG_ATTR, GET_BASE
                PUBLIC  NULL_SEG, GET_LIN, GET_DESC, SV_SEGMENT, ADD_PAGES
                PUBLIC  ACCESS_LOWMEM, MAP_PHYS
                PUBLIC  INIT_DESC, RM_SEG_BASE, RM_SEG_SIZE, INIT_TSS

SV_DATA         SEGMENT

;
; Base address of G_PHYS_SEL (see also G_PHYS_DESC)
;
G_PHYS_BASE     DD      0
;
; New TSS (with I/O bitmap) active
;
NEW_TSS         DB      FALSE                   ; Initially not active

SV_DATA         ENDS


SV_CODE         SEGMENT

                ASSUME  CS:SV_CODE, DS:NOTHING


;
; Create a segment (make GDT/LDT entry)
;
; In:   SI      Pointer to GDT/LDT entry
;       AX      Segment attributes (A_CODE32, A_DATA32, DPL_0, ..., DPL_3)
;       EBX     Base address of segment
;       ECX     Size of segment (limit+1)
;
                ASSUME  DS:NOTHING

CREATE_SEG      PROC    NEAR
                CALL    SEG_ATTR
                CALL    SEG_BASE
                CALL    SEG_SIZE
                RET
CREATE_SEG      ENDP


;
; Set the type/attributes of a segment
;
; In:   DS:SI   Pointer to descriptor
;       AX      Type / attributes (A_CODE32, A_DATA32, DPL_0, ..., DPL_3)
;
SEG_ATTR        PROC    NEAR
                AND     WORD PTR [SI+5], 0CF00H ; Clear all but G, B|D, limit
                OR      [SI+5], AX              ; Set segment type/attributes
                RET
SEG_ATTR        ENDP


;
; Set the base address of a segment
;
; In:   DS:SI   Pointer to GDT entry
;       EBX     Base address of segment
;
                ASSUME  SI:PTR DESCRIPTOR
SEG_BASE        PROC    NEAR
                PUSH    EBX
                MOV     [SI].BASE_0_15, BX      ; Base 0..15
                SHR     EBX, 16
                MOV     [SI].BASE_16_23, BL     ; Base 16..23
                MOV     [SI].BASE_24_31, BH     ; Base 24..31
                POP     EBX
                RET
                ASSUME  SI:NOTHING
SEG_BASE        ENDP


;
; Set the size of a segment
;
; In:   DS:SI   Pointer to GDT entry (already contains access & other info)
;       ECX     Size of segment (limit+1)
;
                ASSUME  SI:PTR DESCRIPTOR
SEG_SIZE        PROC    NEAR
                PUSH    ECX
                AND     [SI].GRAN, 70H          ; Keep B|0|AVL, clear G|limit
                CMP     ECX, 1 SHL 20           ; Byte granular?
                JA      SHORT SS_BIG            ; No -> page granular
                DEC     ECX                     ; ECX := limit (byte granular)
                MOV     [SI].LIMIT_0_15, CX     ; Limit 0..15
                SHR     ECX, 16
                OR      [SI].GRAN, CL           ; Limit 16..19
                JMP     SHORT SS_RET

SS_BIG:         ADD     ECX, 0FFFH              ; Round up
                SHR     ECX, 12                 ; Convert to number of pages
                DEC     ECX                     ; ECX := limit (page granular)
                MOV     [SI].LIMIT_0_15, CX     ; Limit 0..15
                SHR     ECX, 16
                OR      CL, 80H                 ; Set G
                OR      [SI].GRAN, CL           ; Limit 16..19
SS_RET:         POP     ECX
                RET
                ASSUME  SI:NOTHING
SEG_SIZE        ENDP


;
; Get the base address of a segment
;
; In:   DS:SI   Pointer to GDT entry
;
; Out:  EBX     Base address of segment
;
                ASSUME  SI:PTR DESCRIPTOR
GET_BASE        PROC    NEAR
                MOV     BL, [SI].BASE_16_23     ; Base 16..23
                MOV     BH, [SI].BASE_24_31     ; Base 24..31
                SHL     EBX, 16
                MOV     BX, [SI].BASE_0_15      ; Base 0..15
                RET
                ASSUME  SI:NOTHING
GET_BASE        ENDP

;
; Convert virtual address to linear address
;
; In:   AX      Selector
;       EBX     Offset
;       LDTR    Local descriptor table
;
; Out:  EBX     Linear address
;       CY      Error
;
                ASSUME  DS:SV_DATA
GET_LIN         PROC    NEAR
                PUSH    EAX
                PUSH    ESI
                .386P
                SLDT    SI
                .386
                CALL    GET_DESC
                MOV     EAX, EBX
                CALL    GET_BASE
                ADD     EBX, EAX
                CLC
                POP     ESI
                POP     EAX
                RET
GET_LIN         ENDP

;
; Get LDT/GDT entry
;
; In:   AX      Selector
;       SI      Local descriptor table (selector)
;
; Out:  SI      Pointer to table entry
;
                ASSUME  DS:SV_DATA
GET_DESC        PROC    NEAR
                PUSH    AX
                TEST    AX, 04H                 ; GDT?
                JZ      SHORT GDES_1            ; Yes ->
                AND     SI, NOT 07H
                SUB     SI, G_LDT_SEL
                IMUL    SI, LDT_ENTRIES
                LEA     SI, LDTS[SI]
                JMP     SHORT GDES_2
GDES_1:         LEA     SI, GDT
GDES_2:         AND     AX, NOT 07H
                ADD     SI, AX
                POP     AX
                RET
GET_DESC        ENDP

;
; Create a null segment (make GDT/LDT entry)
;
; In:   DS:SI   Pointer to GDT/LDT entry
;
                ASSUME  DS:NOTHING
NULL_SEG        PROC    NEAR
                MOV     DWORD PTR [SI+0], 0
                MOV     DWORD PTR [SI+4], 0
                RET
NULL_SEG        ENDP


;
; Create supervisor data segment (not swappable)
;
; In:   ECX     Size (bytes)
;       DS:SI   Pointer to GDT entry
;
; Out:  CY      Out of memory
;
; Note: No physical memory is allocated for the pages, so the pages must
;       be present if accessed from within the page fault handler (to avoid
;       recursion)
;
                ASSUME  DS:SV_DATA
SV_SEGMENT      PROC    NEAR
                PUSH    EAX
                PUSH    EBX
                PUSH    ECX
                MOV     AL, PIDX_SV
                CALL    ALLOC_PAGES             ; Get linear addresses
                CALL    INIT_PAGES              ; Create page tables
                JC      SHORT SVS_RET           ; Out of memory -> return
                MOV     EBX, PAGE_LOCKED        ; Create locked pages
                MOV     EDX, PIDX_SV OR SRC_ZERO
                CALL    SET_PAGES
                MOV     EBX, EAX                ; Linear address to EBX
                POP     ECX                     ; Number of bytes
                PUSH    ECX
                MOV     AX, A_DATA32 OR DPL_0   ; Supervisor data segment
                CALL    CREATE_SEG              ; Create segment descriptor
SVS_RET:        POP     ECX
                POP     EBX
                POP     EAX
                RET
SV_SEGMENT      ENDP

;
; Allocate, initialize and switch to new TSS.
;
; If there isn't enough memory, NEW_TSS stays FALSE.
;
                ASSUME  DS:SV_DATA
INIT_TSS        PROC    NEAR
;
; Allocate memory
;
                MOV     ECX, SIZE TSS_STRUC + 2001H
                LEA     SI, G_TMP1_DESC         ; Use temporary descriptor
                CALL    SV_SEGMENT              ; Create supervisor segment
                JC      SHORT IT_RET            ; Out of memory -> done
;
; Copy initial TSS to new segment
;
                PUSH    DS                      ; Save DS
                MOV     AX, G_TSS_MEM_SEL
                MOV     DS, AX
                MOV     AX, G_TMP1_SEL
                MOV     ES, AX
                XOR     SI, SI
                XOR     DI, DI
                MOV     CX, SIZE TSS_STRUC
                REP     MOVSB
;
; Initialize I/O bitmap (all 64K ports disabled)
;
                MOV     AX, 0FFH
                MOV     CX, 2001H
                REP     STOSB
                POP     DS                      ; Restore DS
;
; Copy to G_TSS_MEM_SEL descriptor
;
                MOV_ES_DS                       ; ES := DS
                LEA     SI, G_TMP1_DESC
                LEA     DI, G_TSS_MEM_DESC
                MOVSD
                MOVSD
;
; Copy to G_TSS_SEL descriptor
;
                CLI                             ; Disable interrupts
                LEA     SI, G_TMP1_DESC
                LEA     DI, G_TSS_DESC
                MOVSD
                MOVSD
                LEA     SI, G_TSS_DESC
                MOV     AX, A_TSS               ; Turn it into a TSS desciptor
                CALL    SEG_ATTR
;
; Switch to new TSS
;
                AND     TSS_BUSY, NOT 2
                MOV     AX, G_TSS_SEL
                .386P
                LTR     AX
                .386
                STI                             ; Enable interrupts
                MOV     NEW_TSS, NOT FALSE      ; New TSS active
IT_RET:         RET
INIT_TSS        ENDP

;
; Add some pages of memory to a process.  The pages will be added at the
; end of the data segment, above the stack.
;
; In:  DI       Pointer to process table entry
;      ECX      Number of pages
;
; Out: EAX      Address in process space of the memory (or 0 on failure)
; 
                ASSUME  DS:SV_DATA
ADD_PAGES       PROC    NEAR
                ASSUME  DI:PTR PROCESS
                PUSH    SI
                MOV     EAX, [DI].P_DSEG_SIZE
                ADD     EAX, 1000H              ; One empty page
                PUSH    EAX                     ; Save the return value
                SHL     ECX, 12                 ; Compute number of bytes
                ADD     EAX, ECX                ; Adjust end address
                JC      SHORT AP_FAIL           ; Overflow -> fail
                CMP     EAX, [DI].P_LIN_SIZE    ; Beyond end of address space?
                JA      SHORT AP_FAIL           ; Yes -> fail
                MOV     EDX, EAX                ; New data segment size
                POP     EAX                     ; Address of new pages
                PUSH    EAX
                PUSH    EDX
                ADD     EAX, [DI].P_LINEAR      ; Compute linear address
                SHR     ECX, 12                 ; Number of pages
                MOV     EBX, PAGE_WRITE OR PAGE_USER
                MOV     EDX, SRC_NONE
                TEST    [DI].P_FLAGS, PF_DONT_ZERO
                JNZ     SHORT AP_1
                MOV     EDX, SRC_ZERO
AP_1:           MOV     DL, [DI].P_PIDX
                CALL    SET_COMMIT              ; Set PAGE_ALLOC bit
                CALL    SET_PAGES
                POP     EAX                     ; New data segment size
                JC      SHORT AP_FAIL
                MOV     [DI].P_DSEG_SIZE, EAX   ; Update data segment size
                MOV     ECX, [DI].P_DSEG_SIZE
                MOV     SI, [DI].P_LDT_PTR
                ADD     SI, L_DATA_SEL AND NOT 07H
                CALL    SEG_SIZE
                POP     EAX
                POP     SI
                RET

AP_FAIL:        POP     EAX
                POP     SI
                XOR     EAX, EAX                        ; Return 0
                RET
                ASSUME  DI:NOTHING
ADD_PAGES       ENDP


;
; Map pages into the address space of a process
;
; In:   DI      Pointer to process table entry
;       EBX     Physical address
;       ECX     Number of pages
;
; Out:  EAX     Adress in process space (0 if no space)
;
                ASSUME  DS:SV_DATA
                ASSUME  DI:PTR PROCESS
MAP_PHYS        PROC    NEAR
                MOV     EAX, [DI].P_DSEG_SIZE
                ADD     EAX, 1000H
                PUSH    EAX
                MOV     EDX, ECX
                SHL     EDX, 12
                ADD     EAX, EDX
                JC      SHORT MP_FAIL
                CMP     EAX, [DI].P_LIN_SIZE
                JA      SHORT MP_FAIL
                MOV     [DI].P_DSEG_SIZE, EAX
                SUB     EAX, EDX
                ADD     EAX, [DI].P_LINEAR
                MOV     DL, [DI].P_PIDX
                TEST    EBX, NOT 0FFFH          ; Is the address = 0?
                JNZ     SHORT MP_MORE           ; No  -> continue
                PUSH    ECX
                MOV     ECX, 1                  ; Set one page table entry
                CALL    SET_PAGES               ; This is required because
                POP     ECX                     ; SET_PAGES doesn't increment
                DEC     ECX                     ; the address if it is 0
                JZ      SHORT MP_DONE
                ADD     EAX, 1000H
                ADD     EBX, 1000H
MP_MORE:        CALL    SET_PAGES
MP_DONE:        MOV     ECX, [DI].P_DSEG_SIZE
                MOV     SI, [DI].P_LDT_PTR
                ADD     SI, L_DATA_SEL AND NOT 07H
                CALL    SEG_SIZE
                POP     EAX
                RET

MP_FAIL:        POP     EAX
                XOR     EAX, EAX
                RET
                ASSUME  DI:NOTHING
MAP_PHYS       ENDP


;
; Convert real-mode pointer to protected-mode pointer
;
; In:  EAX      Real-mode Segment:Offset
;
; Out: EBX      Offset into G_LOWMEM_SEL segment
;      EAX      Modified
;
ACCESS_LOWMEM   PROC    NEAR
                ROL     EAX, 16                 ; EAX = offset:segment
                MOVZX   EBX, AX                 ; EBX = segment
                SHL     EBX, 4                  ; EBX = base address of segment
                SHR     EAX, 16                 ; EAX = offset
                ADD     EBX, EAX
                RET
ACCESS_LOWMEM   ENDP

SV_CODE         ENDS



INIT_CODE       SEGMENT

                ASSUME  CS:INIT_CODE, DS:NOTHING

;
; Convert the base field of a GDT entry. It already contains the
; offset into the real-mode segment given in AX.
;
; In:   AX      Real-mode segment
;       DI      Pointer to descriptor table entry (SV_DATA)
;
                ASSUME  DS:SV_DATA
                ASSUME  DI:PTR DESCRIPTOR
INIT_DESC       PROC    NEAR
                PUSH    EAX
                PUSH    EDX
                MOVZX   EAX, AX
                SHL     EAX, 4
                MOVZX   EDX, [DI].BASE_0_15     ; Get real-mode offset
                ADD     EAX, EDX
                CALL    RM_SEG_BASE             ; Set base address
                POP     EDX
                POP     EAX
                RET
                ASSUME  DI:NOTHING
INIT_DESC       ENDP



;
; Set the base address of a segment (real-mode version)
;
; In:   DS:DI   Pointer to GDT entry
;       EAX     Base address of segment
;
                ASSUME  DS:SV_DATA
                ASSUME  DI:PTR DESCRIPTOR
RM_SEG_BASE     PROC    NEAR
                PUSH    EAX
                MOV     [DI].BASE_0_15, AX      ; Base 0..15
                SHR     EAX, 16
                MOV     [DI].BASE_16_23, AL     ; Base 16..23
                MOV     [DI].BASE_24_31, AH     ; Base 24..31
                POP     EAX
                RET
                ASSUME  DI:NOTHING
RM_SEG_BASE     ENDP


;
; Set the size of a segment (real-mode version)
;
; In:   DS:DI   Pointer to GDT entry
;       ECX     Size of segment (limit+1)
;
                ASSUME  DS:SV_DATA
                ASSUME  DI:PTR DESCRIPTOR
RM_SEG_SIZE     PROC    NEAR
                PUSH    ECX
                AND     [DI].GRAN, 70H          ; Keep B|0|AVL, clear G|limit
                CMP     ECX, 1 SHL 20           ; Byte granular?
                JA      SHORT RSS_BIG           ; No -> page granular
                DEC     ECX                     ; ECX := limit (byte granular)
                MOV     [DI].LIMIT_0_15, CX     ; Limit 0..15
                SHR     ECX, 16
                OR      [DI].GRAN, CL           ; Limit 16..19
                JMP     SHORT RSS_RET

RSS_BIG:        ADD     ECX, 0FFFH              ; Round up
                SHR     ECX, 12                 ; Convert to number of pages
                DEC     ECX                     ; ECX := limit (page granular)
                MOV     [DI].LIMIT_0_15, CX     ; Limit 0..15
                SHR     ECX, 16
                OR      CL, 80H                 ; Set G
                OR      [DI].GRAN, CL           ; Limit 16..19
RSS_RET:        POP     ECX
                RET
                ASSUME  DI:NOTHING
RM_SEG_SIZE     ENDP


INIT_CODE       ENDS

                END
